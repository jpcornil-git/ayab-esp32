#include <string.h>

#include "esp_log.h"
#include "esp_spiffs.h"
#include "esp_vfs.h"

#include "srv_spiffs.h"

typedef struct {
    char base_path[ESP_VFS_PATH_MAX+1];  
} srv_spiffs_data_t;

static const char *TAG = "srv_spiffs";

static srv_spiffs_data_t _self;

esp_err_t srv_spiffs_start(char* base_path) {
    ESP_LOGI(TAG, "Initializing SPIFFS");
    ESP_ERROR_CHECK(base_path != NULL ? ESP_OK : ESP_FAIL);

    strncpy(_self.base_path, base_path, ESP_VFS_PATH_MAX);
    _self.base_path[ESP_VFS_PATH_MAX] = '\0';

    esp_vfs_spiffs_conf_t conf = {
      .base_path = _self.base_path,
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
        return ret;
    }
/* Not acceptable for production, WDT timeout has to be reconfigured
to at least 70s to avoid a TWDT watchdog reset ... 
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
    } else {
        ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
    }
    return ret;
}

esp_err_t srv_spiffs_restart() {
    return srv_spiffs_start(_self.base_path);
}

esp_err_t srv_spiffs_stop(void) {
    return esp_vfs_spiffs_unregister(NULL);
}
