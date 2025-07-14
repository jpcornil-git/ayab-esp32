#include <string.h>
#include <freertos/FreeRTOS.h>
#include "esp_log.h"

#include "app_config.h"

typedef struct {
    char namespace[NVS_KEY_NAME_MAX_SIZE]; // Namespace for NVS
    app_config_t *config; // Pointer to the application configuration
    size_t config_size; // Size of the configuration array
    SemaphoreHandle_t config_mutex; // Mutex for thread safety
} ota_app_config_t;

static const char *TAG = "app_config";

static ota_app_config_t _self = {
    .namespace = {0},
};

void app_config_init(const char *name, app_config_t *config, size_t size) {
    _self.config_mutex = xSemaphoreCreateMutex();
    strncpy(_self.namespace, name, NVS_KEY_NAME_MAX_SIZE);
    _self.namespace[NVS_KEY_NAME_MAX_SIZE - 1] = '\0';
    _self.config_size = size;
    _self.config = config;
    app_config_reset();
}

char *app_config_get(const char *key) {
    xSemaphoreTake(_self.config_mutex, portMAX_DELAY);
    for (size_t i=0; i< _self.config_size; i++){
        if (strcmp(key, _self.config[i].key) == 0) {
            xSemaphoreGive(_self.config_mutex);
            return _self.config[i].value;
        }
    }
    xSemaphoreGive(_self.config_mutex);
    ESP_LOGW(TAG, "%s key not found (get)", key);
    return NULL;
}

void app_config_reset() {
    xSemaphoreTake(_self.config_mutex, portMAX_DELAY);
    // Set default values
    for (size_t i=0; i< _self.config_size; i++){
        strncpy(_self.config[i].value, _self.config[i].default_value, _self.config[i].size);
    }
    xSemaphoreGive(_self.config_mutex);
}

void app_config_set(const char *key, const char *value) {
    xSemaphoreTake(_self.config_mutex, portMAX_DELAY);
    for (size_t i=0; i< _self.config_size; i++){
        if (strcmp(key, _self.config[i].key) == 0) {
            strncpy(_self.config[i].value, value, _self.config[i].size);
            xSemaphoreGive(_self.config_mutex);
            return;
        }
    }
    xSemaphoreGive(_self.config_mutex);
    ESP_LOGW(TAG, "%s key not found (set)", key);
}

esp_err_t app_config_load() {
    xSemaphoreTake(_self.config_mutex, portMAX_DELAY);
    nvs_handle_t nvs_ayab;
    esp_err_t err = nvs_open(_self.namespace, NVS_READONLY, &nvs_ayab);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Error (%s) opening NVS handle", esp_err_to_name(err));
        xSemaphoreGive(_self.config_mutex);
        return ESP_FAIL;
    }

    for (size_t i=0; i< _self.config_size; i++){
        size_t len = 0;
        err = nvs_get_str(nvs_ayab, _self.config[i].key, NULL, &len);
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "Error (%s) while fetching %s from NVS", esp_err_to_name(err), _self.config[i].key);
            break;
        }
        if (len > _self.config[i].size) {
            ESP_LOGW(TAG, "Not enough space to store %s (%u < %u)", _self.config[i].key, _self.config[i].size, len);
            err = ESP_FAIL;
            break;
        }
        err = nvs_get_str(nvs_ayab, _self.config[i].key, _self.config[i].value, &len);
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "Error (%s) while fetching %s from NVS", esp_err_to_name(err), _self.config[i].key);
            break;
        }
        ESP_LOGI(TAG, "%s = %s", _self.config[i].key, _self.config[i].value);
    }
    nvs_close(nvs_ayab);
    xSemaphoreGive(_self.config_mutex);
    if (err != ESP_OK) {
        err = ESP_FAIL;
    } else {
        ESP_LOGI(TAG, "Configuration read from storage");    
    }
    return err;
}

esp_err_t app_config_save() {
    xSemaphoreGive(_self.config_mutex);
    nvs_handle_t nvs_ayab;
    esp_err_t err = nvs_open(_self.namespace, NVS_READWRITE, &nvs_ayab);
    if (err == ESP_OK) {
        nvs_erase_all(nvs_ayab);
        for (size_t i=0; i< _self.config_size; i++){
            ESP_ERROR_CHECK(nvs_set_str(nvs_ayab, _self.config[i].key, _self.config[i].value));
        }
        nvs_commit(nvs_ayab);        
        ESP_LOGI(TAG, "Configuration committed to storage");  
    } else {
        ESP_LOGW(TAG, "Error (%s) opening NVS handle, storage update failed", esp_err_to_name(err));
    }
    nvs_close(nvs_ayab);
    xSemaphoreGive(_self.config_mutex);
    return err;  
}
