#ifndef _SRV_MDNS_H_
#define _SRV_MDNS_H_
#include "mdns.h"
#include "esp_wifi.h"

void srv_mdns_start(const char *hostname, wifi_interface_t wifi_ifce, const char *service_type, mdns_txt_item_t *serviceTxtData, size_t size);
void srv_mdns_stop();

#endif