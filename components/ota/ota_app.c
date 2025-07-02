#include <string.h>

#include <esp_ota_ops.h>
#include <esp_task_wdt.h>
#include "esp_app_format.h"
#include "esp_log.h"

#include "ota_app.h"

static const char *TAG = "ota_app";

static const esp_partition_t *_update_partition;
static esp_ota_handle_t _update_handle;
static bool _image_header_checked;
static size_t _binary_file_length;

const esp_partition_t *ota_app_get_partition() {
  return _update_partition;
}

esp_err_t ota_app_begin() {
  _update_handle = 0;
  _image_header_checked = false;
  _binary_file_length = 0;

  _update_partition = esp_ota_get_next_update_partition(NULL);
  if (_update_partition == NULL) {
    ESP_LOGE(TAG, "Unable to identify an update partition");
    return ESP_FAIL;
  }
  ESP_LOGI(TAG, "Writing to partition subtype %d at offset 0x%"PRIx32,
            _update_partition->subtype, _update_partition->address);

  return ESP_OK;
}


esp_err_t ota_app_write(char *data, size_t len) {
  esp_err_t err = ESP_OK;

  if (_image_header_checked == false) {
      esp_app_desc_t new_app_info;
      if (len > sizeof(esp_image_header_t) + sizeof(esp_image_segment_header_t) + sizeof(esp_app_desc_t)) {
          // check current version with downloading
          memcpy(&new_app_info, &data[sizeof(esp_image_header_t) + sizeof(esp_image_segment_header_t)], sizeof(esp_app_desc_t));
          ESP_LOGI(TAG, "New firmware version: %s", new_app_info.version);

          const esp_partition_t *running = esp_ota_get_running_partition();
          esp_app_desc_t running_app_info;
          if (esp_ota_get_partition_description(running, &running_app_info) == ESP_OK) {
              ESP_LOGI(TAG, "Running firmware version: %s", running_app_info.version);
          }

          const esp_partition_t* last_invalid_app = esp_ota_get_last_invalid_partition();
          esp_app_desc_t invalid_app_info;
          if (esp_ota_get_partition_description(last_invalid_app, &invalid_app_info) == ESP_OK) {
              ESP_LOGI(TAG, "Last invalid firmware version: %s", invalid_app_info.version);
          }

          // check current version with last invalid partition
          if (last_invalid_app != NULL) {
              if (memcmp(invalid_app_info.version, new_app_info.version, sizeof(new_app_info.version)) == 0) {
                  ESP_LOGW(TAG, "New version is the same as invalid version.");
                  ESP_LOGW(TAG, "Previously, there was an attempt to launch the firmware with %s version, but it failed.", invalid_app_info.version);
                  ESP_LOGW(TAG, "The firmware has been rolled back to the previous version.");
                  return ESP_FAIL;
              }
          }

          if (memcmp(new_app_info.version, running_app_info.version, sizeof(new_app_info.version)) == 0) {
              ESP_LOGW(TAG, "Current running version is the same as a new. We will not continue the update.");
              return ESP_FAIL;
          }

          _image_header_checked = true;

          err = esp_ota_begin(_update_partition, OTA_WITH_SEQUENTIAL_WRITES, &_update_handle);
          if (err != ESP_OK) {
              ESP_LOGE(TAG, "esp_ota_begin failed (%s)", esp_err_to_name(err));
              return ESP_FAIL;
          }
      } else {
          ESP_LOGE(TAG, "Not enough data received to check header");
          return ESP_FAIL;
      }
  }

  err = esp_ota_write( _update_handle, (const void *)data, len);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "esp_ota_write failed (%s)", esp_err_to_name(err));
    return ESP_FAIL;
  }

  _binary_file_length += len;
  ESP_LOGD(TAG, "Written image length %d", _binary_file_length);

  return err;
}

esp_err_t ota_app_end() {
  esp_err_t err = ESP_OK;
  if( _update_handle ) {
    err = esp_ota_end(_update_handle);
    _update_handle = 0;
    if (err != ESP_OK) {
      ESP_LOGE(TAG, "esp_ota_end failed (%s)", esp_err_to_name(err));
    } else {
      err = esp_ota_set_boot_partition(_update_partition);
      if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_set_boot_partition failed (%s)", esp_err_to_name(err));
      } else {
        ESP_LOGI(TAG, "ESP32 OTA App update succeeded");
      }
    }
  }
  return err;
}

void ota_app_abort() {
  if( _update_handle ) {
    esp_ota_abort(_update_handle);
    _update_handle = 0;
  }
}

void ota_app_validate(bool diagnostic_is_ok) {
  const esp_partition_t *running = esp_ota_get_running_partition();
  esp_ota_img_states_t ota_state;
  if (esp_ota_get_state_partition(running, &ota_state) == ESP_OK) {
      if (ota_state == ESP_OTA_IMG_PENDING_VERIFY) {
          // run diagnostic function ...
          if (diagnostic_is_ok) {
              ESP_LOGI(TAG, "Diagnostics completed successfully! Continuing execution ...");
              esp_ota_mark_app_valid_cancel_rollback();
          } else {
              ESP_LOGE(TAG, "Diagnostics failed! Start rollback to the previous version ...");
              esp_ota_mark_app_invalid_rollback_and_reboot();
          }
      }
  }
}