#ifndef _OTA_SPIFFS_H_
#define _OTA_SPIFFS_H_
#include "esp_err.h"
#include "esp_partition.h"

const esp_partition_t *ota_spiffs_get_partition();

esp_err_t ota_spiffs_begin();
esp_err_t ota_spiffs_write(char *data, size_t len);
esp_err_t ota_spiffs_end();
void ota_spiffs_abort();

#endif