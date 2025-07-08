#include "esp_log.h"
#include "esp_idf_version.h"
#include "esp_app_desc.h"

#include "app_config.h"
#include "ra4m1_ctrl.h"
#include "srv_file.h"
#include "srv_websocket.h"

static const char *TAG = "srv_websocket";

struct async_resp_arg {
    httpd_handle_t hServer;
    int hSocket;
    httpd_ws_frame_t *ws_pkt;
};

ws_callback_t _ws_rx_bin_callback;
httpd_handle_t _server;

/* Add IDF and firmware info */
static void _json_add_system_info(cJSON *data) {
    cJSON *esp_idf = cJSON_CreateObject();
    cJSON_AddItemToObject(data, "esp-idf", esp_idf);
    cJSON_AddStringToObject(esp_idf, "version", esp_get_idf_version());

    cJSON *esp32_firmware = cJSON_CreateObject();
    cJSON_AddItemToObject(data, "esp32_firmware", esp32_firmware);
    const esp_app_desc_t *app_description = esp_app_get_description();
    cJSON_AddStringToObject(esp32_firmware, "version", app_description->version);
    cJSON_AddStringToObject(esp32_firmware, "compile_time", app_description->time);
    cJSON_AddStringToObject(esp32_firmware, "compile_date", app_description->date);
}

/* Add network parameters to json object */
static void _json_get_network_param(cJSON *data) {
    // FIXME: should use #define from app_define somehow or move to main or ...
    cJSON_AddStringToObject(data, JSON_WIFI_SSID    , app_config_get("ssid"));
    cJSON_AddStringToObject(data, JSON_WIFI_PASSWORD, app_config_get("password"));
    cJSON_AddStringToObject(data, JSON_HOSTNAME     , app_config_get("hostname"));
}

/* Set network parameters from json object */
static esp_err_t _json_set_network_param(cJSON *data) {
    const char *keys[] ={JSON_WIFI_SSID, JSON_WIFI_PASSWORD, JSON_HOSTNAME};
    const size_t key_size = sizeof(keys)/sizeof(keys[0]);

    if (cJSON_IsObject(data)) {
        for (int i=0; i < key_size; i++) {
            const cJSON *value = cJSON_GetObjectItemCaseSensitive(data, keys[i]);
            if (cJSON_IsString(value) && (value->valuestring != NULL)) {
                app_config_set(keys[i], value->valuestring);
            } else {
                ESP_LOGW(TAG, "Network param '%s' missing or not a string", keys[i]);
                return ESP_FAIL;
            }
        }
        app_config_save();
        return ESP_OK;
    }
    ESP_LOGW(TAG, "Network param JSON is not a dictionary");
    return ESP_FAIL;
}

static void _srv_websocket_send_callback(void *arg) {
    struct async_resp_arg *rep_arg = arg;

    if (rep_arg == NULL || rep_arg->ws_pkt == NULL) {
        ESP_LOGE(TAG, "WS: Invalid argument to _srv_websocket_send_callback");
        goto cleanup;
    }

    httpd_handle_t hServer = rep_arg->hServer;
    // By default, send to a single client
    size_t fds = 1;
    int client_fds[CONFIG_LWIP_MAX_LISTENING_TCP] = {0};
    client_fds[0] = rep_arg->hSocket;

    // If hSocket is negative, send to all clients
    if (rep_arg->hSocket < 0) {
        fds = CONFIG_LWIP_MAX_LISTENING_TCP;
        esp_err_t err = httpd_get_client_list(hServer, &fds, client_fds);
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "WS: Error (%s) Unable to retrieve list of http clients", esp_err_to_name(err));
            goto cleanup;
        }
    }

    for (size_t i = 0; i < fds; i++) {
        httpd_ws_client_info_t client_info = httpd_ws_get_fd_info(hServer, client_fds[i]);
        if (client_info == HTTPD_WS_CLIENT_WEBSOCKET) {
            esp_err_t send_err = httpd_ws_send_frame_async(hServer, client_fds[i], rep_arg->ws_pkt);
            if (send_err != ESP_OK) {
                ESP_LOGW(TAG, "WS: Failed to send frame async to client %d (%s)", client_fds[i], esp_err_to_name(send_err));
            }
        }
    }

cleanup:
    if(rep_arg != NULL){
        if (rep_arg->ws_pkt != NULL) {
            free(rep_arg->ws_pkt->payload);
            free(rep_arg->ws_pkt);
        }
        free(rep_arg);
    }
}

// Type for payload allocator function
typedef esp_err_t (*ws_payload_alloc_fn)(struct async_resp_arg *rep_arg, void *data, uint32_t data_len);

/* Generic function for sending websocket frames (text or binary) */
static esp_err_t _srv_websocket_send(
    httpd_handle_t hServer,
    int hSocket,
    httpd_ws_type_t ws_type,
    ws_payload_alloc_fn payload_alloc,
    void *data,
    uint32_t data_len) {
    if (payload_alloc == NULL) {
        ESP_LOGE(TAG, "Payload allocator function can't be NULL");
        return ESP_ERR_INVALID_ARG;
    }
    
    struct async_resp_arg *rep_arg = malloc(sizeof(struct async_resp_arg));
    if (rep_arg == NULL) {
        return ESP_ERR_NO_MEM;
    }
    rep_arg->hServer = hServer;
    rep_arg->hSocket = hSocket;
    rep_arg->ws_pkt = malloc(sizeof(httpd_ws_frame_t));
    if (rep_arg->ws_pkt == NULL) {
        free(rep_arg);
        return ESP_ERR_NO_MEM;
    }

    rep_arg->ws_pkt->type = ws_type;

    esp_err_t alloc_ret = payload_alloc(rep_arg, data, data_len);
    if (alloc_ret != ESP_OK) {
        free(rep_arg->ws_pkt);
        free(rep_arg);
        return alloc_ret;
    }

    esp_err_t ret = httpd_queue_work(hServer, _srv_websocket_send_callback, (void *) rep_arg);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "WS Unable to send message");
        free(rep_arg->ws_pkt->payload);
        free(rep_arg->ws_pkt);
        free(rep_arg);
        return ESP_FAIL;
    }
    return ESP_OK;
}

// Allocator for JSON payload (text)
static esp_err_t _ws_payload_alloc_json(struct async_resp_arg *rep_arg, void *data, uint32_t data_len) {
    cJSON *msg = (cJSON *)data;
    char *json_str = cJSON_Print(msg);
    if (!json_str) return ESP_ERR_NO_MEM;
    rep_arg->ws_pkt->payload = (uint8_t *)json_str;
    rep_arg->ws_pkt->len = strlen(json_str);
    return ESP_OK;
}

// Allocator for binary payload
static esp_err_t _ws_payload_alloc_bin(struct async_resp_arg *rep_arg, void *data, uint32_t data_len) {
    if (data_len == 0) return ESP_ERR_INVALID_ARG;
    rep_arg->ws_pkt->payload = malloc(data_len);
    if (!rep_arg->ws_pkt->payload) return ESP_ERR_NO_MEM;
    memcpy(rep_arg->ws_pkt->payload, data, data_len);
    rep_arg->ws_pkt->len = data_len;
    return ESP_OK;
}

/* Send result in json message using websocket.
    Function can be called from a different thread) */
static esp_err_t _srv_websocket_send_json_result(httpd_req_t *req, int msgId, esp_err_t result, bool all) {
    cJSON *msg = cJSON_CreateObject();
    cJSON_AddNumberToObject(msg, JSON_MSG, msgId);
    cJSON_AddNumberToObject(msg, "result", result);
    int hSocket = all ? -1 : httpd_req_to_sockfd(req);
    esp_err_t ret = _srv_websocket_send(
        req->handle, hSocket, HTTPD_WS_TYPE_TEXT, _ws_payload_alloc_json, msg, 0);
    cJSON_Delete(msg);
    return ret;
}

/* Send json message using websocket.
    Function can be called from a different thread) */
static esp_err_t _srv_websocket_send_json(httpd_req_t *req, int msgId, void (*json_data)(cJSON *), bool all) {
    cJSON *msg = cJSON_CreateObject();
    cJSON_AddNumberToObject(msg, JSON_MSG, msgId);
    if (json_data) {
        cJSON *data = cJSON_CreateObject();
        json_data(data);
        cJSON_AddItemToObject(msg, JSON_DATA, data);
    }
    int hSocket = all ? -1 : httpd_req_to_sockfd(req);
    esp_err_t ret = _srv_websocket_send(
        req->handle, hSocket, HTTPD_WS_TYPE_TEXT, _ws_payload_alloc_json, msg, 0);
    cJSON_Delete(msg);
    return ret;
}

/* Send binary message using websocket.
    Function can be called from a different thread) */
esp_err_t srv_websocket_send_bin(uint8_t *buffer, uint32_t buffer_length) {
    return _srv_websocket_send(
        _server, -1, HTTPD_WS_TYPE_BINARY, _ws_payload_alloc_bin, buffer, buffer_length);
}

/* Handler processing incoming requests  */
esp_err_t srv_websocket_get_handler(httpd_req_t *req) {
    if (req->method == HTTP_GET) {
        ESP_LOGI(TAG, "Handshake done, a new connection is opened");
        return ESP_OK;
    }
    httpd_ws_frame_t ws_pkt;

    uint8_t *buf = NULL;
    memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
    ws_pkt.type = HTTPD_WS_TYPE_TEXT;
    /* Set max_len = 0 to get the frame len */
    esp_err_t ret = httpd_ws_recv_frame(req, &ws_pkt, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "httpd_ws_recv_frame failed to get frame len with %d", ret);
        return ret;
    }
    if (ws_pkt.len) {
        /* ws_pkt.len + 1 is for NULL termination as we are expecting a string */
        buf = calloc(1, ws_pkt.len + 1);
        if (buf == NULL) {
            ESP_LOGE(TAG, "Failed to calloc memory for buf");
            return ESP_ERR_NO_MEM;
        }
        ws_pkt.payload = buf;
        /* Set max_len = ws_pkt.len to get the frame payload */
        ret = httpd_ws_recv_frame(req, &ws_pkt, ws_pkt.len);
        if (ret == ESP_OK) {
            // Only process the packet if there is a payload and frame was received correctly
            switch (ws_pkt.type) {
                case HTTPD_WS_TYPE_TEXT:
                    // FIXME: Use a table[id] -> JSON_REQ, callback, ... ?
                    cJSON *json = cJSON_Parse((char *)ws_pkt.payload);
                    if (json == NULL) {
                        ESP_LOGE(TAG, "Failed to parse incoming WebSocket JSON: %s", (char *)ws_pkt.payload);
                        break;
                    }
                    cJSON *json_id = cJSON_GetObjectItem(json,"id");
                    if (json_id != NULL) {
                        esp_err_t result;
                        switch (json_id->valueint) {
                            case JSON_MSG_REQ_SYS_INFO:
                                _srv_websocket_send_json(req, JSON_MSG_REP_SYS_INFO, _json_add_system_info, false);                
                                break;
                            case JSON_MSG_REQ_ESP32_RESET:
                                esp_restart();
                                break;
                            case JSON_MSG_REQ_RA4M1_RESET:
                                ra4m1_ctrl_restart();
                                break;
                            case JSON_MSG_REQ_GET_NETWORKPARAM:
                                _srv_websocket_send_json(req, JSON_MSG_REP_GET_NETWORKPARAM, _json_get_network_param, false);
                                break;
                            case JSON_MSG_REQ_SET_NETWORKPARAM:
                                result = _json_set_network_param(cJSON_GetObjectItemCaseSensitive(json,"data"));
                                _srv_websocket_send_json_result(req, JSON_MSG_REP_SET_NETWORKPARAM, result, false);
                                break;
                            case JSON_MSG_REQ_LIST_FILES:
                                _srv_websocket_send_json(req, JSON_MSG_REP_LIST_FILES, srv_file_json_list_files, false);
                                break;
                            case JSON_MSG_REQ_DELETE_FILES:
                                result = srv_file_json_delete_files(cJSON_GetObjectItem(json,"list_files"));
                                _srv_websocket_send_json_result(req, JSON_MSG_REP_DELETE_FILES, result, false);
                                break;
                            default:
                                ESP_LOGW(TAG, "Unknown message id (%d)", json_id->valueint);
                                break;
                        }
                    }
                    cJSON_Delete(json);
                    break;
                case HTTPD_WS_TYPE_BINARY:
                    if (_ws_rx_bin_callback) {
                        _ws_rx_bin_callback(ws_pkt.payload, ws_pkt.len);
                    } else {
                        ESP_LOGW(TAG, "No binary WebSocket callback registered, binary message ignored");
                    }
                    break;
                default:
                    ESP_LOGW(TAG, "Unexpected packet type: %d", ws_pkt.type);
                    break;
            }
        } else {
            ESP_LOGE(TAG, "httpd_ws_recv_frame failed with %d", ret);
        }
    }

    if (buf) {
        free(buf);
    }
    return ret;
}

/* Initialize websocket service*/
esp_err_t srv_websocket_init(httpd_handle_t server, ws_callback_t ws_rx_bin_callback) {
    ESP_LOGI(TAG, "Setup websocket service");
    _server = server;
    _ws_rx_bin_callback = ws_rx_bin_callback;
    return ESP_OK;
}