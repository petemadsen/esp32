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

#include "light.h"
#include "dfplayer.h"


#define CONFIG_BELL_BTN_PIN		GPIO_NUM_19
#define CONFIG_LIGHT_BTN_PIN	GPIO_NUM_21


static const char* MY_TAG = "knusperhaeuschen/main";


#define BTN_DEBOUNCE_DIFF	50


static xQueueHandle gpio_evt_queue = NULL;
static void IRAM_ATTR gpio_isr_handler(void* arg)
{
	uint32_t gpio_num = (uint32_t)arg;
	xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}


static void gpio_task(void* arg)
{
	uint32_t iopin;
	TickType_t last_run = xTaskGetTickCount();

	for (;;)
	{
		if (xQueueReceive(gpio_evt_queue, &iopin, portMAX_DELAY))
		{
			TickType_t diff = xTaskGetTickCount() - last_run;
			if (diff > BTN_DEBOUNCE_DIFF)
			{
				ESP_LOGI(MY_TAG, "button %d", iopin);
				last_run = xTaskGetTickCount();

				switch (iopin)
				{
				case CONFIG_BELL_BTN_PIN:
					dfplayer_bell();
					break;
				case CONFIG_LIGHT_BTN_PIN:
					light_toggle();
					break;
				}
			}
		}
	}
}


void buttons_init()
{
	gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));

	gpio_pad_select_gpio(CONFIG_BELL_BTN_PIN);
	gpio_set_direction(CONFIG_BELL_BTN_PIN, GPIO_MODE_INPUT);
	gpio_set_intr_type(CONFIG_BELL_BTN_PIN, GPIO_INTR_NEGEDGE);

	gpio_pad_select_gpio(CONFIG_LIGHT_BTN_PIN);
	gpio_set_direction(CONFIG_LIGHT_BTN_PIN, GPIO_MODE_INPUT);
	gpio_set_intr_type(CONFIG_LIGHT_BTN_PIN, GPIO_INTR_ANYEDGE);

	
	gpio_install_isr_service(0); //ESP_INTR_FLAG_DEFAULT
//	gpio_isr_handler_add(CONFIG_BELL_BTN_PIN, bell_btn_isr_handler, NULL);
	gpio_isr_handler_add(CONFIG_BELL_BTN_PIN, gpio_isr_handler, (void*)CONFIG_BELL_BTN_PIN);
//	gpio_isr_handler_add(CONFIG_LIGHT_BTN_PIN, light_btn_isr_handler, NULL);
	gpio_isr_handler_add(CONFIG_LIGHT_BTN_PIN, gpio_isr_handler, (void*)CONFIG_LIGHT_BTN_PIN);

	xTaskCreate(gpio_task, "gpio_task", 2048, NULL, 10, NULL);
}
