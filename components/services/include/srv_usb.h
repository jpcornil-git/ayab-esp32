#ifndef _SRV_USB_H_
#define _SRV_USB_H_

#include "esp_err.h"
#include "freertos/FreeRTOS.h"

typedef BaseType_t (*usb_rx_cb_t)(const uint8_t *payload, size_t len);

esp_err_t srv_usb_init(usb_rx_cb_t rx_callback);
esp_err_t srv_usb_send_bin(uint8_t *buffer, uint32_t buffer_length);

#endif
