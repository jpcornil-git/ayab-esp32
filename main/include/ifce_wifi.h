#ifndef _IFCE_WIFI_H_
#define _IFCE_WIFI_H_

void ifce_wifi_start_STA(EventGroupHandle_t *event_group, const char* ssid, const char* password);
void ifce_wifi_start_AP(EventGroupHandle_t *event_group);

void ifce_wifi_stop(void);

#endif