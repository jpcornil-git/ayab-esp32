#ifndef _SRV_MDNS_H_
#define _SRV_MDNS_H_
#include "mdns.h"
#include "esp_wifi.h"


/**
 * @brief Start the mDNS service and register a service on the network.
 *
 * @param hostname      The hostname to advertise via mDNS.
 * @param wifi_ifce     The Wi-Fi interface to use for getting the MAC address.
 * @param service_type  The type of service to advertise (e.g., "_http").
 * @param serviceTxtData Pointer to an array of TXT records for the service.
 * @param size          Number of TXT records in serviceTxtData.
 */
void srv_mdns_start(const char *hostname, wifi_interface_t wifi_ifce, const char *service_type, mdns_txt_item_t *serviceTxtData, size_t size);

/**
 * @brief Stop the mDNS service and free resources.
 */
void srv_mdns_stop();

#endif