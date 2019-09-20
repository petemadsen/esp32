/**
 * This code is public domain.
 */
#ifndef PROJECT_LIGHT_H
#define PROJECT_LIGHT_H


#include <freertos/FreeRTOS.h>


void light_btn_task(void* arg);


void light_toggle();


void light_off();


void light_on();


int light_status();


int64_t light_on_secs();


#endif
