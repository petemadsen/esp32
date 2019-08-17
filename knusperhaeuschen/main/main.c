/**
 * This code is public domain.
 */
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <esp_log.h>
#include <driver/gpio.h>

#include "sdkconfig.h"

#include "dfplayer.h"


#define CONFIG_BELL_BTN_PIN		GPIO_NUM_19
#define CONFIG_LIGHT_BTN_PIN	GPIO_NUM_21
#define CONFIG_RELAY_PIN		GPIO_NUM_18


static const char* MY_TAG = "DFPLAYER/main";


SemaphoreHandle_t xBellBtnSem = NULL;
SemaphoreHandle_t xLightBtnSem = NULL;


#define BTN_DEBOUNCE_DIFF	50


void IRAM_ATTR light_btn_isr_handler(void* arg)
{
	xSemaphoreGiveFromISR(xLightBtnSem, NULL);
}
static void light_btn_task(void* arg)
{
	xLightBtnSem = xSemaphoreCreateBinary();

	gpio_pad_select_gpio(CONFIG_RELAY_PIN);
	gpio_set_direction(CONFIG_RELAY_PIN, GPIO_MODE_OUTPUT);

	gpio_pad_select_gpio(CONFIG_LIGHT_BTN_PIN);
	gpio_set_direction(CONFIG_LIGHT_BTN_PIN, GPIO_MODE_INPUT);
	gpio_set_intr_type(CONFIG_LIGHT_BTN_PIN, GPIO_INTR_NEGEDGE);
//	gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);

	gpio_isr_handler_add(CONFIG_LIGHT_BTN_PIN, light_btn_isr_handler, NULL);

	bool relay_on = false;
	TickType_t last_run = xTaskGetTickCount();

	for (;;)
	{
		if (xSemaphoreTake(xLightBtnSem, portMAX_DELAY) == true)
		{
			TickType_t diff = xTaskGetTickCount() - last_run;
			if (diff > BTN_DEBOUNCE_DIFF)
			{
				ESP_LOGI(MY_TAG, "light-btn");
				last_run = xTaskGetTickCount();

				relay_on = !relay_on;
				gpio_set_level(CONFIG_RELAY_PIN, relay_on);
			}
		}
	}

	vTaskDelete(NULL);
}


void IRAM_ATTR bell_btn_isr_handler(void* arg)
{
	xSemaphoreGiveFromISR(xBellBtnSem, NULL);
}
static void bell_btn_task(void* arg)
{
	xBellBtnSem = xSemaphoreCreateBinary();

	gpio_pad_select_gpio(CONFIG_BELL_BTN_PIN);
	gpio_set_direction(CONFIG_BELL_BTN_PIN, GPIO_MODE_INPUT);
	gpio_set_intr_type(CONFIG_BELL_BTN_PIN, GPIO_INTR_NEGEDGE);
//	gpio_install_isr_service(0); //ESP_INTR_FLAG_DEFAULT
	gpio_isr_handler_add(CONFIG_BELL_BTN_PIN, bell_btn_isr_handler, NULL);

	TickType_t last_run = xTaskGetTickCount();

	for (;;)
	{
		if (xSemaphoreTake(xBellBtnSem, portMAX_DELAY) == true)
		{
			TickType_t diff = xTaskGetTickCount() - last_run;
			if (diff > BTN_DEBOUNCE_DIFF)
			{
				ESP_LOGI(MY_TAG, "bell-btn");
				last_run = xTaskGetTickCount();

				dfplayer_bell();
			}
		}
	}

	vTaskDelete(NULL);
}


void app_main()
{
	gpio_install_isr_service(0);

	dfplayer_init();

    xTaskCreate(light_btn_task, "light_btn_task", 2048, NULL, 10, NULL);

    xTaskCreate(bell_btn_task, "bell_btn_task", 2048, NULL, 10, NULL);
}
