#ifndef _RA4M1_CTRL_H_
#define _RA4M1_CTRL_H_

#include "driver/gpio.h"
#include "driver/uart.h"

#define LOW 0
#define HIGH 1

void ra4m1_ctrl_init(gpio_num_t resetPin, gpio_num_t bootPin);

void ra4m1_ctrl_bootPin_set(int level);
void ra4m1_ctrl_resetPin_set(int level);
int ra4m1_ctrl_resetPin_get();
void ra4m1_ctrl_reset_toggle();
void ra4m1_ctrl_restart();

void ra4m1_ctrl_enter_programming();
void ra4m1_ctrl_exit_programming();
bool ra4m1_ctrl_is_programming();

void ra4m1_ctrl_restart();

#endif