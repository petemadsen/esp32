/**
 * This code is public domain.
 */
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>
#include <freertos/semphr.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <esp_log.h>
#include <driver/gpio.h>

#include "sdkconfig.h"

#include "light.h"
#include "common.h"



static const char* MY_TAG = "khaus/light";


static bool relay_on = false;
static int64_t relay_on_at = 0; // time in secs from boot when relay was turned on


static void set_relay(bool on);

static EventGroupHandle_t xLightEvents;
#define LIGHT_OFF		(1 << 0)
#define LIGHT_ON		(1 << 1)
#define LIGHT_TOGGLE	(1 << 2)


#define BTN_DEBOUNCE_DIFF	50


void light_btn_task(void* arg)
{
	xLightEvents = xEventGroupCreate();

	gpio_pad_select_gpio(PROJECT_LIGHT_RELAY_PIN);
	gpio_set_direction(PROJECT_LIGHT_RELAY_PIN, GPIO_MODE_OUTPUT);

	TickType_t last_run = xTaskGetTickCount();
	light_off();

	for (;;)
	{
		EventBits_t bits = xEventGroupWaitBits(
				xLightEvents,
				LIGHT_ON | LIGHT_OFF | LIGHT_TOGGLE,
				pdTRUE,		// auto clear
				pdFALSE,	// any single bit will do
				100 /*portMAX_DELAY*/);

		if (bits & LIGHT_ON)
		{
			ESP_LOGI(MY_TAG, "light-on");
			set_relay(true);
		}
		else if (bits & LIGHT_OFF)
		{
			ESP_LOGI(MY_TAG, "light-off");
			set_relay(false);
		}
		else if (bits & LIGHT_TOGGLE)
		{
			TickType_t diff = xTaskGetTickCount() - last_run;
			if (diff > BTN_DEBOUNCE_DIFF)
			{
				last_run = xTaskGetTickCount();
				ESP_LOGI(MY_TAG, "light-toggle");
				set_relay(!relay_on);
			}
		}
	}
}


void set_relay(bool on)
{
	relay_on = on;

	if (relay_on)
		relay_on_at = esp_timer_get_time() / 1000 / 1000;
	else
		relay_on_at = 0;

	gpio_set_level(PROJECT_LIGHT_RELAY_PIN, relay_on);
}


void light_on()
{
	xEventGroupSetBits(xLightEvents, LIGHT_ON);
}


int64_t light_on_secs()
{
	if (relay_on_at == 0)
		return 0;

	int64_t now = esp_timer_get_time() / 1000 / 1000;
	return now - relay_on_at;
}


void light_off()
{
	xEventGroupSetBits(xLightEvents, LIGHT_OFF);
}


void light_toggle()
{
	xEventGroupSetBits(xLightEvents, LIGHT_TOGGLE);
}


int light_status()
{
	return relay_on;
}
