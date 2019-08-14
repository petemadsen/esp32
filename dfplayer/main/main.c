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


#define CONFIG_BUTTON_PIN 0


SemaphoreHandle_t xButtonSem = NULL;
volatile bool btn_pressed = false;


static void user_task(void* arg)
{
	for (;;)
	{
        vTaskDelay(1000 / portTICK_PERIOD_MS);

		if (btn_pressed)
		{
			printf("--btn_pressed\n");
			btn_pressed = false;
		}
	}
}


void IRAM_ATTR button_isr_handler(void* arg)
{
	xSemaphoreGiveFromISR(xButtonSem, NULL);
}
static void button_task(void* arg)
{
	xButtonSem = xSemaphoreCreateBinary();

	gpio_pad_select_gpio(CONFIG_BUTTON_PIN);
	gpio_set_direction(CONFIG_BUTTON_PIN, GPIO_MODE_INPUT);
	gpio_set_intr_type(CONFIG_BUTTON_PIN, GPIO_INTR_NEGEDGE);
	gpio_install_isr_service(0); //ESP_INTR_FLAG_DEFAULT
	gpio_isr_handler_add(CONFIG_BUTTON_PIN, button_isr_handler, NULL);

	for (;;)
	{
		if (xSemaphoreTake(xButtonSem, portMAX_DELAY) == true)
		{
			btn_pressed = true;
			dfplayer_bell();
		}
	}
	vTaskDelete(NULL);
}


void app_main()
{
	dfplayer_init();

    xTaskCreate(user_task, "user_task", 2048, NULL, 10, NULL);

    xTaskCreate(button_task, "button_task", 2048, NULL, 10, NULL);
}
