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

esp_err_t srv_websocket_send_bin(uint8_t *buffer, uint32_t buffer_length);
esp_err_t srv_websocket_get_handler(httpd_req_t *req);
esp_err_t srv_websocket_init(httpd_handle_t server, ws_callback_t ws_rx_bin_callback);

#endif