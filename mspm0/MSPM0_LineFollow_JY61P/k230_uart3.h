#ifndef K230_UART3_H
#define K230_UART3_H

#include <stdbool.h>
#include <stdint.h>

void K230_UART3_Init(void);
bool K230_UART3_Available(void);
uint16_t K230_UART3_GetAvailableCount(void);
bool K230_UART3_ReadByte(uint8_t *byte);
void K230_UART3_Process(void);

extern volatile uint32_t g_k230_rx_byte_count;
extern volatile uint32_t g_k230_valid_ping_count;
extern volatile uint32_t g_k230_tx_pong_count;
extern volatile uint32_t g_k230_parse_error_count;
extern volatile uint32_t g_k230_rx_overflow_count;
extern volatile uint32_t g_k230_last_sequence;

#endif /* K230_UART3_H */

