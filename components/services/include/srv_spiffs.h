#ifndef _SRV_SPIFFS_H_
#define _SRV_SPIFFS_H_

#include "esp_err.h"

/**
 * @brief Initialize and mount the SPIFFS filesystem.
 *
 * This function mounts the SPIFFS filesystem at the given base path.
 * It must be called before any file operations.
 *
 * @param base_path The mount point path for SPIFFS (e.g., "/spiffs").
 * @return ESP_OK on success, or an error code from esp_err_t on failure.
 */
esp_err_t srv_spiffs_start(char *base_path);

/**
 * @brief Restart (unmount and remount) the SPIFFS filesystem.
 *
 * This function unmounts and remounts the SPIFFS filesystem using the last base path.
 *
 * @return ESP_OK on success, or an error code from esp_err_t on failure.
 */
esp_err_t srv_spiffs_restart(void);

/**
 * @brief Unmount the SPIFFS filesystem and release resources.
 *
 * This function unmounts the SPIFFS filesystem and frees associated resources.
 *
 * @return ESP_OK on success, or an error code from esp_err_t on failure.
 */
esp_err_t srv_spiffs_stop(void);

#endif