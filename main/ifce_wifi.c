#include "esp_log.h"
#include "esp_mac.h"
#include "lwip/inet.h"
#include "esp_wifi.h"

#include "app_define.h"
#include "ifce_wifi.h"


static const char *TAG = "ifce_wifi";

static esp_netif_t *wifi_netif_handle = NULL;

#define MAX_STA_CONNECT_RETRIES 3
static int sta_connect_retries = 0;

static EventGroupHandle_t *wifi_event_group; 

// Wifi event handler
static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data) {
    // Wifi events
    if (event_base == WIFI_EVENT) {
        // STA Mode events
        if (event_id == WIFI_EVENT_STA_START) {
            sta_connect_retries = 0;
            esp_wifi_connect();
            ESP_LOGI(TAG, "Starting STA mode.");
        } else if (event_id == WIFI_EVENT_STA_CONNECTED) {       
            wifi_event_sta_connected_t *event = (wifi_event_sta_connected_t *)event_data;
            xEventGroupSetBits(*wifi_event_group, WIFI_STA_CONNECTED_EVENT); 
            ESP_LOGI(TAG, "Connected to AP (ssid=%s, channel=%d)", event->ssid, event->channel);           
        } else if (event_id == WIFI_EVENT_STA_DISCONNECTED) {       
            if (sta_connect_retries < MAX_STA_CONNECT_RETRIES) {
                sta_connect_retries++;
                esp_wifi_connect();
                ESP_LOGI(TAG, "Disconnected from AP, trying to reconnect ... (attempt %d/%d).", sta_connect_retries, MAX_STA_CONNECT_RETRIES);
            } else {
                xEventGroupSetBits(*wifi_event_group, WIFI_STA_CANT_CONNECT_EVENT);
                ESP_LOGI(TAG, "Failed to reconnect to AP after %d attempts.", MAX_STA_CONNECT_RETRIES);
            }
        // AP Mode events
        } else if (event_id == WIFI_EVENT_AP_START) {
            xEventGroupSetBits(*wifi_event_group, WIFI_AP_CONNECTED_EVENT);
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
            sta_connect_retries = 0;
            xEventGroupSetBits(*wifi_event_group, WIFI_STA_CONNECTED_EVENT);
            ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
            ESP_LOGI(TAG, "Connected to AP with IP = " IPSTR, IP2STR(&event->ip_info.ip));
        }
    }
}

void ifce_wifi_start_STA(EventGroupHandle_t *event_group, const char* ssid, const char* password)  {
    wifi_event_group = event_group;

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));

    // Initialize Wi-Fi including netif with default STA config
    wifi_netif_handle = esp_netif_create_default_wifi_sta();
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA)); 

    wifi_config_t wifi_config = {0};
    strncpy((char *)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid)-1);
    strncpy((char *)wifi_config.sta.password, password, sizeof(wifi_config.sta.password)-1);
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));

    ESP_ERROR_CHECK(esp_wifi_start());
}

void ifce_wifi_start_AP(EventGroupHandle_t *event_group) {
    wifi_event_group = event_group;

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));

    // Initialize Wi-Fi including netif with default AP config
    wifi_netif_handle = esp_netif_create_default_wifi_ap();
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

    char ip_addr[16];
    esp_netif_ip_info_t ip_info;
    esp_netif_get_ip_info(wifi_netif_handle, &ip_info);
    inet_ntoa_r(ip_info.ip.addr, ip_addr, 16);
    ESP_LOGI(TAG, "softAP on IP: %s", ip_addr);
}

void ifce_wifi_stop(void) {
    esp_err_t err = esp_wifi_stop();
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "esp_wifi_stop failed");
        return;
    }
    ESP_ERROR_CHECK(esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler));
    esp_wifi_deinit();
    esp_netif_destroy(wifi_netif_handle);
}