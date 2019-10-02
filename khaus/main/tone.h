/**
 * This code is public domain.
 */
#ifndef PROJECT_TONE_H
#define PROJECT_TONE_H


#include <freertos/FreeRTOS.h>


void tone_init();


void tone_bell();


bool tone_set(int num);


#endif
