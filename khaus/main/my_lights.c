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
#define EVENT_LAMP_OFF	1
#define EVENT_LAMP_ON		2
#define EVENT_LAMP_TOGGLE	4
#define EVENT_WS2812B_NEW	8

#define BTN_DEBOUNCE_DIFF	50


void my_lights_task(void* args)
{
	xEvents = xEventGroupCreate();

	lamp_init(PROJECT_LIGHT_RELAY_PIN);

	TickType_t last_run = xTaskGetTickCount();

	struct ws2812b_leds_t leds;
	leds.num_leds = 60;
	leds.leds = malloc(sizeof(uint32_t) * leds.num_leds);
	bool ok = ws2812b_init(&leds, PROJECT_WS2812B_PIN);
	ESP_LOGI(MY_TAG, "ws2812b: %d", ok);

	lamp_off();

	for (;;)
	{
		EventBits_t bits = xEventGroupWaitBits(
				xEvents,
				EVENT_LAMP_ON | EVENT_LAMP_OFF | EVENT_LAMP_TOGGLE | EVENT_WS2812B_NEW,
				pdTRUE,		// auto clear
				pdFALSE,	// any single bit will do
				portMAX_DELAY);
		printf("--wait: 0x%x\n", bits);

		if (bits & EVENT_LAMP_ON)
		{
			ESP_LOGI(MY_TAG, "lamp-on");
			lamp_set_relay(true);
		}
		else if (bits & EVENT_LAMP_OFF)
		{
			ESP_LOGI(MY_TAG, "lamp-off");
			lamp_set_relay(false);
		}
		else if (bits & EVENT_LAMP_TOGGLE)
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
		uint32_t color = lamp_get_relay() ? 0x1f0000 : 0x000000;
		ws2812b_animation(lamp_get_relay());
		ws2812b_fill(&leds, 0, leds.num_leds, color);

		if (bits & EVENT_WS2812B_NEW)
		{
//			ws2812b_update();
		}
	}
}



void lamp_toggle()
{
	xEventGroupSetBits(xEvents, EVENT_LAMP_TOGGLE);
}

void lamp_off()
{
	xEventGroupSetBits(xEvents, EVENT_LAMP_OFF);
}

void lamp_on()
{
	xEventGroupSetBits(xEvents, EVENT_LAMP_ON);
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
