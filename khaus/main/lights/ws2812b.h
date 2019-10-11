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
	SemaphoreHandle_t sem;
	uint16_t num_leds;
	uint32_t* leds;
//  rmt_item32_t* rmt_items;
};


bool ws2812b_init(struct ws2812b_leds_t* leds, gpio_num_t pin);


void ws2812b_update();


void ws2812b_fill(struct ws2812b_leds_t* leds, int from, int to, uint32_t color);


#endif
