/**
 * This code is public domain.
 */
#ifndef PROJECT_WS2812B_H
#define PROJECT_WS2812B_H


#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <driver/rmt.h>

struct ws2812b_leds_t
{
	SemaphoreHandle_t mutex;
	uint16_t num_leds;
	uint32_t* leds;
};


bool ws2812b_init(struct ws2812b_leds_t* leds, gpio_num_t pin, gpio_num_t power_pin);


void ws2812b_update(void);


void ws2812b_fill(struct ws2812b_leds_t* leds, uint32_t from, uint32_t to, uint32_t color);


void ws2812b_animation(bool b, uint32_t color);


#endif
