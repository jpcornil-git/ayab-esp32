#ifndef _OTA_APP_H_
#define _OTA_APP_H_
#include "esp_err.h"
#include "esp_partition.h"

const esp_partition_t *ota_app_get_partition();

/**
 * @brief Initialize the OTA update for ESP32 firmware.
 *
 * This function sets up OTA for ESP32, preparing it for firmware updates.
 *
 * @return ESP_OK on success, or an error code on failure.
 */
esp_err_t ota_app_begin();

/**
 * @brief Write chunk of firmware data to flash.
 *
 * This function writes a chunk of data to flash during OTA update.
 *
 * @param data Pointer to the data to write.
 * @param len Length of the data in bytes.
 * @return ESP_OK on success, or an error code on failure.
 */
 esp_err_t ota_app_write(char *data, size_t len);

 /**
 * @brief End the OTA firmware update.
 *
 * This function finalizes the OTA application, marking it as complete.
 *
 * @return ESP_OK on success, or an error code on failure.
 */
 esp_err_t ota_app_end();

 /**
 * @brief Check if the OTA update is in progress.
 *
 * This function checks if an OTA update is currently in progress.
 *
 * @return true if an OTA update is in progress, false otherwise.
 */
 void ota_app_abort();

 /**
 * @brief Validate the ESP32 firmware after OTA update.
 *
 * This function validates the OTA application after an update.
 * It runs diagnostics and either marks the application as valid or rolls back to the previous version.
 *
 * @param diagnostic_is_ok Flag indicating if diagnostics passed successfully.
 */
 void ota_app_validate(bool diagnostic_is_ok);

#endif