/**
 * This code is public domain.
 */
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>

#include <esp_log.h>
#include <driver/gpio.h>

#include "sdkconfig.h"

#include "my_lights.h"
#include "tone.h"
#include "common.h"


static const char* MY_TAG = PROJECT_TAG("buttons");


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
				case PROJECT_BELL_BTN_PIN:
					tone_bell();
					break;
				case PROJECT_LIGHT_BTN_PIN:
					lamp_toggle();
					break;
				}
			}
		}
	}
}


void buttons_init()
{
	gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));

	gpio_pad_select_gpio(PROJECT_BELL_BTN_PIN);
	gpio_set_direction(PROJECT_BELL_BTN_PIN, GPIO_MODE_INPUT);
//	gpio_set_pull_mode(PROJECT_BELL_BTN_PIN, GPIO_PULLUP_ONLY);
	gpio_set_intr_type(PROJECT_BELL_BTN_PIN, GPIO_INTR_NEGEDGE);

	gpio_pad_select_gpio(PROJECT_LIGHT_BTN_PIN);
	gpio_set_direction(PROJECT_LIGHT_BTN_PIN, GPIO_MODE_INPUT);
//	gpio_set_pull_mode(PROJECT_LIGHT_BTN_PIN, GPIO_PULLUP_ONLY);
	gpio_set_intr_type(PROJECT_LIGHT_BTN_PIN, GPIO_INTR_ANYEDGE);

	gpio_install_isr_service(0); //ESP_INTR_FLAG_DEFAULT
	gpio_isr_handler_add(PROJECT_BELL_BTN_PIN, gpio_isr_handler, (void*)PROJECT_BELL_BTN_PIN);
	gpio_isr_handler_add(PROJECT_LIGHT_BTN_PIN, gpio_isr_handler, (void*)PROJECT_LIGHT_BTN_PIN);

	xTaskCreate(gpio_task, "gpio_task", 2048, NULL, 10, NULL);
}
