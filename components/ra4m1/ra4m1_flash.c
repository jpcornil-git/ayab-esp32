#include <stdio.h>
#include <string.h>

#include "esp_log.h"

#include "ra4m1_flash.h"
#include "ra4m1_samba.h"

static const char *TAG = "ra4m1_flash";

static const esp_partition_t _dummy_partition = {
    .size= RA4M1_FLASH_SIZE, 
    .erase_size=RA4M1_FLASH_PAGE_SIZE,
};

static size_t _buffer_offset;
static size_t _flash_offset;
static uint32_t _bufferSize;
static uint8_t *_buffer;

const esp_partition_t *ra4m1_flash_get_partition() {
    return &_dummy_partition;
}

esp_err_t ra4m1_flash_begin() {
    _buffer_offset = 0;
    _flash_offset = 0;
    
    ra4m1_samba_connect();

    ESP_LOGI(TAG, "Programming flash");
    if (ra4m1_samba_erase(0) != ESP_OK) {
        ESP_LOGE(TAG, "Flash erase failed");
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "Flash erased");

    vTaskDelay(100 / portTICK_PERIOD_MS); // Wait for the flash erase to complete

    _bufferSize = ra4m1_samba_write_bufferSize();
    _buffer = (uint8_t *) malloc(_bufferSize);
    if( ! _buffer) {
        ESP_LOGE(TAG, "Unable to allocate buffer memory");
        return ESP_ERR_NO_MEM;
    }
    return ESP_OK;
}

esp_err_t _ra4m1_flash_writeBuffer() {
    if (ra4m1_samba_load_buffer(_buffer, _bufferSize) == ESP_OK) {
        if (ra4m1_samba_write_buffer(_flash_offset, _bufferSize) != ESP_OK) {
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
    if ((_flash_offset + len) > _dummy_partition.size) {
        ESP_LOGE(TAG, "Not enough space left on flash");
        return ESP_FAIL;
    }

    size_t data_left = len;
    while ((_buffer_offset + data_left) >= _bufferSize) {
        size_t data_to_cpy = _bufferSize - _buffer_offset;
        memcpy(_buffer + _buffer_offset, data + (len - data_left), data_to_cpy);

        if (_ra4m1_flash_writeBuffer() != ESP_OK) {
            return ESP_FAIL;   
        }

        _buffer_offset = 0;
        data_left -= data_to_cpy;
        _flash_offset += _bufferSize;
    }
    
    memcpy(_buffer + _buffer_offset, data + (len - data_left), data_left);
    _buffer_offset += data_left;

    return ESP_OK;
}

esp_err_t ra4m1_flash_end() {
    esp_err_t err = ESP_OK;
    if (_buffer_offset != 0) {
        memset(_buffer + _buffer_offset, 0, _bufferSize - _buffer_offset);
        err = _ra4m1_flash_writeBuffer();
    }

    free(_buffer);
    ra4m1_samba_disconnect();

    if (err == ESP_OK) {
        ESP_LOGI(TAG, "RA4M1 firmware update succeeded");
    }

    return err;
}

void ra4m1_flash_abort() {
    free(_buffer);
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
                    while ((err == ESP_OK) && ((bytes_read = fread(_buffer, 1, _bufferSize, fd_image)) > 0)) {
                        ESP_LOGI(TAG, "%lu %%", (100 * _flash_offset) / image_size);
                        if (bytes_read == _bufferSize) {
                            err = _ra4m1_flash_writeBuffer();
                            _flash_offset += _bufferSize;
                        } else {
                            _buffer_offset += bytes_read;
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