#include "ti_msp_dl_config.h"
#include "jy61p.h"
#include "k230_uart3.h"
#include "motor.h"
#include "sensor.h"

#include <stdbool.h>
#include <stdint.h>

/* 榛戠嚎瀵瑰簲楂樼數骞虫椂涓?锛涜嫢瀹為檯榛戠嚎璇绘暟涓?锛屾敼鎴?銆?*/
#define LINE_ACTIVE_LEVEL        (1U)

/* PWM SysConfig 鍛ㄦ湡涓?00锛屽洜姝ゆ墍鏈夐€熷害鍙傛暟鑼冨洿鏄?锝?00銆?*/
#define BASE_SPEED               (280)
#define MAX_SPEED                (470)
#define MAX_TURN                 (300.0f)

/*
 * 鍒濆鍙傛暟鍋忎繚瀹堛€傝皟鍙傞『搴忥細LINE_KP -> RATE_KP -> LINE_KD -> YAW_KP銆? */
#define LINE_KP                  (1.25f)
#define LINE_KD                  (2.00f)
#define RATE_KP                  (0.80f)
#define YAW_KP                   (1.10f)
#define MAX_DESIRED_RATE_DPS     (130.0f)
#define STRAIGHT_ERROR_LIMIT     (8)

#define CONTROL_PERIOD_MS        (10U)
#define LINE_LOST_STOP_CYCLES    (25U)
#define IMU_STALE_STOP_CYCLES    (20U)

static int clamp_int(int value, int minimum, int maximum)
{
    if (value < minimum) {
        return minimum;
    }
    if (value > maximum) {
        return maximum;
    }
    return value;
}

static float clamp_float(float value, float minimum, float maximum)
{
    if (value < minimum) {
        return minimum;
    }
    if (value > maximum) {
        return maximum;
    }
    return value;
}

static int abs_int(int value)
{
    return (value < 0) ? -value : value;
}

static float wrap_angle_deg(float angle)
{
    while (angle > 180.0f) {
        angle -= 360.0f;
    }
    while (angle < -180.0f) {
        angle += 360.0f;
    }
    return angle;
}

/*
 * 杩斿洖 -40锝?40 鐨勭嚎璺綅缃宸€俽esult[0] 蹇呴』鏄溅浣撳乏渚ф帰澶达紱
 * 鑻ュ疄闄呭乏鍙崇浉鍙嶏紝璋冩崲 weight[] 椤哄簭銆? */
static bool calculate_line_error(
    const uint8_t result[8], int *error, uint8_t *active_count)
{
    static const int8_t weight[8] = {-40, -25, -14, -5, 5, 14, 25, 40};
    int weighted_sum = 0;
    uint8_t count = 0U;
    uint8_t i;

    for (i = 0U; i < 8U; i++) {
        if (result[i] == LINE_ACTIVE_LEVEL) {
            weighted_sum += weight[i];
            count++;
        }
    }

    *active_count = count;
    if (count == 0U) {
        return false;
    }

    *error = weighted_sum / (int) count;
    return true;
}

int main(void)
{
    uint8_t sensor[8] = {0U};
    uint8_t active_count = 0U;
    int line_error = 0;
    int previous_error = 0;
    int last_seen_error = 0;
    uint16_t line_lost_cycles = 0U;
    uint16_t imu_stale_cycles = 0U;
    uint32_t imu_counter;
    uint32_t previous_imu_counter;
    float yaw_reference = 0.0f;
    bool yaw_reference_valid = false;

    SYSCFG_DL_init();
    Motor_Init();
    JY61P_Init();
    K230_UART3_Init();

    /* JY61P 涓婄數鍚庝繚鎸侀潤姝紝绛夊緟鍐呴儴闆跺亸鏍″噯銆?*/
    delay_cycles(CPUCLK_FREQ * 2U);
    previous_imu_counter = JY61P_GetUpdateCounter();

    while (1) {
        float desired_yaw_rate;
        float rate_error;
        float turn;
        int left_speed;
        int right_speed;

        K230_UART3_Process();
        Sensor_Read_All(sensor);

        imu_counter = JY61P_GetUpdateCounter();
        if (imu_counter == previous_imu_counter) {
            if (imu_stale_cycles < UINT16_MAX) {
                imu_stale_cycles++;
            }
        } else {
            imu_stale_cycles = 0U;
            previous_imu_counter = imu_counter;
        }

        /*
         * 鏈敹鍒癑Y61P鏁版嵁锛屾垨杩炵画绾?00ms娌℃湁鏂板抚鏃跺仠杞︼紝闃叉涓插彛鏂嚎鍚?         * 缁х画浣跨敤鏃ц閫熷害銆?         */
        if ((!JY61P_IsValid()) ||
            (imu_stale_cycles > IMU_STALE_STOP_CYCLES)) {
            Motor_Stop();
            delay_cycles(
                (CPUCLK_FREQ / 1000U) * CONTROL_PERIOD_MS);
            continue;
        }

        if (!yaw_reference_valid) {
            yaw_reference = JY61P_GetYaw();
            yaw_reference_valid = true;
        }

        if (calculate_line_error(sensor, &line_error, &active_count)) {
            line_lost_cycles = 0U;
            last_seen_error = line_error;
        } else {
            line_lost_cycles++;
            line_error = (last_seen_error < 0) ? -40 : 40;

            if (line_lost_cycles > LINE_LOST_STOP_CYCLES) {
                Motor_Stop();
                previous_error = line_error;
                delay_cycles(
                    (CPUCLK_FREQ / 1000U) * CONTROL_PERIOD_MS);
                continue;
            }
        }

        /*
         * 澶栫幆锛氱伆搴︿綅缃宸浆鎹负鏈熸湜Z杞磋閫熷害銆?         * 姝ｈ宸〃绀虹嚎璺湪鍙充晶锛岀害瀹氭瑙掗€熷害涔熻〃绀哄悜鍙宠浆銆?         */
        desired_yaw_rate =
            LINE_KP * (float) line_error +
            LINE_KD * (float) (line_error - previous_error);

        /*
         * 鎺ヨ繎鐩寸嚎鏃剁煭鏈熶繚鎸佽埅鍚戯紱杞集鏃舵洿鏂板弬鑰冭锛岄伩鍏嶅叚杞碕Y61P鐨?         * 绉垎鑸悜涓庣伆搴﹁矾绾夸簰鐩稿鎶椼€?         */
        if (abs_int(line_error) <= STRAIGHT_ERROR_LIMIT) {
            desired_yaw_rate +=
                YAW_KP *
                wrap_angle_deg(yaw_reference - JY61P_GetYaw());
        } else {
            yaw_reference = JY61P_GetYaw();
        }

        desired_yaw_rate = clamp_float(
            desired_yaw_rate,
            -MAX_DESIRED_RATE_DPS,
            MAX_DESIRED_RATE_DPS);

        /* 鍐呯幆锛氫娇鐢↗Y61P Z杞磋閫熷害鎶戝埗鎽嗗姩骞惰窡韪湡鏈涜浆鍚戦€熷害銆?*/
        rate_error = desired_yaw_rate - JY61P_GetGyroZ();
        turn = clamp_float(RATE_KP * rate_error, -MAX_TURN, MAX_TURN);

        left_speed = clamp_int(
            BASE_SPEED + (int) turn, -MAX_SPEED, MAX_SPEED);
        right_speed = clamp_int(
            BASE_SPEED - (int) turn, -MAX_SPEED, MAX_SPEED);

        Motor_SetDifferential(left_speed, right_speed);
        previous_error = line_error;

        delay_cycles((CPUCLK_FREQ / 1000U) * CONTROL_PERIOD_MS);
    }
}

