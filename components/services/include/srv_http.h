#ifndef _SRV_HTTP_H_
#define _SRV_HTTP_H_

#include <freertos/FreeRTOS.h>


/**
 * @brief Path for static web content.
 */
#define SRV_HTTP_PATH_WWW "/www"
/**
 * @brief Path for websocket endpoint.
 */
#define SRV_HTTP_PATH_WS "/ws"
/**
 * @brief Path for OTA update endpoint.
 */
#define SRV_HTTP_PATH_OTA "/ota"

/**
 * @brief Callback type for handling received websocket binary frames.
 *
 * @param payload Pointer to the received binary data.
 * @param len     Length of the received data.
 * @return BaseType_t Application-defined return value.
 */
typedef BaseType_t (*ws_callback_t)(const uint8_t *payload, size_t len);


/**
 * @brief Start the HTTP server and register URI handlers.
 *
 * @param base_path           Base path for static file serving.
 * @param ws_rx_bin_callback  Callback for handling websocket binary frames.
 */
void srv_http_start(const char *base_path, ws_callback_t ws_rx_bin_callback);

/**
 * @brief Stop the HTTP server and release resources.
 */
void srv_http_stop();

#endif