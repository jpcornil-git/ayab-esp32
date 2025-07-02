#ifndef _APP_CONFIG_H_
#define _APP_CONFIG_H_

#include <stddef.h>
#include "nvs_flash.h"

#define APP_CONFIG_VALUE_SIZE 32

typedef struct {
    char key[NVS_KEY_NAME_MAX_SIZE];
    char *value;
    size_t size;
    char *default_value;
} app_config_t;

void app_config_init(const char *name, app_config_t *config, size_t size);
char *app_config_get(const char *key);
void app_config_reset();
void app_config_set(const char *key, const char *value);
esp_err_t app_config_load();
esp_err_t app_config_save();

#endif   