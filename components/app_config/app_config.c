#include <string.h>
#include "esp_log.h"

#include "app_config.h"

static const char *TAG = "app_config";

static char namespace[NVS_KEY_NAME_MAX_SIZE] = {0};

static app_config_t *app_config; // Reference to a global variable
static size_t app_config_size;

//static SemaphoreHandle_t app_config_mutex = NULL;

void app_config_init(const char *name, app_config_t *config, size_t size) {
    // Initialize NVS
    strncpy(namespace, name, NVS_KEY_NAME_MAX_SIZE);
    app_config_size = size;
    app_config = config;
    app_config_reset();
//    app_config_mutex = xSemaphoreCreateMutex();
}

char *app_config_get(const char *key) { 
//    xSemaphoreTake(app_config_mutex, portMAX_DELAY);
    for (size_t i=0; i< app_config_size; i++){
        if (strcmp(key, app_config[i].key) == 0) {
            return app_config[i].value;
        }
    }
//    xSemaphoreGive(app_config_mutex);
    ESP_LOGW(TAG, "%s key not found (get)", key);
    return NULL;
}

void app_config_reset() {
    // Set default values
    for (size_t i=0; i< app_config_size; i++){
        strncpy(app_config[i].value, app_config[i].default_value, app_config[i].size);
    }
}

void app_config_set(const char *key, const char *value) {
//    xSemaphoreTake(app_config_mutex, portMAX_DELAY);
    for (size_t i=0; i< app_config_size; i++){
        if (strcmp(key, app_config[i].key) == 0) {
            strncpy(app_config[i].value, value, app_config[i].size);
            return;
        }
    }
//    xSemaphoreGive(app_config_mutex);
    ESP_LOGW(TAG, "%s key not found (set)", key);
}

esp_err_t app_config_load() {
//    xSemaphoreTake(app_config_mutex, portMAX_DELAY);
    // Read config from nvs
    nvs_handle_t nvs_ayab;
    esp_err_t err = nvs_open(namespace, NVS_READONLY, &nvs_ayab);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Error (%s) opening NVS handle", esp_err_to_name(err));
        return ESP_FAIL;
    }

    for (size_t i=0; i< app_config_size; i++){
        size_t len = 0;
        err = nvs_get_str(nvs_ayab, app_config[i].key, NULL, &len);
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "Error (%s) while fetching %s from NVS", esp_err_to_name(err), app_config[i].key);
            break;
        }
        if (len > app_config[i].size) {
            ESP_LOGW(TAG, "Not enough space to store %s (%u < %u)", app_config[i].key, app_config[i].size, len);
            err = ESP_FAIL;
            break;
        }
        err = nvs_get_str(nvs_ayab, app_config[i].key, app_config[i].value, &len);
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "Error (%s) while fetching %s from NVS", esp_err_to_name(err), app_config[i].key);
            break;
        }
        ESP_LOGI(TAG, "%s = %s", app_config[i].key, app_config[i].value);
    }
    nvs_close(nvs_ayab);
    if (err != ESP_OK) {
        err = ESP_FAIL;
    } else {
        ESP_LOGI(TAG, "Configuration read from storage");    
    }
//    xSemaphoreGive(app_config_mutex);
    return err;
}
    
esp_err_t app_config_save() {
//    xSemaphoreTake(app_config_mutex, portMAX_DELAY);
    // Save config to nvs
    nvs_handle_t nvs_ayab;
    esp_err_t err = nvs_open(namespace, NVS_READWRITE, &nvs_ayab);
    if (err == ESP_OK) {
        nvs_erase_all(nvs_ayab);
        for (size_t i=0; i< app_config_size; i++){
            ESP_ERROR_CHECK(nvs_set_str(nvs_ayab, app_config[i].key, app_config[i].value));
        }
        nvs_commit(nvs_ayab);        
        ESP_LOGI(TAG, "Configuration committed to storage");  
    } else {
        ESP_LOGW(TAG, "Error (%s) opening NVS handle, storage update failed", esp_err_to_name(err));
    }
    nvs_close(nvs_ayab);
//    xSemaphoreGive(app_config_mutex);
    return err;  
}
