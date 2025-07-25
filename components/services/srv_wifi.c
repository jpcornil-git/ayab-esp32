#include "esp_log.h"
#include "esp_mac.h"
#include "lwip/inet.h"
#include "esp_wifi.h"

#include "app_define.h"
#include "srv_wifi.h"

#define MAX_STA_CONNECT_RETRIES 3

typedef struct {
    EventGroupHandle_t event_group; /**< Event group for WiFi events */
    esp_netif_t *netif_handle;      /**< Network interface handle for WiFi */
    int sta_connect_retries;        /**< Number of connection retries for STA mode */
    bool initialized;               /**< Flag indicating if WiFi is initialized */
} srv_wifi_data_t;

static const char *TAG = "srv_wifi";

static srv_wifi_data_t _self = {
    .netif_handle = NULL,
    .sta_connect_retries = 0,
    .initialized = false,
};

// Wifi event handler
static void _wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data) {
    // Wifi events
    if (event_base == WIFI_EVENT) {
        // STA Mode events
        if (event_id == WIFI_EVENT_STA_START) {
            _self.sta_connect_retries = 0;
            esp_wifi_connect();
            ESP_LOGI(TAG, "Starting STA mode.");
        } else if (event_id == WIFI_EVENT_STA_CONNECTED) {       
            wifi_event_sta_connected_t *event = (wifi_event_sta_connected_t *)event_data;
            xEventGroupSetBits(_self.event_group, WIFI_STA_CONNECTED_EVENT); 
            ESP_LOGI(TAG, "Connected to AP (ssid=%s, channel=%d)", event->ssid, event->channel);           
        } else if (event_id == WIFI_EVENT_STA_DISCONNECTED) {       
            if (_self.sta_connect_retries < MAX_STA_CONNECT_RETRIES) {
                _self.sta_connect_retries++;
                esp_wifi_connect();
                ESP_LOGI(TAG, "Disconnected from AP, trying to reconnect ... (attempt %d/%d).", _self.sta_connect_retries, MAX_STA_CONNECT_RETRIES);
            } else {
                xEventGroupSetBits(_self.event_group, WIFI_STA_CANT_CONNECT_EVENT);
                ESP_LOGI(TAG, "Failed to reconnect to AP after %d attempts.", MAX_STA_CONNECT_RETRIES);
            }
        // AP Mode events
        } else if (event_id == WIFI_EVENT_AP_START) {
            xEventGroupSetBits(_self.event_group, WIFI_AP_CONNECTED_EVENT);
            ESP_LOGI(TAG, "Starting AP mode.");
        } else if (event_id == WIFI_EVENT_AP_STACONNECTED) {
            wifi_event_ap_staconnected_t *event = (wifi_event_ap_staconnected_t *)event_data;
            ESP_LOGI(TAG, "Station " MACSTR " join, AID=%d",
                    MAC2STR(event->mac), event->aid);
        } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
            wifi_event_ap_stadisconnected_t *event = (wifi_event_ap_stadisconnected_t *)event_data;
            ESP_LOGI(TAG, "Station " MACSTR " leave, AID=%d, reason=%d",
                    MAC2STR(event->mac), event->aid, event->reason);
        } 
    // IP events
    } else if (event_base == IP_EVENT) {
        if (event_id == IP_EVENT_STA_GOT_IP) {
            _self.sta_connect_retries = 0;
            xEventGroupSetBits(_self.event_group, WIFI_STA_CONNECTED_EVENT);
            ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
            ESP_LOGI(TAG, "Connected to AP with IP = " IPSTR, IP2STR(&event->ip_info.ip));
        }
    }
}

void _wifi_start_common(EventGroupHandle_t event_group)  {
    _self.event_group = event_group;

    if (!_self.initialized) {
        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
        ESP_ERROR_CHECK(esp_wifi_init(&cfg));
        _self.initialized = true;
    }

    // Unregister the event handler first to avoid duplicate handlers if switching modes
    esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, &_wifi_event_handler);
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &_wifi_event_handler, NULL));
}

void srv_wifi_start_STA(EventGroupHandle_t event_group, const char* ssid, const char* password)  {
    _wifi_start_common(event_group);

    // Initialize Wi-Fi including netif with default STA config
    _self.netif_handle = esp_netif_create_default_wifi_sta();
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA)); 

    wifi_config_t wifi_config = {0};
    strncpy((char *)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid) - 1);
    wifi_config.sta.ssid[sizeof(wifi_config.sta.ssid) - 1] = '\0';
    strncpy((char *)wifi_config.sta.password, password, sizeof(wifi_config.sta.password) - 1);
    wifi_config.sta.password[sizeof(wifi_config.sta.password) - 1] = '\0';
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));

    ESP_ERROR_CHECK(esp_wifi_start());
}

void srv_wifi_start_AP(EventGroupHandle_t event_group) {
    _wifi_start_common(event_group);

    // Initialize Wi-Fi including netif with default AP config
    _self.netif_handle = esp_netif_create_default_wifi_ap();
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = CONFIG_DEFAULT_WIFI_SSID,
            .max_connection = 4,
            .authmode = WIFI_AUTH_OPEN,
        },
    };  
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));

    ESP_ERROR_CHECK(esp_wifi_start());

    char ip_addr[INET_ADDRSTRLEN];
    esp_netif_ip_info_t ip_info;
    esp_netif_get_ip_info(_self.netif_handle, &ip_info);
    inet_ntoa_r(ip_info.ip.addr, ip_addr, INET_ADDRSTRLEN);
    ESP_LOGI(TAG, "softAP on IP: %s", ip_addr);
}

void srv_wifi_stop(void) {
    esp_err_t err = esp_wifi_stop();
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "esp_wifi_stop failed, continuing cleanup anyway");
    }
    // Always try to unregister the event handler and deinit WiFi
    ESP_ERROR_CHECK(esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, &_wifi_event_handler));
    esp_wifi_deinit();
    _self.initialized = false;
    if (_self.netif_handle != NULL) {
        esp_netif_destroy(_self.netif_handle);
        _self.netif_handle = NULL;
    }
}