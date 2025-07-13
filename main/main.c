#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

#include "esp_log.h"
#include "esp_mac.h"
#include "esp_wifi.h"
#include "driver/uart.h"

#include "app_define.h"
#include "app_config.h"
#include "ra4m1_ctrl.h"
#include "ra4m1_uart.h"
#include "ra4m1_samba.h"
#include "ra4m1_flash.h"
#include "srv_spiffs.h"
#include "srv_http.h"
#include "srv_mdns.h"
#include "srv_websocket.h"
#include "srv_wifi.h"

static const char *TAG = "main";

// Main application event queue
static EventGroupHandle_t app_event_group;

// Message queue for messages from WebSocket to UART
static QueueHandle_t app_queue_uart_tx;

// Global variables for configuration parameters
static char wifiSSID[APP_CONFIG_VALUE_SIZE] = {0};
static char wifiPassword[APP_CONFIG_VALUE_SIZE] = {0};
static char hostName[APP_CONFIG_VALUE_SIZE] = {0};

app_config_t  defaultConfig[] = {
    {.key = NVS_WIFI_SSID    , .value = wifiSSID    , .size = APP_CONFIG_VALUE_SIZE, .default_value = CONFIG_DEFAULT_WIFI_SSID},
    {.key = NVS_WIFI_PASSWORD, .value = wifiPassword, .size = APP_CONFIG_VALUE_SIZE, .default_value = CONFIG_DEFAULT_WIFI_PASSWORD},
    {.key = NVS_HOSTNAME     , .value = hostName    , .size = APP_CONFIG_VALUE_SIZE, .default_value = CONFIG_DEFAULT_AYAB_HOSTNAME},                
};

// MDNS capabilities for webapp service
static mdns_txt_item_t serviceTxtData[] = {
    {.key = "path"    , .value = SRV_HTTP_PATH_WS},
    {.key = "board_id", .value = BOARD_ID},
    {.key = "api_ver" , .value = API_VERSION}
};

// Called from a different task context -> use a queue to forward data to ra4m1 uart
BaseType_t ws_rx_bin_callback(const uint8_t *payload, size_t len) {
    BaseType_t rv= pdFALSE;
    uart_msg_t msg;

    msg.len = len;
    msg.payload = (uint8_t *)malloc(len * sizeof(uint8_t));
    if(msg.payload != NULL) {
        memcpy(msg.payload, payload, len);
        rv = xQueueSendToBack(app_queue_uart_tx, &msg, 0);
        if (rv != pdTRUE) {
            free(msg.payload); // Failed to send message, free payload
            ESP_LOGE(TAG, "Failed to send message to UART queue");
        } else {
            xEventGroupSetBits(app_event_group, RA4M1_UART_TX);
        }
    } else {
        ESP_LOGE(TAG, "Failed to allocate memory for UART message payload");
    }
    return rv;
}

void app_setup() {
    // Create the default event loop
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // Create an event group to collect events from various tasks
    app_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(app_event_group != NULL ? ESP_OK : ESP_FAIL);

    // Create a queue to handle messages from httpd/ws task to uart
    app_queue_uart_tx = xQueueCreate(APP_QUEUE_UART_TX_SIZE, sizeof(uart_msg_t) );
    ESP_ERROR_CHECK(app_queue_uart_tx != 0 ? ESP_OK : ESP_FAIL);

    // Setup RA4M1 Interfaces
    ra4m1_ctrl_init(RA4M1_PIN_RESET, RA4M1_PIN_BOOT);
    ra4m1_uart_init(RA4M1_UART, RA4M1_UART_BAUDRATE, RA4M1_UART_TX_PIN, RA4M1_UART_RX_PIN, app_event_group, RA4M1_UART_RX);
    ra4m1_samba_init(RA4M1_UART, RA4M1_SAMBA_BAUDRATE);

    // Initialize NVS
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // NVS partition was truncated and needs to be erased
        ESP_ERROR_CHECK(nvs_flash_erase());
        // Retry nvs_flash_init
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    // Initialize config from NVS
    app_config_init(NVS_APP_PARTITION, defaultConfig, sizeof(defaultConfig) / sizeof(app_config_t));
    if (app_config_load() != ESP_OK) {
        app_config_reset();
        app_config_save();
        ESP_LOGI(TAG, "NVS partition (" NVS_APP_PARTITION ") initialized");
    }; 

    // Initialise SPIFFS file system
    srv_spiffs_start(SPIFFS_BASE_PATH);

    // Initialize the networking stack
    ESP_ERROR_CHECK(esp_netif_init());

    // Try to update RA4M1 firmware with a new flash image
    struct stat st;
    if (stat(FIRST_BOOT, &st) == 0) {
        ESP_LOGI(TAG, "First boot ... updating RA4M1 firmware");
        // Update firmware and release reset pin      
        ra4m1_flash_image(SPIFFS_BASE_PATH "/" DEFAULT_FIRMWARE);
        // Remove file even if above fails, i.e. only try once
        unlink(FIRST_BOOT);
    } else {
        ra4m1_ctrl_restart();
    }
}

void app_main() {   
    ESP_LOGI(TAG, "ayab-webapp starting");

    app_setup();

    // Start WiFi STA mode as default (fallback to AP mode after 3 failures)
    srv_wifi_start_STA(app_event_group,
        app_config_get(NVS_WIFI_SSID),
        app_config_get(NVS_WIFI_PASSWORD)
    );

    while(true) {
        EventBits_t event_bits = xEventGroupWaitBits(app_event_group,
                                             WIFI_AP_CONNECTED_EVENT | WIFI_STA_CONNECTED_EVENT | WIFI_STA_CANT_CONNECT_EVENT | RA4M1_UART_RX | RA4M1_UART_TX,
                                             pdTRUE, // xClearOnExit
                                             pdFALSE, // xWaitForAllBits
                                             portMAX_DELAY); // xTicksToWait

        if ((event_bits & WIFI_AP_CONNECTED_EVENT) || (event_bits & WIFI_STA_CONNECTED_EVENT)) {
            ESP_LOGI(TAG, "Restarting http & mDNS servers");
            // Start the mDNS server
            wifi_interface_t wifi_interface = (event_bits & WIFI_AP_CONNECTED_EVENT) ? WIFI_IF_AP : WIFI_IF_STA;
            srv_mdns_start(app_config_get(NVS_HOSTNAME), wifi_interface, APP_MDNS_SERVICE_TYPE, serviceTxtData, sizeof(serviceTxtData) / sizeof(mdns_txt_item_t));
            // Start the http server    
            srv_http_start(SPIFFS_BASE_PATH, ws_rx_bin_callback);
        } else if (event_bits & WIFI_STA_CANT_CONNECT_EVENT) {
            ESP_LOGI(TAG, "Unable to connect as STA, switching to softAP mode");
            srv_http_stop();
            srv_mdns_stop();
            srv_wifi_stop();
            srv_wifi_start_AP(app_event_group);    
        } 
        
        // Websocket - Serial communication bridge
        if (event_bits & RA4M1_UART_RX) {
            // UART (RA4M1) rx => WS tx
            uart_msg_t message;
            ra4m1_uart_rx(&message);
            if (message.payload != NULL) {
                srv_websocket_send_bin(message.payload, message.len);
                free(message.payload);
            }
        }

        if (event_bits & RA4M1_UART_TX) {
            // WS rx => UART (RA4M1) tx
            uart_msg_t message;
            if (xQueueReceive(app_queue_uart_tx, &message, 0) == pdTRUE) {
                ra4m1_uart_tx(&message);
                free(message.payload);
            }
        }
    }
}

