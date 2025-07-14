#ifndef _OTA_SPIFFS_H_
#define _OTA_SPIFFS_H_
#include "esp_err.h"
#include "esp_partition.h"

const esp_partition_t *ota_spiffs_get_partition();

/**
 * @brief Initialize the SPIFFS file system for OTA updates.
 *
 * This function sets up the SPIFFS file system for OTA updates.
 *
 * @return ESP_OK on success, or an error code on failure.
 */
esp_err_t ota_spiffs_begin();

/**
 * @brief Write chunk of data to SPIFFS during OTA update.
 *
 * This function writes a chunk of data to SPIFFS during an OTA update.
 *
 * @param data Pointer to the data to write.
 * @param len Length of the data in bytes.
 * @return ESP_OK on success, or an error code on failure.
 */
esp_err_t ota_spiffs_write(char *data, size_t len);

/**
 * @brief End the SPIFFS file system update.
 *
 * This function finalizes the SPIFFS file system update, marking it as complete.
 *
 * @return ESP_OK on success, or an error code on failure.
 */
esp_err_t ota_spiffs_end();

/**
 * @brief Check if the SPIFFS update is in progress.
 *
 * This function checks if a SPIFFS update is currently in progress.
 *
 * @return true if a SPIFFS update is in progress, false otherwise.
 */
void ota_spiffs_abort();

#endif