#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <esp_log.h>
#include <driver/gpio.h>

#include "sdkconfig.h"

#include "msgeq7.h"
#include "config.h"

extern void ws2812b_init();

#define min(a,b) ((a)<(b)?(a):(b))
#define max(a,b) ((a)>(b)?(a):(b))

#define CONFIG_BUTTON_PIN 5


SemaphoreHandle_t xButtonSem = NULL;
volatile bool btn_pressed = false;


const char* MY_TAG = "msgeq7";


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

	ESP_LOGE(MY_TAG, "button-task.");
	for (;;)
	{
		if (xSemaphoreTake(xButtonSem, portMAX_DELAY) == true)
		{
			btn_pressed = true;
			ESP_LOGE(MY_TAG, "button-pressed.");
		}
	}
}


static struct leds_t leds;
void app_main()
{
	msgeq7_init();

    xTaskCreate(button_task, "button_task", 2048, NULL, 10, NULL);
}
