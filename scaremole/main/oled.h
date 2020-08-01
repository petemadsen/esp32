/**
 * This code is public domain.
 */
#ifndef PROJECT_OLED_H
#define PROJECT_OLED_H

#include <freertos/FreeRTOS.h>


#define OLED_INVALID_DEVICE 0xff


// Return device ID or 0xff if not found.
uint8_t oled_init();


void oled_clear(uint8_t addr);


void oled_print(int x, int y, const char* text);


void oled_flush(uint8_t addr);


#endif
