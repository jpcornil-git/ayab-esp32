#include <string.h>

#include "esp_log.h"
#include "esp_littlefs.h"
#include "esp_vfs.h"

#include "srv_littlefs.h"

typedef struct {
    char base_path[ESP_VFS_PATH_MAX+1];  
} srv_littlefs_data_t;

static const char *TAG = "srv_littlefs";

static srv_littlefs_data_t _self;

esp_err_t srv_littlefs_start(char* base_path) {
    ESP_LOGI(TAG, "Initializing LITTLEFS");
    ESP_ERROR_CHECK(base_path != NULL ? ESP_OK : ESP_FAIL);

    strncpy(_self.base_path, base_path, ESP_VFS_PATH_MAX);
    _self.base_path[ESP_VFS_PATH_MAX] = '\0';

    esp_vfs_littlefs_conf_t conf = {
      .base_path = _self.base_path,
      .partition_label = NULL,
      .format_if_mount_failed = false
    };
    esp_err_t ret = esp_vfs_littlefs_register(&conf);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "Failed to find LITTLEFS partition");
        } else {
            ESP_LOGE(TAG, "Failed to initialize LITTLEFS (%s)", esp_err_to_name(ret));
        }
        return ret;
    }

    size_t total = 0, used = 0;
    ret = esp_littlefs_info(NULL, &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get LITTLEFS partition information (%s)", esp_err_to_name(ret));
    } else {
        ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
    }
    return ret;
}

esp_err_t srv_littlefs_restart() {
    return srv_littlefs_start(_self.base_path);
}

esp_err_t srv_littlefs_stop(void) {
    return esp_vfs_littlefs_unregister(NULL);
}
