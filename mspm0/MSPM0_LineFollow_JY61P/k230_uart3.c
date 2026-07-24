#include "k230_uart3.h"
#include "ti_msp_dl_config.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#define UART3_RX_BUFFER_SIZE 128U
#define K230_UART3_MAX_SEQUENCE_DIGITS 10U

volatile uint32_t g_k230_rx_byte_count = 0U;
volatile uint32_t g_k230_valid_ping_count = 0U;
volatile uint32_t g_k230_tx_pong_count = 0U;
volatile uint32_t g_k230_parse_error_count = 0U;
volatile uint32_t g_k230_rx_overflow_count = 0U;
volatile uint32_t g_k230_last_sequence = 0U;

static volatile uint8_t g_rx_buffer[UART3_RX_BUFFER_SIZE];
static volatile uint16_t g_rx_head = 0U;
static volatile uint16_t g_rx_tail = 0U;

static enum {
    K230_RX_STATE_WAIT_P = 0,
    K230_RX_STATE_WAIT_I,
    K230_RX_STATE_WAIT_N,
    K230_RX_STATE_WAIT_G,
    K230_RX_STATE_WAIT_COMMA,
    K230_RX_STATE_READ_SEQ,
    K230_RX_STATE_WAIT_LF
} g_k230_rx_state = K230_RX_STATE_WAIT_P;

static uint32_t g_k230_rx_sequence = 0U;
static uint8_t g_k230_rx_digit_count = 0U;

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

static uint16_t RxBuffer_Count(void)
{
    return (uint16_t)((UART3_RX_BUFFER_SIZE + g_rx_head - g_rx_tail) % UART3_RX_BUFFER_SIZE);
}

bool K230_UART3_Available(void)
{
    return (RxBuffer_Count() != 0U);
}

uint16_t K230_UART3_GetAvailableCount(void)
{
    return RxBuffer_Count();
}

bool K230_UART3_ReadByte(uint8_t *byte)
{
    uint16_t count = RxBuffer_Count();
    if (count == 0U || byte == NULL) {
        return false;
    }

    *byte = g_rx_buffer[g_rx_tail];
    g_rx_tail = (uint16_t)((g_rx_tail + 1U) % UART3_RX_BUFFER_SIZE);
    return true;
}

static void K230_UART3_SendByte(uint8_t byte)
{
    while (DL_UART_Main_isTXFIFOFull(UART3)) {
        /* Wait until UART3 TX FIFO has space. */
    }

    DL_UART_Main_transmitData(UART3, byte);
}

static void K230_UART3_SendBuffer(const uint8_t *data, uint16_t length)
{
    uint16_t i;
    for (i = 0U; i < length; i++) {
        K230_UART3_SendByte(data[i]);
    }
}

static void K230_UART3_SendPong(uint32_t sequence)
{
    char reply[24];
    int len;

    len = sprintf(reply, "PONG,%lu\r\n", (unsigned long) sequence);
    if (len > 0) {
        K230_UART3_SendBuffer((const uint8_t *) reply, (uint16_t) len);
        g_k230_tx_pong_count++;
    }
}

static void K230_UART3_HandleParseError(void)
{
    g_k230_parse_error_count++;
    g_k230_rx_state = K230_RX_STATE_WAIT_P;
    g_k230_rx_sequence = 0U;
    g_k230_rx_digit_count = 0U;
}

static void K230_UART3_HandleValidPing(void)
{
    g_k230_valid_ping_count++;
    g_k230_last_sequence = g_k230_rx_sequence;
    K230_UART3_SendPong(g_k230_rx_sequence);
    g_k230_rx_state = K230_RX_STATE_WAIT_P;
    g_k230_rx_sequence = 0U;
    g_k230_rx_digit_count = 0U;
}

static void K230_UART3_ParseByte(uint8_t data)
{
    switch (g_k230_rx_state) {
        case K230_RX_STATE_WAIT_P:
            if (data == 'P') {
                g_k230_rx_state = K230_RX_STATE_WAIT_I;
            }
            break;

        case K230_RX_STATE_WAIT_I:
            if (data == 'I') {
                g_k230_rx_state = K230_RX_STATE_WAIT_N;
            } else if (data == 'P') {
                g_k230_rx_state = K230_RX_STATE_WAIT_I;
            } else {
                K230_UART3_HandleParseError();
            }
            break;

        case K230_RX_STATE_WAIT_N:
            if (data == 'N') {
                g_k230_rx_state = K230_RX_STATE_WAIT_G;
            } else if (data == 'P') {
                g_k230_rx_state = K230_RX_STATE_WAIT_I;
            } else {
                K230_UART3_HandleParseError();
            }
            break;

        case K230_RX_STATE_WAIT_G:
            if (data == 'G') {
                g_k230_rx_state = K230_RX_STATE_WAIT_COMMA;
            } else if (data == 'P') {
                g_k230_rx_state = K230_RX_STATE_WAIT_I;
            } else {
                K230_UART3_HandleParseError();
            }
            break;

        case K230_RX_STATE_WAIT_COMMA:
            if (data == ',') {
                g_k230_rx_sequence = 0U;
                g_k230_rx_digit_count = 0U;
                g_k230_rx_state = K230_RX_STATE_READ_SEQ;
            } else if (data == 'P') {
                g_k230_rx_state = K230_RX_STATE_WAIT_I;
            } else {
                K230_UART3_HandleParseError();
            }
            break;

        case K230_RX_STATE_READ_SEQ:
            if ((data >= '0') && (data <= '9')) {
                if (g_k230_rx_digit_count < K230_UART3_MAX_SEQUENCE_DIGITS) {
                    g_k230_rx_sequence = g_k230_rx_sequence * 10U + (uint32_t)(data - '0');
                    g_k230_rx_digit_count++;
                } else {
                    K230_UART3_HandleParseError();
                }
            } else if (data == '\r') {
                if (g_k230_rx_digit_count > 0U) {
                    g_k230_rx_state = K230_RX_STATE_WAIT_LF;
                } else {
                    K230_UART3_HandleParseError();
                }
            } else {
                K230_UART3_HandleParseError();
            }
            break;

        case K230_RX_STATE_WAIT_LF:
            if (data == '\n') {
                K230_UART3_HandleValidPing();
            } else {
                K230_UART3_HandleParseError();
            }
            break;

        default:
            K230_UART3_HandleParseError();
            break;
    }
}

void K230_UART3_Process(void)
{
    uint8_t byte;

    while (K230_UART3_ReadByte(&byte)) {
        K230_UART3_ParseByte(byte);
    }
}

void K230_UART3_Init(void)
{
    DL_UART_Main_reset(UART3);

    /* Enable GPIOB and UART3 power. */
    DL_GPIO_enablePower(GPIOB);
    DL_UART_Main_enablePower(UART3);

    /* Configure PB2/PB3 to UART3 TX/RX without touching other PB pins. */
    DL_GPIO_initPeripheralOutputFunction(IOMUX_PINCM15, IOMUX_PINCM15_PF_UART3_TX);
    DL_GPIO_initPeripheralInputFunction(IOMUX_PINCM16, IOMUX_PINCM16_PF_UART3_RX);

    /* UART3 uses the 32 MHz bus clock, matching the baud-rate calculation. */
    DL_UART_Main_setClockConfig(
        UART3, (DL_UART_Main_ClockConfig *) &g_uart3_clock_config);
    DL_UART_Main_init(UART3, (DL_UART_Main_Config *) &g_uart3_config);

    DL_UART_Main_configBaudRate(UART3, 32000000U, 115200U);

    DL_UART_Main_enableInterrupt(UART3, DL_UART_MAIN_INTERRUPT_RX);
    DL_UART_Main_enable(UART3);

    NVIC_ClearPendingIRQ(UART3_INT_IRQn);
    NVIC_EnableIRQ(UART3_INT_IRQn);
}

void UART3_IRQHandler(void)
{
    if (DL_UART_Main_getPendingInterrupt(UART3) == DL_UART_MAIN_IIDX_RX) {
        uint8_t data = (uint8_t) DL_UART_Main_receiveData(UART3);
        uint16_t next_head = (uint16_t)((g_rx_head + 1U) % UART3_RX_BUFFER_SIZE);
        if (next_head != g_rx_tail) {
            g_rx_buffer[g_rx_head] = data;
            g_rx_head = next_head;
            g_k230_rx_byte_count++;
        } else {
            g_k230_rx_overflow_count++;
        }
    }
}

