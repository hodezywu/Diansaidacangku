#include "jy61p.h"
#include "ti_msp_dl_config.h"

/*
 * 璁╁皬杞﹀師鍦板悜鍙虫棆杞細鑻?JY61P_GetGyroZ() 鍙樹负璐熸暟锛屾妸杩欓噷鏀规垚 -1.0f銆? */
#define JY61P_GYRO_Z_SIGN (1.0f)

static volatile float g_gyro_z_dps = 0.0f;
static volatile float g_yaw_deg = 0.0f;
static volatile bool g_valid = false;
static volatile uint32_t g_update_counter = 0U;

static int16_t read_i16_le(const uint8_t *low)
{
    return (int16_t) (((uint16_t) low[1] << 8U) | (uint16_t) low[0]);
}

static void process_frame(const uint8_t frame[11])
{
    int16_t raw_z;

    if (frame[1] == 0x52U) {
        raw_z = read_i16_le(&frame[6]);
        g_gyro_z_dps =
            JY61P_GYRO_Z_SIGN * ((float) raw_z * 2000.0f / 32768.0f);
        g_valid = true;
        g_update_counter++;
    } else if (frame[1] == 0x53U) {
        raw_z = read_i16_le(&frame[6]);
        g_yaw_deg = (float) raw_z * 180.0f / 32768.0f;
        g_valid = true;
        g_update_counter++;
    }
}

static void push_byte(uint8_t data)
{
    static uint8_t frame[11];
    static uint8_t index = 0U;
    uint16_t checksum = 0U;
    uint8_t i;

    if ((index == 0U) && (data != 0x55U)) {
        return;
    }

    frame[index++] = data;
    if (index < 11U) {
        return;
    }

    for (i = 0U; i < 10U; i++) {
        checksum += frame[i];
    }

    if ((uint8_t) checksum == frame[10]) {
        process_frame(frame);
    }

    index = 0U;
}

void JY61P_Init(void)
{
    NVIC_ClearPendingIRQ(IMU_UART_INST_INT_IRQN);
    NVIC_EnableIRQ(IMU_UART_INST_INT_IRQN);
}

bool JY61P_IsValid(void)
{
    return g_valid;
}

float JY61P_GetGyroZ(void)
{
    return g_gyro_z_dps;
}

float JY61P_GetYaw(void)
{
    return g_yaw_deg;
}

uint32_t JY61P_GetUpdateCounter(void)
{
    return g_update_counter;
}

void IMU_UART_INST_IRQHandler(void)
{
    switch (DL_UART_Main_getPendingInterrupt(IMU_UART_INST)) {
        case DL_UART_MAIN_IIDX_RX:
            push_byte((uint8_t) DL_UART_Main_receiveData(IMU_UART_INST));
            break;

        default:
            break;
    }
}

