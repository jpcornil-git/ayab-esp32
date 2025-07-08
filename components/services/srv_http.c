#include <stdint.h>

#include "esp_log.h"
#include "esp_http_server.h"

#include "app_config.h"
#include "ota_app.h"
#include "ota_handler.h"
#include "srv_file.h"
#include "srv_http.h"
#include "srv_websocket.h"

static const char *TAG = "srv_http";

static httpd_handle_t _server = NULL;

void srv_http_start(const char *base_path, ws_callback_t ws_rx_bin_callback) {

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.uri_match_fn = httpd_uri_match_wildcard;
    config.lru_purge_enable = true;

    // Start the httpd server
    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);

    esp_err_t err = httpd_start(&_server, &config);
    if (err == ESP_OK) {
        // Initialize websocket service
        srv_websocket_init(_server, ws_rx_bin_callback);

        // Set URI handlers
        ESP_LOGI(TAG, "Registering URI handlers");
        // URI handler for websocket
        httpd_uri_t ws_uri = {
            .uri       = SRV_HTTP_PATH_WS,  // websocket path
            .method    = HTTP_GET,
            .handler   = srv_websocket_get_handler,
            .user_ctx  = NULL,
            .is_websocket = true
        };
        httpd_register_uri_handler(_server, &ws_uri);

        /* URI handler for uploading app image to flash */
        httpd_uri_t ota_upload = {
            .uri       = SRV_HTTP_PATH_OTA,   // Match all URIs of type /upload/path/to/file
            .method    = HTTP_POST,
            .handler   = ota_post_handler,
            .user_ctx  = NULL
        };
        httpd_register_uri_handler(_server, &ota_upload);
        
        // Start file service (should be called after above code as it registers uri as well)
        srv_file_start(_server, base_path, SRV_HTTP_PATH_WWW);
    }
    // Rollback in case of error after a fisrt App execution
    ota_app_validate(err);
}

void srv_http_stop() {
    if (_server != NULL) {
        // Stop the httpd server
        ESP_LOGI(TAG, "Stopping server");
        httpd_stop(_server);
        _server = NULL;
        srv_file_stop();
    } else {
        ESP_LOGW(TAG, "Server stopped already !");
    }
}