#ifndef _RA4M1_SAMBA_H_
#define _RA4M1_SAMBA_H_

#include "driver/uart.h"

/**
 * @brief Initialize the RA4M1 SAM-BA interface.
 *
 * This function initializes the SAM-BA interface for communication with the RA4M1.
 *
 * @param uartPort The UART port to use for SAM-BA communication.
 * @param baud_rate The baud rate for the UART communication.
 */
void ra4m1_samba_init(uart_port_t uartPort, int baud_rate);

/**
 * @brief Connect to the RA4M1 SAM-BA interface.
 *
 * This function establishes a connection to the RA4M1 SAM-BA interface.
 * It should be called before any other SAM-BA operations.
 */
void ra4m1_samba_connect();

/**
 * @brief Erase a section of the RA4M1 SAM-BA memory.
 *
 * This function erases a specified section of the RA4M1 SAM-BA memory.
 *
 * @param start_addr The starting address of the section to erase.
 * @return ESP_OK on success, or an error code on failure.
 */
esp_err_t ra4m1_samba_erase(unsigned int start_addr);

/**
 * @brief Load a buffer into the RA4M1 SAM-BA interface.
 *
 * This function loads a buffer into the RA4M1 SAM-BA interface for writing.
 *
 * @param buffer Pointer to the data buffer to load.
 * @param size Size of the data buffer in bytes.
 * @return ESP_OK on success, or an error code on failure.
 */
esp_err_t ra4m1_samba_load_buffer(const uint8_t* buffer, uint32_t size);

/**
 * @brief Write a buffer to the RA4M1 SAM-BA memory.
 *
 * This function writes a buffer to the RA4M1 SAM-BA memory at a specified address.
 *
 * @param dst_addr The destination address in the RA4M1 SAM-BA memory.
 * @param size The size of the data to write in bytes.
 * @return ESP_OK on success, or an error code on failure.
 */
esp_err_t ra4m1_samba_write_buffer(uint32_t dst_addr, uint32_t size);

/**
 * @brief Disconnect from the RA4M1 SAM-BA interface.
 *
 * This function disconnects from the RA4M1 SAM-BA interface and cleans up resources.
 */
void ra4m1_samba_disconnect();

/**
 * @brief Get the size of the buffer used for writing in the RA4M1 SAM-BA interface.
 *
 * This function returns the size of the buffer used for writing data to the RA4M1 SAM-BA memory.
 *
 * @return The size of the write buffer in bytes.
 */
uint32_t ra4m1_samba_write_bufferSize();

#endif