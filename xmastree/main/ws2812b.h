/*
 * ws2812b.h
 */
#ifndef MAIN_WS2812B_H_
#define MAIN_WS2812B_H_


#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include "freertos/task.h"
#include "freertos/queue.h"

struct leds_t
{
	SemaphoreHandle_t sem;
	uint16_t num_leds;
	uint32_t* leds;
//	rmt_item32_t* rmt_items;
};


void ws2812b_init(struct leds_t* leds);


#endif /* MAIN_WS2812B_H_ */
