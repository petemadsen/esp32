/**
 * This code is public domain.
 */
#ifndef PROJECT_TONE_H
#define PROJECT_TONE_H


#include <freertos/FreeRTOS.h>


void tone_init(void);


void tone_bell(void);


bool tone_set(int num);


int tone_get(void);


void tone_set_volume_p(int level);


int tone_get_volume_p(void);


#endif
