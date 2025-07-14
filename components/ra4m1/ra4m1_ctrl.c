#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "esp_log.h"
#include "ra4m1_ctrl.h"

typedef struct {
    gpio_num_t resetPin;  /*!< GPIO pin for reset */
    gpio_num_t bootPin;   /*!< GPIO pin for boot mode */
    bool program_mode; /*!< Flag indicating if the device is in programming mode */
} ra4m1_ctrl_data_t;

static const char *TAG = "ra4m1_device";

static ra4m1_ctrl_data_t _self;

void ra4m1_ctrl_init(gpio_num_t resetPin, gpio_num_t bootPin) {
    _self.resetPin = resetPin;
    _self.bootPin  = bootPin;
    _self.program_mode = false;

    // Define startup levels
    ra4m1_ctrl_bootPin_set(HIGH);
    ra4m1_ctrl_resetPin_set(LOW);

    //configure GPIO for RA4M1
    gpio_config_t pin_config = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_INPUT_OUTPUT,
        .pin_bit_mask = ((1ULL<<resetPin) | (1ULL<<bootPin)),
        .pull_down_en = 0,
        .pull_up_en = 0,
    };
    gpio_config(&pin_config);    
}

void ra4m1_ctrl_bootPin_set(int level) {
    gpio_set_level(_self.bootPin, level);
}

void ra4m1_ctrl_resetPin_set(int level) {
    gpio_set_level(_self.resetPin, level);
}

int ra4m1_ctrl_resetPin_get() {
    return gpio_get_level(_self.resetPin);
}

void ra4m1_ctrl_reset_toggle() {
    ra4m1_ctrl_resetPin_set(gpio_get_level(_self.resetPin) ? 0 : 1);
}

void ra4m1_ctrl_restart() {
    ESP_LOGI(TAG, "Reset MCU");
    ra4m1_ctrl_resetPin_set(LOW);
    vTaskDelay(100 / portTICK_PERIOD_MS);
    ra4m1_ctrl_resetPin_set(HIGH);
}

void ra4m1_ctrl_enter_programming() {
    ESP_LOGI(TAG, "Enter programming mode");
    _self.program_mode = true;
    // Double reset sequence tp enter programming mode
    ra4m1_ctrl_restart();
    vTaskDelay(100 / portTICK_PERIOD_MS);
    ra4m1_ctrl_restart();
}

void ra4m1_ctrl_exit_programming() {
    ESP_LOGI(TAG, "Exit programming mode");
    ra4m1_ctrl_restart();
    _self.program_mode = false;
}

bool ra4m1_ctrl_is_programming() {
    return _self.program_mode;
}