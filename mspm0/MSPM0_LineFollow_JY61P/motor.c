#include "motor.h"
#include "ti_msp_dl_config.h"

/*
 * 濡傛灉鏋剁┖娴嬭瘯鏃舵煇涓€渚ф閫熷害鍗村悜鍚庤浆锛屽彧淇敼瀵瑰簲鐨勫畯锛屼笉瑕佸悓鏃惰皟鎹? * PWM 閫氶亾鍜屽乏鍙宠疆瀹氫箟銆? */
#define LEFT_FORWARD_CIN1_HIGH   (1)
#define RIGHT_FORWARD_BIN1_HIGH  (1)

static int clamp_speed(int speed)
{
    if (speed > 500) {
        return 500;
    }
    if (speed < -500) {
        return -500;
    }
    return speed;
}

static void set_left_direction(int forward)
{
    if ((forward != 0) == (LEFT_FORWARD_CIN1_HIGH != 0)) {
        DL_GPIO_setPins(MOTOR_DIR_PORT, MOTOR_DIR_CIN1_PIN);
        DL_GPIO_clearPins(MOTOR_DIR_PORT, MOTOR_DIR_CIN2_PIN);
    } else {
        DL_GPIO_clearPins(MOTOR_DIR_PORT, MOTOR_DIR_CIN1_PIN);
        DL_GPIO_setPins(MOTOR_DIR_PORT, MOTOR_DIR_CIN2_PIN);
    }
}

static void set_right_direction(int forward)
{
    if ((forward != 0) == (RIGHT_FORWARD_BIN1_HIGH != 0)) {
        DL_GPIO_setPins(MOTOR_DIR_PORT, MOTOR_DIR_BIN1_PIN);
        DL_GPIO_clearPins(MOTOR_DIR_PORT, MOTOR_DIR_BIN2_PIN);
    } else {
        DL_GPIO_clearPins(MOTOR_DIR_PORT, MOTOR_DIR_BIN1_PIN);
        DL_GPIO_setPins(MOTOR_DIR_PORT, MOTOR_DIR_BIN2_PIN);
    }
}

/**
 * @brief  鍒濆鍖栫數鏈虹浉鍏冲璁? * @note   PWM 鍜?GPIO 宸茬敱 SysConfig 鍒濆鍖栥€? */
void Motor_Init(void)
{
    DL_TimerA_startCounter(PWM_MOTOR_INST);
    Motor_Stop();
}

void Motor_Stop(void)
{
    DL_TimerA_setCaptureCompareValue(
        PWM_MOTOR_INST, 0U, DL_TIMER_CC_0_INDEX);
    DL_TimerA_setCaptureCompareValue(
        PWM_MOTOR_INST, 0U, DL_TIMER_CC_1_INDEX);

    DL_GPIO_clearPins(
        MOTOR_DIR_PORT,
        MOTOR_DIR_CIN1_PIN | MOTOR_DIR_CIN2_PIN |
        MOTOR_DIR_BIN1_PIN | MOTOR_DIR_BIN2_PIN);
}

/**
 * @brief  璁剧疆宸﹀彸杞樊閫熷強鏂瑰悜
 * @param  left_speed:  宸﹁疆閫熷害 (-500 ~ 500)锛屾鏁板墠杩涳紝璐熸暟鍚庨€€
 * @param  right_speed: 鍙宠疆閫熷害 (-500 ~ 500)锛屾鏁板墠杩涳紝璐熸暟鍚庨€€
 */
void Motor_SetDifferential(int left_speed, int right_speed) 
{
    left_speed = clamp_speed(left_speed);
    right_speed = clamp_speed(right_speed);

    if (left_speed >= 0) {
        set_left_direction(1);
    } else {
        set_left_direction(0);
        left_speed = -left_speed;
    }

    if (right_speed >= 0) {
        set_right_direction(1);
    } else {
        set_right_direction(0);
        right_speed = -right_speed;
    }

    DL_TimerA_setCaptureCompareValue(
        PWM_MOTOR_INST, (uint32_t) left_speed, DL_TIMER_CC_0_INDEX);
    DL_TimerA_setCaptureCompareValue(
        PWM_MOTOR_INST, (uint32_t) right_speed, DL_TIMER_CC_1_INDEX);
}

