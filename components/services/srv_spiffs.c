#include <string.h>

#include "esp_log.h"
#include "esp_spiffs.h"
#include "esp_vfs.h"

#include "srv_spiffs.h"

static const char *TAG = "srv_spiffs";

static char _base_path[ESP_VFS_PATH_MAX+1];

void srv_spiffs_start(char* base_path) {
    ESP_LOGI(TAG, "Initializing SPIFFS");
    ESP_ERROR_CHECK(base_path != NULL ? ESP_OK : ESP_FAIL);

    strncpy(_base_path, base_path, ESP_VFS_PATH_MAX);

    esp_vfs_spiffs_conf_t conf = {
      .base_path = _base_path,
      .partition_label = NULL,
      .max_files = 4,
      .format_if_mount_failed = false
    };
    esp_err_t ret = esp_vfs_spiffs_register(&conf);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "Failed to find SPIFFS partition");
        } else {
            ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        return;
    }
/* Trigger a WDT
    ESP_LOGI(TAG, "Performing SPIFFS_check().");
    ret = esp_spiffs_check(conf.partition_label);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPIFFS_check() failed (%s)", esp_err_to_name(ret));
        return;
    } else {
        ESP_LOGI(TAG, "SPIFFS_check() successful");
    }
*/
    size_t total = 0, used = 0;
    ret = esp_spiffs_info(NULL, &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
        return;
    }
    ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
}

void srv_spiffs_restart() {
    srv_spiffs_start(_base_path);
}

void srv_spiffs_stop(void) {
    esp_vfs_spiffs_unregister(NULL);
}
