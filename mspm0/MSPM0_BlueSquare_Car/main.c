#include "ti_msp_dl_config.h"

#include <stdbool.h>
#include <stdint.h>

/*
 * PB3 <- K230 GPIO3 UART1_TX; PB2 -> K230 GPIO4 UART1_RX.
 * Left Motor C: PA21=PWMC, PB6=CIN1, PB7=CIN2.
 * Right Motor B: PA22=PWMB, PB8=BIN1, PB9=BIN2.
 */
#define MOTOR_LEFT_PWM_PORT GPIOA
#define MOTOR_LEFT_PWM_PIN DL_GPIO_PIN_21
#define MOTOR_RIGHT_PWM_PORT GPIOA
#define MOTOR_RIGHT_PWM_PIN DL_GPIO_PIN_22
#define MOTOR_DIRECTION_PORT GPIOB
#define MOTOR_LEFT_IN1_PIN DL_GPIO_PIN_6
#define MOTOR_LEFT_IN2_PIN DL_GPIO_PIN_7
#define MOTOR_RIGHT_IN1_PIN DL_GPIO_PIN_8
#define MOTOR_RIGHT_IN2_PIN DL_GPIO_PIN_9
#define LEFT_FORWARD_IN1_HIGH (1)
#define RIGHT_FORWARD_IN1_HIGH (1)
#define CONTROL_TICK_MS (1U)
#define COMMAND_TIMEOUT_MS (300U)
#define PWM_PHASE_COUNT (2U)
#define PWM_ON_PHASES (1U)
#define RX_BUFFER_SIZE (32U)

static volatile uint8_t g_rx_buffer[RX_BUFFER_SIZE];
static volatile uint8_t g_rx_head = 0U;
static volatile uint8_t g_rx_tail = 0U;
static bool g_drive_requested = false;
static uint32_t g_ms_since_command = COMMAND_TIMEOUT_MS + 1U;
static uint8_t g_pwm_phase = 0U;

static const DL_UART_Main_ClockConfig g_uart3_clock_config = {
    .clockSel = DL_UART_MAIN_CLOCK_BUSCLK,
    .divideRatio = DL_UART_MAIN_CLOCK_DIVIDE_RATIO_1
};

static const DL_UART_Main_Config g_uart3_config = {
    .mode = DL_UART_MAIN_MODE_NORMAL,
    .direction = DL_UART_MAIN_DIRECTION_TX_RX,
    .flowControl = DL_UART_MAIN_FLOW_CONTROL_NONE,
    .parity = DL_UART_MAIN_PARITY_NONE,
    .wordLength = DL_UART_MAIN_WORD_LENGTH_8_BITS,
    .stopBits = DL_UART_MAIN_STOP_BITS_ONE
};

static void set_direction_pair(uint32_t in1_pin, uint32_t in2_pin, bool in1_high)
{
    if (in1_high) {
        DL_GPIO_setPins(MOTOR_DIRECTION_PORT, in1_pin);
        DL_GPIO_clearPins(MOTOR_DIRECTION_PORT, in2_pin);
    } else {
        DL_GPIO_clearPins(MOTOR_DIRECTION_PORT, in1_pin);
        DL_GPIO_setPins(MOTOR_DIRECTION_PORT, in2_pin);
    }
}

static void Motor_Stop(void)
{
    DL_GPIO_clearPins(MOTOR_LEFT_PWM_PORT, MOTOR_LEFT_PWM_PIN);
    DL_GPIO_clearPins(MOTOR_RIGHT_PWM_PORT, MOTOR_RIGHT_PWM_PIN);
    DL_GPIO_clearPins(
        MOTOR_DIRECTION_PORT,
        MOTOR_LEFT_IN1_PIN | MOTOR_LEFT_IN2_PIN |
        MOTOR_RIGHT_IN1_PIN | MOTOR_RIGHT_IN2_PIN);
}

static void Motor_SetForwardDirection(void)
{
    set_direction_pair(MOTOR_LEFT_IN1_PIN, MOTOR_LEFT_IN2_PIN,
        LEFT_FORWARD_IN1_HIGH != 0);
    set_direction_pair(MOTOR_RIGHT_IN1_PIN, MOTOR_RIGHT_IN2_PIN,
        RIGHT_FORWARD_IN1_HIGH != 0);
}

static void Motor_UpdatePwm(void)
{
    bool pwm_on;
    if ((!g_drive_requested) || (g_ms_since_command > COMMAND_TIMEOUT_MS)) {
        Motor_Stop();
        return;
    }
    Motor_SetForwardDirection();
    pwm_on = (g_pwm_phase < PWM_ON_PHASES);
    if (pwm_on) {
        DL_GPIO_setPins(MOTOR_LEFT_PWM_PORT, MOTOR_LEFT_PWM_PIN);
        DL_GPIO_setPins(MOTOR_RIGHT_PWM_PORT, MOTOR_RIGHT_PWM_PIN);
    } else {
        DL_GPIO_clearPins(MOTOR_LEFT_PWM_PORT, MOTOR_LEFT_PWM_PIN);
        DL_GPIO_clearPins(MOTOR_RIGHT_PWM_PORT, MOTOR_RIGHT_PWM_PIN);
    }
}

static void UART3_Init(void)
{
    DL_UART_Main_reset(UART3);
    DL_GPIO_enablePower(GPIOB);
    DL_UART_Main_enablePower(UART3);
    DL_GPIO_initPeripheralOutputFunction(
        IOMUX_PINCM15, IOMUX_PINCM15_PF_UART3_TX);
    DL_GPIO_initPeripheralInputFunction(
        IOMUX_PINCM16, IOMUX_PINCM16_PF_UART3_RX);
    DL_UART_Main_setClockConfig(
        UART3, (DL_UART_Main_ClockConfig *) &g_uart3_clock_config);
    DL_UART_Main_init(UART3, (DL_UART_Main_Config *) &g_uart3_config);
    DL_UART_Main_configBaudRate(UART3, 32000000U, 115200U);
    DL_UART_Main_enableInterrupt(UART3, DL_UART_MAIN_INTERRUPT_RX);
    DL_UART_Main_enable(UART3);
    NVIC_ClearPendingIRQ(UART3_INT_IRQn);
    NVIC_EnableIRQ(UART3_INT_IRQn);
}

static bool Rx_Read(uint8_t *value)
{
    if (g_rx_head == g_rx_tail) {
        return false;
    }
    *value = g_rx_buffer[g_rx_tail];
    g_rx_tail = (uint8_t)((g_rx_tail + 1U) % RX_BUFFER_SIZE);
    return true;
}

static void ProcessUartCommands(void)
{
    static const char prefix[] = "DRIVE,";
    static uint8_t prefix_index = 0U;
    static uint8_t command_value = 0U;
    static bool value_received = false;
    uint8_t byte;

    while (Rx_Read(&byte)) {
        if (prefix_index < (sizeof(prefix) - 1U)) {
            if (byte == (uint8_t) prefix[prefix_index]) {
                prefix_index++;
            } else {
                prefix_index = (byte == 'D') ? 1U : 0U;
            }
            value_received = false;
            continue;
        }
        if (!value_received) {
            if ((byte == '0') || (byte == '1')) {
                command_value = (uint8_t)(byte - '0');
                value_received = true;
            } else {
                prefix_index = (byte == 'D') ? 1U : 0U;
            }
            continue;
        }
        if (byte == '\r') {
            continue;
        }
        if (byte == '\n') {
            g_drive_requested = (command_value != 0U);
            g_ms_since_command = 0U;
        }
        prefix_index = (byte == 'D') ? 1U : 0U;
        value_received = false;
    }
}

int main(void)
{
    SYSCFG_DL_init();
    Motor_Stop();
    UART3_Init();
    while (1) {
        ProcessUartCommands();
        if (g_ms_since_command <= COMMAND_TIMEOUT_MS) {
            g_ms_since_command += CONTROL_TICK_MS;
        }
        g_pwm_phase = (uint8_t)((g_pwm_phase + 1U) % PWM_PHASE_COUNT);
        Motor_UpdatePwm();
        delay_cycles((CPUCLK_FREQ / 1000U) * CONTROL_TICK_MS);
    }
}

void UART3_IRQHandler(void)
{
    if (DL_UART_Main_getPendingInterrupt(UART3) == DL_UART_MAIN_IIDX_RX) {
        uint8_t data = (uint8_t) DL_UART_Main_receiveData(UART3);
        uint8_t next = (uint8_t)((g_rx_head + 1U) % RX_BUFFER_SIZE);
        if (next != g_rx_tail) {
            g_rx_buffer[g_rx_head] = data;
            g_rx_head = next;
        }
    }
}
