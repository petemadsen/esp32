/**
 * This code is public domain.
 */
#ifndef PROJECT_LAMP_H
#define PROJECT_LAMP_H


#include <freertos/FreeRTOS.h>


void lamp_btn_task(void* arg);


void lamp_toggle();


void lamp_off();


void lamp_on();


int lamp_status();


int64_t lamp_on_secs();


#endif
