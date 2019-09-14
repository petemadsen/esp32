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

		TickType_t diff = xTaskGetTickCount() - last_run;
		bool ok_to_run = (diff > BTN_DEBOUNCE_DIFF);

		if (bits & LIGHT_ON)
		{
			ESP_LOGI(MY_TAG, "light-on");
			relay_on = true;
			gpio_set_level(PROJECT_LIGHT_RELAY_PIN, relay_on);
		}
		else if (bits & LIGHT_OFF)
		{
			ESP_LOGI(MY_TAG, "light-off");
			relay_on = false;
			gpio_set_level(PROJECT_LIGHT_RELAY_PIN, relay_on);
		}
		else if (bits & LIGHT_TOGGLE)
		{
			if (ok_to_run)
			{
				last_run = xTaskGetTickCount();
				ESP_LOGI(MY_TAG, "light-toggle");
				relay_on = !relay_on;
				gpio_set_level(PROJECT_LIGHT_RELAY_PIN, relay_on);
			}
		}
	}

	vTaskDelete(NULL);
}


void light_on()
{
	xEventGroupSetBits(xLightEvents, LIGHT_ON);
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
