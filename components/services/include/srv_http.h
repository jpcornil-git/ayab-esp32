#ifndef _SRV_HTTP_H_
#define _SRV_HTTP_H_

#include <freertos/FreeRTOS.h>

#define SRV_HTTP_PATH_WWW "/www"
#define SRV_HTTP_PATH_WS "/ws"
#define SRV_HTTP_PATH_OTA "/ota"
#define SRV_HTTP_PATH_NETWORK "/network"

typedef BaseType_t (*ws_callback_t)(const uint8_t *payload, size_t len);

void srv_http_start(const char *base_path, ws_callback_t ws_rx_bin_callback);
void srv_http_stop();

#endif