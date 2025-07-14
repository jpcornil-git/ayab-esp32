#ifndef _RA4M1_CTRL_H_
#define _RA4M1_CTRL_H_

#include "driver/gpio.h"
#include "driver/uart.h"

#define LOW 0
#define HIGH 1

/**
 * @brief Initialize the RA4M1 control module.
 *
 * This function sets up the GPIO pins for reset and boot control.
 *
 * @param resetPin The GPIO pin used for the reset signal.
 * @param bootPin The GPIO pin used for the boot signal.
 */
void ra4m1_ctrl_init(gpio_num_t resetPin, gpio_num_t bootPin);

/**
 * @brief Set the boot pin to a specific level.
 *
 * @param level The level to set the boot pin (LOW or HIGH).
 */
void ra4m1_ctrl_bootPin_set(int level);

/**
 * @brief Set the reset pin to a specific level.
 *
 * @param level The level to set the reset pin (LOW or HIGH).
 */
void ra4m1_ctrl_resetPin_set(int level);

/**
 * @brief Get the current level of the reset pin.
 *
 * @return The current level of the reset pin (LOW or HIGH).
 */
int ra4m1_ctrl_resetPin_get();

/**
 * @brief Toggle the reset pin.
 *
 * This function toggles the state of the reset pin between LOW and HIGH.
 */
void ra4m1_ctrl_reset_toggle();

/**
 * @brief Restart the RA4M1 controller.
 *
 * This function performs a reset of the RA4M1 controller by toggling the reset pin.
 */
void ra4m1_ctrl_restart();

/**
 * @brief Enter programming mode for the RA4M1 controller.
 *
 * This function initiates the double reset sequence to enter programming mode.
 */
void ra4m1_ctrl_enter_programming();

/**
 * @brief Exit programming mode for the RA4M1 controller.
 *
 * This function resets the RA4M1 controller to exit programming mode.
 */
void ra4m1_ctrl_exit_programming();

/**
 * @brief Check if the RA4M1 controller is in programming mode.
 *
 * @return true if in programming mode, false otherwise.
 */
bool ra4m1_ctrl_is_programming();

/**
 * @brief Reset the RA4M1 controller.
 *
 * This function performs a reset by setting the reset pin to LOW and then HIGH.
 */
void ra4m1_ctrl_restart();

#endif