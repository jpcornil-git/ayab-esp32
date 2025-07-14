#include <stdio.h>
#include <string.h>

#include "esp_log.h"

#include "ra4m1_flash.h"
#include "ra4m1_samba.h"

typedef struct {
    const esp_partition_t partition; // Pointer to the partition structure
    size_t buffer_offset; // Offset in the buffer
    size_t flash_offset;  // Offset in the flash memory
    uint32_t bufferSize;  // Size of the buffer used for writing
    uint8_t *buffer;      // Pointer to the buffer used for writing
} ra4m1_flash_data_t;

static const char *TAG = "ra4m1_flash";

static ra4m1_flash_data_t _self = {
    .partition = {
        .size= RA4M1_FLASH_SIZE, 
        .erase_size=RA4M1_FLASH_PAGE_SIZE,
    }, 
};

const esp_partition_t *ra4m1_flash_get_partition() {
    return &_self.partition;
}

esp_err_t ra4m1_flash_begin() {
    _self.buffer_offset = 0;
    _self.flash_offset = 0;
    
    ra4m1_samba_connect();

    ESP_LOGI(TAG, "Programming flash");
    if (ra4m1_samba_erase(0) != ESP_OK) {
        ESP_LOGE(TAG, "Flash erase failed");
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "Flash erased");

    vTaskDelay(100 / portTICK_PERIOD_MS); // Wait for the flash erase to complete

    _self.bufferSize = ra4m1_samba_write_bufferSize();
    _self.buffer = (uint8_t *) malloc(_self.bufferSize);
    if( ! _self.buffer) {
        ESP_LOGE(TAG, "Unable to allocate buffer memory");
        return ESP_ERR_NO_MEM;
    }
    return ESP_OK;
}

esp_err_t _ra4m1_flash_writeBuffer() {
    if (ra4m1_samba_load_buffer(_self.buffer, _self.bufferSize) == ESP_OK) {
        if (ra4m1_samba_write_buffer(_self.flash_offset, _self.bufferSize) != ESP_OK) {
            ESP_LOGE(TAG, "Unable to write buffer to flash");
            return ESP_FAIL;   
        }
    } else {
        ESP_LOGE(TAG, "Unable to load buffer");
        return ESP_FAIL;               
    }
    return ESP_OK;
}

esp_err_t ra4m1_flash_write(char *data, size_t len) {
    // FIXME: Need to check content to make sure it is a correct binary ?
    if ((_self.flash_offset + len) > _self.partition.size) {
        ESP_LOGE(TAG, "Not enough space left on flash");
        return ESP_FAIL;
    }

    size_t data_left = len;
    while ((_self.buffer_offset + data_left) >= _self.bufferSize) {
        size_t data_to_cpy = _self.bufferSize - _self.buffer_offset;
        memcpy(_self.buffer + _self.buffer_offset, data + (len - data_left), data_to_cpy);

        if (_ra4m1_flash_writeBuffer() != ESP_OK) {
            return ESP_FAIL;   
        }

        _self.buffer_offset = 0;
        data_left -= data_to_cpy;
        _self.flash_offset += _self.bufferSize;
    }
    
    memcpy(_self.buffer + _self.buffer_offset, data + (len - data_left), data_left);
    _self.buffer_offset += data_left;

    return ESP_OK;
}

esp_err_t ra4m1_flash_end() {
    esp_err_t err = ESP_OK;
    if (_self.buffer_offset != 0) {
        memset(_self.buffer + _self.buffer_offset, 0, _self.bufferSize - _self.buffer_offset);
        err = _ra4m1_flash_writeBuffer();
    }

    free(_self.buffer);
    ra4m1_samba_disconnect();

    if (err == ESP_OK) {
        ESP_LOGI(TAG, "RA4M1 firmware update succeeded");
    }

    return err;
}

void ra4m1_flash_abort() {
    free(_self.buffer);
    ra4m1_samba_disconnect();
    ESP_LOGI(TAG, "RA4M1 firmware update aborted");
}

esp_err_t ra4m1_flash_image(const char* filename) {
    FILE *fd_image = NULL;
    long image_size;

    esp_err_t err = ra4m1_flash_begin();
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Opening firmware image (%s)", filename);
        fd_image = fopen(filename, "rb");
        if (! fd_image) {
            ESP_LOGE(TAG, "Unable to open firmware image !");
            err = ESP_FAIL;
        } else {
            if (fseek(fd_image, 0, SEEK_END) != 0 || (image_size = ftell(fd_image)) < 0) {
                ESP_LOGE(TAG, "Unable to retrieve image size !");
                err = ESP_FAIL;
            } else {
                if (image_size > RA4M1_FLASH_SIZE) {
                    ESP_LOGW(TAG, "File too large !");
                    err = ESP_FAIL;
                } else {
                    size_t bytes_read;
                    ESP_LOGI(TAG, "About to write %lu bytes to flash (%lu bytes)", (unsigned long) image_size, (unsigned long) RA4M1_FLASH_SIZE);
                    rewind(fd_image);
                    while ((err == ESP_OK) && ((bytes_read = fread(_self.buffer, 1, _self.bufferSize, fd_image)) > 0)) {
                        ESP_LOGI(TAG, "%lu %%", (100 * _self.flash_offset) / image_size);
                        if (bytes_read == _self.bufferSize) {
                            err = _ra4m1_flash_writeBuffer();
                            _self.flash_offset += _self.bufferSize;
                        } else {
                            _self.buffer_offset += bytes_read;
                            ra4m1_flash_end();
                            break;
                        }
                    }
                }
            }
            fclose(fd_image);
        }
    }
    if (err != ESP_OK) {
        ra4m1_flash_abort();
    }
    return err;
}