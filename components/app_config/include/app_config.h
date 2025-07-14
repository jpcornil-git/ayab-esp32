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

/**
 * @brief Initialize the application configuration.
 *
 * @param name Name of the NVS namespace.
 * @param config Pointer to the application configuration array.
 * @param size Size of the configuration array.
 */
void app_config_init(const char *name, app_config_t *config, size_t size);

/**
 * @brief Get the value associated with a key in the application configuration.
 *
 * @param key The key to look up.
 * @return The value associated with the key, or NULL if not found.
 */
char *app_config_get(const char *key);

/**
 * @brief Reset the application configuration to default values.
 */
void app_config_reset();

/**
 * @brief Set a value in the application configuration.
 *
 * @param key The key to set.
 * @param value The value to associate with the key.
 */
void app_config_set(const char *key, const char *value);

/**
 * @brief Load the application configuration from NVS.
 *
 * @return ESP_OK on success, or an error code on failure.
 */
esp_err_t app_config_load();

/**
 * @brief Save the application configuration to NVS.
 *
 * @return ESP_OK on success, or an error code on failure.
 */
esp_err_t app_config_save();

#endif   