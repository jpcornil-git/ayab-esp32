#ifndef _SRV_SPIFFS_H_
#define _SRV_SPIFFS_H_

#include "esp_err.h"

/**
 * @brief Initialize and mount the LITTLEFS filesystem.
 *
 * This function mounts the LITTLEFS filesystem at the given base path.
 * It must be called before any file operations.
 *
 * @param base_path The mount point path for LITTLEFS (e.g., "/littlefs").
 * @return ESP_OK on success, or an error code from esp_err_t on failure.
 */
esp_err_t srv_littlefs_start(char *base_path);

/**
 * @brief Restart (unmount and remount) the LITTLEFS filesystem.
 *
 * This function unmounts and remounts the LITTLEFS filesystem using the last base path.
 *
 * @return ESP_OK on success, or an error code from esp_err_t on failure.
 */
esp_err_t srv_littlefs_restart(void);

/**
 * @brief Unmount the LITTLEFS filesystem and release resources.
 *
 * This function unmounts the LITTLEFS filesystem and frees associated resources.
 *
 * @return ESP_OK on success, or an error code from esp_err_t on failure.
 */
esp_err_t srv_littlefs_stop(void);

#endif