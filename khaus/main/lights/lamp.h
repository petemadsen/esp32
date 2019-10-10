/**
 * This code is public domain.
 */
#ifndef PROJECT_LAMP_H
#define PROJECT_LAMP_H


#include <freertos/FreeRTOS.h>


void lamp_init(uint8_t relay_pin);

void lamp_set_relay(bool on);

int lamp_get_relay(void);

int64_t lamp_get_on_time();

#endif
