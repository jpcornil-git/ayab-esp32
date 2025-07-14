#include <stdint.h>
#include <string.h>
#include <sys/param.h>

#include "esp_log.h"

#include "ra4m1_ctrl.h"
#include "ra4m1_samba.h"

// Timeout definitions for serial read
#define TIMEOUT_NORMAL  (1000 / portTICK_PERIOD_MS)
#define TIMEOUT_LONG    (5000 / portTICK_PERIOD_MS)

// Bootloader buffer offser (no applet -> 0)
#define BUFFER_START_ADDRESS ((long unsigned int) 0)

typedef struct {
    uint32_t baud_rate;  // Baud rate for the UART connection
    uint32_t saved_baud_rate; // Saved baud rate before connecting to samba
    uart_port_t uartPort; // UART port used for samba communication
} ra4m1_samba_data_t;

static const char *TAG = "ra4m1_samba";

static ra4m1_samba_data_t _self;

uint32_t ra4m1_samba_write_bufferSize() { return 4096; }

void ra4m1_samba_init(uart_port_t uartPort, int baud_rate) {
        _self.uartPort = uartPort;
        _self.baud_rate = baud_rate;
}

void ra4m1_samba_connect() {
    char cmd[256];
    size_t num;

    // Reset RA4M1 and set program mode
    ra4m1_ctrl_enter_programming();
    vTaskDelay(100 / portTICK_PERIOD_MS); // Wait until samba is ready, 100ms ?

    ESP_ERROR_CHECK(uart_get_baudrate(_self.uartPort, &_self.saved_baud_rate));
    ESP_ERROR_CHECK(uart_set_baudrate(_self.uartPort, _self.baud_rate));
    ESP_ERROR_CHECK(uart_flush(_self.uartPort));

    // Set binary mode (no '>' prompt)
    uart_write_bytes(_self.uartPort, "N#", 2);
    uart_read_bytes(_self.uartPort, cmd, sizeof(cmd), TIMEOUT_NORMAL); // Expects "\n\r"

    // Get/log ChipId ("nRF52840-QIAA")
    // FIXME: Check return value ?
    uart_write_bytes(_self.uartPort, "I#", 2);
    num = uart_read_bytes(_self.uartPort, cmd, sizeof(cmd), TIMEOUT_NORMAL);
    cmd[num-2] = '\0'; // remove "\n\r" (-2)
    ESP_LOGI(TAG, "ChipId (%d): %s", num, cmd);

    // Get/log bootloader version ("Arduino Bootloader (SAM-BA extended) 2.0 [Arduino:IKXYZ]")
    uart_write_bytes(_self.uartPort, "V#", 2);
    num = uart_read_bytes(_self.uartPort, cmd, sizeof(cmd), TIMEOUT_NORMAL);
    cmd[num-2] = '\0'; // remove "\n\r" (-2)
    ESP_LOGI(TAG, "Bootloader (%d): %s", num, cmd);
}

esp_err_t ra4m1_samba_erase(unsigned int start_addr) {
    uint8_t cmd[64];
 
    // Erase flash from start_addr
    int l = snprintf((char*) cmd, sizeof(cmd), "X%08X#", start_addr);
    if (uart_write_bytes(_self.uartPort, cmd, l) != l)
        return ESP_FAIL;
    uart_read_bytes(_self.uartPort, cmd, 3, TIMEOUT_LONG); // Expects "X\n\r"
    if (cmd[0] != 'X')
        return ESP_FAIL;

    return ESP_OK;
}

esp_err_t ra4m1_samba_load_buffer(const uint8_t* buffer, uint32_t size) {
    uint8_t cmd[20];

    // Store the data to be written to the flash in the buffer memory
    snprintf((char*) cmd, sizeof(cmd), "S%08lX,%08lX#", BUFFER_START_ADDRESS, size);
    if (uart_write_bytes(_self.uartPort, cmd, sizeof(cmd) - 1) != sizeof(cmd) - 1)
        return ESP_FAIL;

    while (size) {
        int written = uart_write_bytes(_self.uartPort, buffer, size);
        if (written == 0)
            return ESP_FAIL;
        buffer += written;  
        size -= written;
    }

    return ESP_OK;
}

esp_err_t ra4m1_samba_write_buffer(uint32_t dst_addr, uint32_t size) {

    // Set the start address of the buffer to be copied into flash
    uint8_t cmd[20];
    int l = snprintf((char*) cmd, sizeof(cmd), "Y%08lX,0#", BUFFER_START_ADDRESS);
    if (uart_write_bytes(_self.uartPort, cmd, l) != l)
        return ESP_FAIL;

    cmd[0] = 0;
    uart_read_bytes(_self.uartPort, cmd, 3, TIMEOUT_NORMAL); // Expects "Y\n\r"
    if (cmd[0] != 'Y')
        return ESP_FAIL;

    // Write buffer to flash at dst_addr address
    l = snprintf((char*) cmd, sizeof(cmd), "Y%08lX,%08lX#", dst_addr, size);
    if (uart_write_bytes(_self.uartPort, cmd, l) != l)
        return ESP_FAIL;

    cmd[0] = 0;
    uart_read_bytes(_self.uartPort, cmd, 3, TIMEOUT_LONG); // Expects "Y\n\r"
    if (cmd[0] != 'Y')
        return ESP_FAIL;

    return ESP_OK;
}

void ra4m1_samba_disconnect() {
    ra4m1_ctrl_exit_programming();
    ESP_ERROR_CHECK(uart_set_baudrate(_self.uartPort, _self.saved_baud_rate));
    ESP_ERROR_CHECK(uart_flush(_self.uartPort));
}