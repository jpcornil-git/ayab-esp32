#include <string.h>
#include "esp_log.h"

#include "esp_mac.h"

#include "app_config.h"
#include "srv_mdns.h"

static const char *TAG = "srv_mdns";

void srv_mdns_start(const char *hostname, wifi_interface_t wifi_ifce, const char *service_type, mdns_txt_item_t *serviceTxtData, size_t size) {
    char instance_name[APP_CONFIG_VALUE_SIZE + 6*3 + 1];
    uint8_t mac[6];

    esp_wifi_get_mac(wifi_ifce, mac);
    //sprintf(instance_name, "%s-" MACSTR, hostname, MAC2STR(mac));
    sprintf(instance_name, MACSTR, MAC2STR(mac));

    //initialize mDNS
    ESP_ERROR_CHECK( mdns_init() );
    //set mDNS hostname (required if you want to advertise services)
    ESP_ERROR_CHECK( mdns_hostname_set(hostname) );
    ESP_LOGI(TAG, "mdns hostname set to: %s", hostname);
    //set default mDNS instance name
    ESP_ERROR_CHECK( mdns_instance_name_set(instance_name) );
    //initialize service
    ESP_ERROR_CHECK( mdns_service_add(instance_name, "_http", "_tcp", 80, NULL, 0) );
    if(strcmp(service_type, "_http") != 0) {
        ESP_ERROR_CHECK( mdns_service_add(instance_name, service_type, "_tcp", 80, serviceTxtData, size) );
    }
}

void srv_mdns_stop() {
    mdns_free();
}