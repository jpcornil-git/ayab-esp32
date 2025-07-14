#ifndef _RA4M1_FLASH_H
#define _RA4M1_FLASH_H

#include "esp_err.h"
#include "esp_partition.h"

#define RA4M1_FLASH_PAGE_SIZE 2048
#define RA4M1_FLASH_NUM_PAGES  128
#define RA4M1_FLASH_SIZE (RA4M1_FLASH_NUM_PAGES*RA4M1_FLASH_PAGE_SIZE)

/**
 * @brief RA4M1 flash partition structure.
 *
 * This structure holds information about the RA4M1 flash partition.
 * 
 * @return A pointer to the RA4M1 flash partition.
 */
const esp_partition_t *ra4m1_flash_get_partition();

/**
 * @brief Initialize the RA4M1 flash for writing.
 *
 * This function prepares the flash memory for writing by erasing it and allocating a buffer.
 *
 * @return ESP_OK on success, or an error code on failure.
 */
esp_err_t ra4m1_flash_begin();

/**
 * @brief Write data to the RA4M1 flash.
 *
 * This function writes the provided data to the flash memory.
 *
 * @param data Pointer to the data to write.
 * @param len Length of the data in bytes.
 * @return ESP_OK on success, or an error code on failure.
 */
esp_err_t ra4m1_flash_write(char *data, size_t len);

/**
 * @brief Finalize the RA4M1 flash writing process.
 *
 * This function writes any remaining data in the buffer to the flash and cleans up resources.
 *
 * @return ESP_OK on success, or an error code on failure.
 */
esp_err_t ra4m1_flash_end();

/**
 * @brief Abort the RA4M1 flash writing process.
 *
 * This function cleans up resources and disconnects from the flash without writing any data.
 */
void ra4m1_flash_abort();

/**
 * @brief Write a firmware image to the RA4M1 flash.
 *
 * This function writes a firmware image from a file to the RA4M1 flash memory.
 *
 * @param filename Path to the firmware image file.
 * @return ESP_OK on success, or an error code on failure.
 */
esp_err_t ra4m1_flash_image(const char* filename);

#endif