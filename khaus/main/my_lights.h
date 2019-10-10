/**
 * This code is public domain.
 */
#ifndef PROJECT_LIGHTS_H
#define PROJECT_LIGHTS_H


#include <freertos/FreeRTOS.h>


void my_lights_task(void* args);


void lamp_toggle();
void lamp_off();
void lamp_on();
int lamp_status();
int64_t lamp_on_secs();


#endif
