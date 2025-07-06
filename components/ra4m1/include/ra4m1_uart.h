#ifndef _RA4M1_UART_H_
#define _RA4M1_UART_H_

#include <stddef.h>
#include <stdint.h>

typedef struct uart_tx_message {
    size_t len;
    uint8_t *payload;
} uart_msg_t;

void ra4m1_uart_init(int uart_num, int baud_rate, int tx_pin, int rx_pin, EventGroupHandle_t event_group, EventBits_t event_rx);
void ra4m1_uart_rx(uart_msg_t *message);
int ra4m1_uart_tx(uart_msg_t *message);

#endif