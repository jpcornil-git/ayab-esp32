#include "srv_usb.h"
#include "tinyusb.h"
#include "tinyusb_cdc_acm.h"
#include "tusb.h"
#include "tinyusb_default_config.h"

#include "tinyusb_console.h"

#include "esp_log.h"

static const char *TAG = "srv_usb";
static usb_rx_cb_t _rx_callback = NULL;

static void tinyusb_cdc_rx_callback(int itf, cdcacm_event_t *event) {
    if (event->type == CDC_EVENT_RX) {
        size_t rx_size = 0;
        uint8_t buf[CONFIG_TINYUSB_CDC_RX_BUFSIZE];
        esp_err_t ret = tinyusb_cdcacm_read(itf, buf, CONFIG_TINYUSB_CDC_RX_BUFSIZE, &rx_size);
        if (ret == ESP_OK && rx_size > 0 && _rx_callback) {
            _rx_callback(buf, rx_size);
        }
    }
}

esp_err_t srv_usb_init(usb_rx_cb_t rx_callback) {
    _rx_callback = rx_callback;

    const tinyusb_config_t tusb_cfg = TINYUSB_DEFAULT_CONFIG();
    ESP_ERROR_CHECK(tinyusb_driver_install(&tusb_cfg));

    // Initialize USB CDC ACM Port 0 for the Ayab Data Bridge
    tinyusb_config_cdcacm_t ayab_cfg = { 0 };
    ayab_cfg.cdc_port = TINYUSB_CDC_ACM_0;
    ayab_cfg.callback_rx = &tinyusb_cdc_rx_callback;
    ESP_ERROR_CHECK(tinyusb_cdcacm_init(&ayab_cfg));

    // Initialize USB CDC ACM Port 1 for the System Console
    tinyusb_config_cdcacm_t log_cfg = { 0 };
    log_cfg.cdc_port = TINYUSB_CDC_ACM_1;
    ESP_ERROR_CHECK(tinyusb_cdcacm_init(&log_cfg));

    // Redirect console output (stdout/stderr) to USB CDC ACM Port 1
    return tinyusb_console_init(TINYUSB_CDC_ACM_1);
}


esp_err_t srv_usb_send_bin(uint8_t *buffer, uint32_t buffer_length) {
    if (!tud_cdc_n_connected(TINYUSB_CDC_ACM_0)) {
        return ESP_ERR_INVALID_STATE;
    }
    size_t queued = tinyusb_cdcacm_write_queue(TINYUSB_CDC_ACM_0, buffer, buffer_length);
    if (queued < buffer_length) {
        ESP_LOGW(TAG, "USB CDC ACM: Only %zu of %u bytes queued", queued, (unsigned int)buffer_length);
    }
    return tinyusb_cdcacm_write_flush(TINYUSB_CDC_ACM_0, 0);
}
