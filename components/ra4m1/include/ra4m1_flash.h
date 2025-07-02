#ifndef _RA4M1_FLASH_H
#define _RA4M1_FLASH_H

#include "esp_err.h"
#include "esp_partition.h"

#define RA4M1_FLASH_PAGE_SIZE 2048
#define RA4M1_FLASH_NUM_PAGES  128
#define RA4M1_FLASH_SIZE (RA4M1_FLASH_NUM_PAGES*RA4M1_FLASH_PAGE_SIZE)

const esp_partition_t *ra4m1_flash_get_partition();

esp_err_t ra4m1_flash_begin();
esp_err_t ra4m1_flash_write(char *data, size_t len);
esp_err_t ra4m1_flash_end();
void ra4m1_flash_abort();

esp_err_t ra4m1_flash_image(const char* filename);

#endif