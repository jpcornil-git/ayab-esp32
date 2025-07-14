#ifndef _RA4M1_UART_H_
#define _RA4M1_UART_H_

#include <stddef.h>
#include <stdint.h>

typedef struct uart_tx_message {
    size_t len;
    uint8_t *payload;
} uart_msg_t;

/**
 * @brief Initialize the RA4M1 UART interface.
 *
 * This function initializes the UART interface for communication with the RA4M1.
 *
 * @param uart_num The UART port number to use.
 * @param baud_rate The baud rate for the UART communication.
 * @param tx_pin The GPIO pin number for UART transmission.
 * @param rx_pin The GPIO pin number for UART reception.
 * @param event_group Event group handle for synchronization.
 * @param event_rx Event bit to signal reception events.
 */
void ra4m1_uart_init(int uart_num, int baud_rate, int tx_pin, int rx_pin, EventGroupHandle_t event_group, EventBits_t event_rx);

/**
 * @brief Receive data from the RA4M1 UART interface.
 *
 * This function reads data from the UART interface and stores it in the provided message structure.
 *
 * @param message Pointer to the uart_msg_t structure to store received data.
 */
void ra4m1_uart_rx(uart_msg_t *message);

/**
 * @brief Transmit data over the RA4M1 UART interface.
 *
 * This function sends data through the UART interface.
 *
 * @param message Pointer to the uart_msg_t structure containing data to send.
 * @return The number of bytes transmitted, or a negative value on error.
 */
int ra4m1_uart_tx(uart_msg_t *message);

#endif