#ifndef _APP_DEFINE_H_
#define _APP_DEFINE_H_

#include "driver/gpio.h"

// RA4M1 Interface
#define RA4M1_PIN_BOOT  GPIO_NUM_9
#define RA4M1_PIN_RESET GPIO_NUM_4

#define RA4M1_UART UART_NUM_0
#define RA4M1_UART_BAUDRATE 115200
#define RA4M1_UART_TX_PIN GPIO_NUM_43
#define RA4M1_UART_RX_PIN GPIO_NUM_44

#define RA4M1_SAMBA_BAUDRATE 230400

// LITTLEFS
#define LITTLEFS_BASE_PATH "/littlefs"
#define DEFAULT_FIRMWARE "firmware.bin"
#define FIRST_BOOT (LITTLEFS_BASE_PATH "/" "firstBoot")

// NVS
#define NVS_APP_PARTITION "ayab"
#define NVS_WIFI_SSID "ssid"
#define NVS_WIFI_PASSWORD "password"
#define NVS_HOSTNAME "hostname"

// MDSN
#define APP_MDNS_SERVICE_TYPE "_ayab"

// APP events
#define WIFI_AP_CONNECTED_EVENT     BIT0
#define WIFI_STA_CONNECTED_EVENT    BIT1
#define WIFI_STA_CANT_CONNECT_EVENT BIT2
#define RA4M1_FLASH                 BIT3
#define RA4M1_UART_RX               BIT4
#define RA4M1_UART_TX               BIT5
#define APP_QUEUE_UART_TX_SIZE      16

// App definitions
#define BOARD_ID "UnoR4"
#define API_VERSION "0.1"

#endif