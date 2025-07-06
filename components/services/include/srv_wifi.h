#ifndef _SRV_WIFI_H_
#define _SRV_WIFI_H_

void srv_wifi_start_STA(EventGroupHandle_t event_group, const char* ssid, const char* password);
void srv_wifi_start_AP(EventGroupHandle_t event_group);

void srv_wifi_stop(void);

#endif