#ifndef _OTA_LITTLEFS_H_
#define _OTA_LITTLEFS_H_
#include "esp_err.h"
#include "esp_partition.h"

const esp_partition_t *ota_littlefs_get_partition();

/**
 * @brief Initialize the LITTLEFS file system for OTA updates.
 *
 * This function sets up the LITTLEFS file system for OTA updates.
 *
 * @return ESP_OK on success, or an error code on failure.
 */
esp_err_t ota_littlefs_begin();

/**
 * @brief Write chunk of data to LITTLEFS during OTA update.
 *
 * This function writes a chunk of data to LITTLEFS during an OTA update.
 *
 * @param data Pointer to the data to write.
 * @param len Length of the data in bytes.
 * @return ESP_OK on success, or an error code on failure.
 */
esp_err_t ota_littlefs_write(char *data, size_t len);

/**
 * @brief End the LITTLEFS file system update.
 *
 * This function finalizes the LITTLEFS file system update, marking it as complete.
 *
 * @return ESP_OK on success, or an error code on failure.
 */
esp_err_t ota_littlefs_end();

/**
 * @brief Check if the LITTLEFS update is in progress.
 *
 * This function checks if a LITTLEFS update is currently in progress.
 *
 * @return true if a LITTLEFS update is in progress, false otherwise.
 */
void ota_littlefs_abort();

#endif