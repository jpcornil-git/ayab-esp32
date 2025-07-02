#ifndef _OTA_APP_H_
#define _OTA_APP_H_
#include "esp_err.h"
#include "esp_partition.h"

const esp_partition_t *ota_app_get_partition();

esp_err_t ota_app_begin();
esp_err_t ota_app_write(char *data, size_t len);
esp_err_t ota_app_end();
void ota_app_abort();
void ota_app_validate(bool diagnostic_is_ok);

#endif