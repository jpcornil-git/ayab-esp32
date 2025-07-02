#include <string.h>

#include "esp_log.h"

#include "srv_spiffs.h"
#include "ota_spiffs.h"

static const char *TAG = "ota_spiffs";

static const esp_partition_t *_update_partition;
static char *_buffer;
static size_t _buffer_offset;
static size_t _partition_offset;

const esp_partition_t *ota_spiffs_get_partition() {
  return _update_partition;
}

esp_err_t ota_spiffs_begin() {
    _buffer_offset = 0;
    _partition_offset = 0;
    _update_partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_SPIFFS, NULL);
    if( ! _update_partition) {
        ESP_LOGE(TAG, "No SPIFFS partion found !");
        return ESP_FAIL;
    }

    _buffer = (char *) malloc(_update_partition->erase_size);
    if( ! _buffer) {
        ESP_LOGE(TAG, "Unable to allocate buffer memory !");
        return ESP_FAIL;
    }

    srv_spiffs_stop();

    ESP_LOGI(TAG, "Writing to partition subtype %d at offset 0x%"PRIx32"(size = 0x%"PRIx32")",
            _update_partition->subtype, _update_partition->address, _update_partition->size);

    return ESP_OK;
}

esp_err_t _ota_spiffs_writeBuffer(size_t len) {
    esp_err_t err = ESP_OK;

    err = esp_partition_erase_range(_update_partition, _partition_offset, _update_partition->erase_size);
    if (err != ESP_OK) {
            ESP_LOGE(TAG, "Unable to erase spiffs sector @ 0x%08x (%s)", _partition_offset, esp_err_to_name(err));
            return ESP_FAIL;            
    }

    err = esp_partition_write(_update_partition, _partition_offset, _buffer, len);
    if (err != ESP_OK) {
            ESP_LOGE(TAG, "Unable to write spiffs sector @ 0x%08x (%s)", _partition_offset, esp_err_to_name(err));
            return ESP_FAIL;            
    }
    //ESP_LOGI(TAG, "Wrote %d bytes at 0x%08x", len , _partition_offset);
    return err;
}

// TODO: Add support for compressed FS ?
esp_err_t ota_spiffs_write(char *data, size_t len) {
    // FIXME: Need to check content to make sure it is a spiffs binary
    if ((_partition_offset + len) > _update_partition->size) {
        ESP_LOGE(TAG, "Not enough space left on flash!");
        return ESP_FAIL;
    }

    size_t data_left = len;
    while ((_buffer_offset + data_left) >= _update_partition->erase_size) {
        size_t data_to_cpy = _update_partition->erase_size - _buffer_offset;
        memcpy(_buffer + _buffer_offset, data + (len - data_left), data_to_cpy);

        if (_ota_spiffs_writeBuffer(_update_partition->erase_size) != ESP_OK) {
            return ESP_FAIL;   
        }

        _buffer_offset = 0;
        data_left -= data_to_cpy;
        _partition_offset +=_update_partition->erase_size;
    }
    
    memcpy(_buffer + _buffer_offset, data + (len - data_left), data_left);
    _buffer_offset += data_left;

    return ESP_OK;
}

esp_err_t ota_spiffs_end() {
    esp_err_t err = ESP_OK;

    if (_buffer_offset != 0) {
        err = _ota_spiffs_writeBuffer(_buffer_offset);
    }

    free(_buffer);
    ESP_LOGI(TAG, "OTA spiffs update succeeded");

    srv_spiffs_restart();
    
    return err;
}

void ota_spiffs_abort() {
    free(_buffer);
    ESP_LOGI(TAG, "OTA spiffs update aborted");
}