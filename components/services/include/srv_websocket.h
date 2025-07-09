#ifndef _SRV_WEBSOCKET_H_
#define _SRV_WEBSOCKET_H_

#include "esp_http_server.h"

#include "srv_http.h"
#include "cJSON.h"

#define JSON_MSG  "id"
#define JSON_DATA "data"
#define JSON_WIFI_SSID "ssid"
#define JSON_WIFI_PASSWORD "password"
#define JSON_HOSTNAME "hostname"
#define JSON_RA4M1_RESET "ra4m1_reset"

#define JSON_MSG_REQ_SYS_INFO           1
#define JSON_MSG_REP_SYS_INFO           (128 + JSON_MSG_REQ_SYS_INFO)
#define JSON_MSG_REQ_ESP32_RESET        2
#define JSON_MSG_REQ_RA4M1_RESET        3
#define JSON_MSG_REQ_GET_NETWORKPARAM   16
#define JSON_MSG_REP_GET_NETWORKPARAM   (128 + JSON_MSG_REQ_GET_NETWORKPARAM)
#define JSON_MSG_REQ_SET_NETWORKPARAM   17
#define JSON_MSG_REP_SET_NETWORKPARAM   (128 + JSON_MSG_REQ_SET_NETWORKPARAM)
#define JSON_MSG_REQ_LIST_FILES         32
#define JSON_MSG_REP_LIST_FILES         (128 + JSON_MSG_REQ_LIST_FILES)
#define JSON_MSG_REQ_DELETE_FILES       33
#define JSON_MSG_REP_DELETE_FILES       (128 + JSON_MSG_REQ_DELETE_FILES)

/**
 * @brief Send a binary WebSocket message to all connected clients.
 *
 * @param buffer Pointer to the binary data to send.
 * @param buffer_length Length of the binary data in bytes.
 * @return ESP_OK on success, or an error code from esp_err_t on failure.
 */
esp_err_t srv_websocket_send_bin(uint8_t *buffer, uint32_t buffer_length);

/**
 * @brief HTTP server handler for incoming WebSocket frames.
 *
 * This function should be registered as the handler for WebSocket requests.
 * It processes incoming WebSocket frames (text or binary), dispatches JSON commands,
 * and calls the registered binary callback if needed.
 *
 * @param req Pointer to the HTTP server request structure.
 * @return ESP_OK on success, or an error code from esp_err_t on failure.
 */
esp_err_t srv_websocket_get_handler(httpd_req_t *req);

/**
 * @brief Initialize the WebSocket service and register the binary receive callback.
 *
 * @param server The HTTP server handle.
 * @param ws_rx_bin_callback Callback function to handle incoming binary WebSocket frames.
 * @return ESP_OK on success, or an error code from esp_err_t on failure.
 */
esp_err_t srv_websocket_init(httpd_handle_t server, ws_callback_t ws_rx_bin_callback);

#endif