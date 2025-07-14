#include <string.h>

#include "esp_log.h"

#include "srv_spiffs.h"
#include "ota_spiffs.h"

typedef struct {
    const esp_partition_t* partition;  /*!< Pointer to the SPIFFS partition */
    size_t partition_offset;           /*!< Current offset in the partition */
    char* buffer;                      /*!< Buffer for writing data */
    size_t buffer_offset;              /*!< Current offset in the buffer */
} ota_spiffs_data_t;

static const char *TAG = "ota_spiffs";

static ota_spiffs_data_t _self;

const esp_partition_t *ota_spiffs_get_partition() {
  return _self.partition;
}

esp_err_t ota_spiffs_begin() {
    _self.buffer_offset = 0;
    _self.partition_offset = 0;
    _self.partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_SPIFFS, NULL);
    if( ! _self.partition) {
        ESP_LOGE(TAG, "No SPIFFS partion found !");
        return ESP_FAIL;
    }

    _self.buffer = (char *) malloc(_self.partition->erase_size);
    if( ! _self.buffer) {
        ESP_LOGE(TAG, "Unable to allocate buffer memory !");
        return ESP_FAIL;
    }

    srv_spiffs_stop();

    ESP_LOGI(TAG, "Writing to partition subtype %d at offset 0x%"PRIx32"(size = 0x%"PRIx32")",
            _self.partition->subtype, _self.partition->address, _self.partition->size);

    return ESP_OK;
}

esp_err_t _ota_spiffs_writeBuffer(size_t len) {
    esp_err_t err = ESP_OK;

    err = esp_partition_erase_range(_self.partition, _self.partition_offset, _self.partition->erase_size);
    if (err != ESP_OK) {
            ESP_LOGE(TAG, "Unable to erase spiffs sector @ 0x%08x (%s)", _self.partition_offset, esp_err_to_name(err));
            return ESP_FAIL;            
    }

    err = esp_partition_write(_self.partition, _self.partition_offset, _self.buffer, len);
    if (err != ESP_OK) {
            ESP_LOGE(TAG, "Unable to write spiffs sector @ 0x%08x (%s)", _self.partition_offset, esp_err_to_name(err));
            return ESP_FAIL;            
    }
    //ESP_LOGI(TAG, "Wrote %d bytes at 0x%08x", len , _self.partition_offset);
    return err;
}

// TODO: Add support for compressed FS ?
esp_err_t ota_spiffs_write(char *data, size_t len) {
    // FIXME: Need to check content to make sure it is a spiffs binary
    if ((_self.partition_offset + len) > _self.partition->size) {
        ESP_LOGE(TAG, "Not enough space left on flash!");
        return ESP_FAIL;
    }

    size_t data_left = len;
    while ((_self.buffer_offset + data_left) >= _self.partition->erase_size) {
        size_t data_to_cpy = _self.partition->erase_size - _self.buffer_offset;
        memcpy(_self.buffer + _self.buffer_offset, data + (len - data_left), data_to_cpy);

        if (_ota_spiffs_writeBuffer(_self.partition->erase_size) != ESP_OK) {
            return ESP_FAIL;   
        }

        _self.buffer_offset = 0;
        data_left -= data_to_cpy;
        _self.partition_offset +=_self.partition->erase_size;
    }
    
    memcpy(_self.buffer + _self.buffer_offset, data + (len - data_left), data_left);
    _self.buffer_offset += data_left;

    return ESP_OK;
}

esp_err_t ota_spiffs_end() {
    esp_err_t err = ESP_OK;

    if (_self.buffer_offset != 0) {
        err = _ota_spiffs_writeBuffer(_self.buffer_offset);
    }

    free(_self.buffer);
    ESP_LOGI(TAG, "OTA spiffs update succeeded");

    srv_spiffs_restart();
    
    return err;
}

void ota_spiffs_abort() {
    free(_self.buffer);
    ESP_LOGI(TAG, "OTA spiffs update aborted");
}