/**
 * This code is public domain.
 */
#ifndef PROJECT_ONE_WIRE_H
#define PROJECT_ONE_WIRE_H


#include <driver/gpio.h>

esp_err_t one_wire_reset(gpio_num_t pin);
esp_err_t one_wire_write(gpio_num_t pin, uint8_t data);
esp_err_t one_wire_read(gpio_num_t pin, uint8_t* reply);



#endif
