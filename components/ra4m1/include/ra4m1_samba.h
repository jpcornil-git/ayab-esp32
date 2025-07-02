#ifndef _RA4M1_SAMBA_H_
#define _RA4M1_SAMBA_H_

#include "driver/uart.h"

void ra4m1_samba_init(uart_port_t uartPort, int baud_rate);
void ra4m1_samba_connect();
esp_err_t ra4m1_samba_erase(unsigned int start_addr);
esp_err_t ra4m1_samba_load_buffer(const uint8_t* buffer, uint32_t size);
esp_err_t ra4m1_samba_write_buffer(uint32_t dst_addr, uint32_t size);
void ra4m1_samba_disconnect();

uint32_t ra4m1_samba_write_bufferSize();

#endif