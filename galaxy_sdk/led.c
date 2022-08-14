/**
 * @file led.c
 * @author tkyzp
 * @brief led灯控制
 * @version 0.1
 * @date 2022-08-08
 * 
 * @copyright Copyright (c) 2022
 * 
 */
#include "led.h"
#include "hal_gpio.h"

static GpioDevice led = {
    .group = 0,
    .irq_reload = 0,
    .invert = 1,
    .port = 19,
    .type = IO_MODE_OUT_PULL,
};

int led_init(){
    return hal_gpio_init(&led);
}

int led_on(){
    return hal_gpio_output_high(&led);
}

int led_off(){
    return hal_gpio_output_low(&led);
}