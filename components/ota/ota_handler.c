#include <sys/param.h>

#include "esp_log.h"
#include "esp_partition.h"

#include "ota_handler.h"
#include "ota_app.h"
#include "ota_littlefs.h"
#include "ra4m1_flash.h"

#define OTA_ESP32_APP      0
#define OTA_ESP32_LITTLEFS 1
#define OTA_RA4M1_FIRMWARE 2
#define OTA_BUFFER_SIZE 8192

typedef struct {
    esp_err_t (*begin)();
    esp_err_t (*write)(char *data, size_t len);
    esp_err_t (*end)();
    void (*abort)();
    const esp_partition_t *(*get_partition)();
} ota_func_t;

static const char *TAG = "ota_handler";

static ota_func_t ota_types[] = {
    { .begin = ota_app_begin,     .write = ota_app_write,     .end = ota_app_end,     .abort = ota_app_abort,      .get_partition = ota_app_get_partition},
    { .begin = ota_littlefs_begin,.write = ota_littlefs_write,.end = ota_littlefs_end,.abort = ota_littlefs_abort, .get_partition = ota_littlefs_get_partition},
    { .begin = ra4m1_flash_begin, .write = ra4m1_flash_write, .end = ra4m1_flash_end, .abort = ra4m1_flash_abort , .get_partition = ra4m1_flash_get_partition},
};

// Handler to update OTA updates
esp_err_t ota_post_handler(httpd_req_t *req) {
    char *ota_buffer;
    char *query_buf;
    int ota_type = -1;
    ota_func_t ota_func;

    size_t query_buf_len = httpd_req_get_url_query_len(req) + 1;
    if (query_buf_len > 1) {
        query_buf = malloc(query_buf_len);
        if (httpd_req_get_url_query_str(req, query_buf, query_buf_len) == ESP_OK) {
            char value[16];
            if (httpd_query_key_value(query_buf, "binaryType", value, sizeof(value)) == ESP_OK) {
                if (!strcmp("esp32_app", value)) {
                    ota_type = OTA_ESP32_APP;
                } else if (!strcmp("esp32_littlefs", value)) {
                    ota_type = OTA_ESP32_LITTLEFS;
                } else if (!strcmp("ra4m1_app", value)) {
                    ota_type = OTA_RA4M1_FIRMWARE;
                }
                ESP_LOGI(TAG, "OTA (type=%s [%d])", value, ota_type);
            }
        }
    }
    if (ota_type < 0) {
        ESP_LOGE(TAG, "Invalid or missing binaryType in the URI");
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid or missing binaryType in the URI");
        return ESP_FAIL;        
    }

    ota_func = ota_types[ota_type];

    ota_buffer = malloc(OTA_BUFFER_SIZE * sizeof(uint8_t));
    if (ota_buffer == NULL) {
        ESP_LOGE(TAG, "Unable to allocate OTA buffer (%d bytes)", OTA_BUFFER_SIZE);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Unable to allocate OTA memory");
        return ESP_FAIL;
    }

    esp_err_t err = ota_func.begin();
    if (err != ESP_OK) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Unable to begin OTA update");
        return ESP_FAIL;
    }

    /* File cannot be larger than the  partition size */
    if (req->content_len > ota_func.get_partition()->size) {
        ESP_LOGE(TAG, "OTA Binary too large : %d bytes", req->content_len);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Binary too large !");
        return ESP_FAIL;
    }

    int received;
    int progress = 0;
    int remaining = req->content_len;
    ESP_LOGI(TAG, "Flashing binary (%u bytes) ...", remaining);
    while (remaining > 0) {
        /* Receive the file part by part into a buffer */
        if ((received = httpd_req_recv(req, ota_buffer, MIN(remaining, OTA_BUFFER_SIZE))) > 0) {
            err = ota_func.write(ota_buffer, received);
            if (err != ESP_OK) {
                httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "OTA update failed");
                ota_func.abort();
                return ESP_FAIL;
            }
        } else {
            if (received == HTTPD_SOCK_ERR_TIMEOUT) {
                /* Retry if timeout occurred */
                continue;
            }
            ESP_LOGE(TAG, "Failed to receive file!");
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Error while receiving app binary");
            ota_func.abort();
            return ESP_FAIL;            
        }
        remaining -= received;
        int new_progress = 100 - (100 * remaining) / req->content_len;
        if ((new_progress - progress) > 4) {
            progress = new_progress;
            ESP_LOGI(TAG, "%d %%", progress);
        }
    }
    ESP_LOGI(TAG, "OTA update completed successfully");
    ota_func.end();
    free(ota_buffer);

    return httpd_resp_sendstr(req, "OTA update completed successfully");
}
