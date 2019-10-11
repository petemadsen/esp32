/**
 * This code is public domain.
 */
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>

#include <esp_log.h>

#include "my_lights.h"

#include "lights/lamp.h"
#include "lights/ws2812b.h"

#include "common.h"


static const char* MY_TAG = PROJECT_TAG("my_lights");



static EventGroupHandle_t xEvents;
#define LAMP_OFF	1
#define LAMP_ON		2
#define LAMP_TOGGLE	4
#define WS2812B_NEW	8

#define BTN_DEBOUNCE_DIFF	50


void my_lights_task(void* args)
{
	xEvents = xEventGroupCreate();

	lamp_init(PROJECT_LIGHT_RELAY_PIN);

	TickType_t last_run = xTaskGetTickCount();

	struct ws2812b_leds_t leds;
	leds.num_leds = 10;
	leds.leds = malloc(sizeof(uint32_t) * leds.num_leds);
	ws2812b_init(&leds, PROJECT_WS2812B_PIN);

	for (;;)
	{
		EventBits_t bits = xEventGroupWaitBits(
				xEvents,
				LAMP_ON | LAMP_OFF | LAMP_TOGGLE | WS2812B_NEW,
				pdTRUE,		// auto clear
				pdFALSE,	// any single bit will do
				100 /*portMAX_DELAY*/);

		if (bits & LAMP_ON)
		{
			ESP_LOGI(MY_TAG, "lamp-on");
			lamp_set_relay(true);
		}
		else if (bits & LAMP_OFF)
		{
			ESP_LOGI(MY_TAG, "lamp-off");
			lamp_set_relay(false);
		}
		else if (bits & LAMP_TOGGLE)
		{
			TickType_t diff = xTaskGetTickCount() - last_run;
			if (diff > BTN_DEBOUNCE_DIFF)
			{
				last_run = xTaskGetTickCount();
				ESP_LOGI(MY_TAG, "lamp-toggle");
				lamp_set_relay(!lamp_get_relay());
			}
		}

		// FIXME
		uint32_t color = lamp_get_relay() ? 0xff0000 : 0x000000;
		xSemaphoreTake(leds.sem, portMAX_DELAY);
		ws2812b_fill(&leds, 0, leds.num_leds, color);
		xSemaphoreGive(leds.sem);

		if (bits & WS2812B_NEW)
		{
//			ws2812b_update();
		}
	}
}



void lamp_toggle()
{
	xEventGroupSetBits(xEvents, LAMP_TOGGLE);
}

void lamp_off()
{
	xEventGroupSetBits(xEvents, LAMP_OFF);
}

void lamp_on()
{
	xEventGroupSetBits(xEvents, LAMP_ON);
}

int lamp_status()
{
	return lamp_get_relay();
}

int64_t lamp_on_secs()
{
	int64_t relay_on_at = lamp_get_on_time();
	if (relay_on_at == 0)
		return 0;

	int64_t now = esp_timer_get_time() / 1000 / 1000;
	return now - relay_on_at;
}
