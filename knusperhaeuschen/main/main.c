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
#include "telnet.h"
#include "parser.h"
#include "wifi.h"
#include "light.h"


#define CONFIG_BELL_BTN_PIN		GPIO_NUM_19
#define CONFIG_LIGHT_BTN_PIN	GPIO_NUM_21
#define CONFIG_RELAY_PIN		GPIO_NUM_18


static const char* MY_TAG = "DFPLAYER/main";


SemaphoreHandle_t xBellBtnSem = NULL;


#define BTN_DEBOUNCE_DIFF	50


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

	wifi_init();

	dfplayer_init();

	parser_init();

    xTaskCreate(light_btn_task, "light_btn_task", 2048, NULL, 10, NULL);

    xTaskCreate(bell_btn_task, "bell_btn_task", 1024, NULL, 10, NULL);

	xTaskCreate(telnet_server, "telnet_task", 4096, &parse_input, 10, NULL);
}
