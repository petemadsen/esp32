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

extern void ws2812b_init();

#define min(a,b) ((a)<(b)?(a):(b))
#define max(a,b) ((a)>(b)?(a):(b))

#define CONFIG_BUTTON_PIN 0


SemaphoreHandle_t xButtonSem = NULL;
volatile bool btn_pressed = false;


static void user_task(void* arg)
{
#if 0
    struct leds_t* leds = (struct leds_t*)arg;

    uint8_t num_run = 0;

    int type = 0;

	uint32_t blue_and_black[] = { 0x0000ff, 0x000000, 0xffffffff };
	uint32_t black_and_blue[] = { 0x000000, 0x0000ff, 0xffffffff };

	for (;;)
	{
//		fill_one_by_one(leds);
//		continue;

        for (int i=0; i<3; ++i)
        {
			xSemaphoreTake(leds->sem, portMAX_DELAY);
			fill_with_colors(leds, 0, count, blue_and_black);
			xSemaphoreGive(leds->sem);

			vTaskDelay(500 / portTICK_PERIOD_MS);

			xSemaphoreTake(leds->sem, portMAX_DELAY);
			fill_with_colors(leds, 0, count, black_and_blue);
			xSemaphoreGive(leds->sem);

			vTaskDelay(500 / portTICK_PERIOD_MS);
        }

		fill_with_color(leds, 0, count, 0x000000);
        for (int i=0; i<count; ++i)
        {
			xSemaphoreTake(leds->sem, portMAX_DELAY);
			fill_with_color(leds, 0, i, 0x0000ff);
			xSemaphoreGive(leds->sem);

			vTaskDelay(20 / portTICK_PERIOD_MS);
        }

		vTaskDelay(5000 / portTICK_PERIOD_MS);

		fill_with_color(leds, 0, count, 0x000000);
		float h = 1.0;
		int steps = 100;
		float dh = h / (float)steps;
        for (int i=0; i<steps; ++i)
        {
			xSemaphoreTake(leds->sem, portMAX_DELAY);
			fill_with_color(leds, 0, count, rainbow(h));
			xSemaphoreGive(leds->sem);

			h -= dh;
			vTaskDelay(10 / portTICK_PERIOD_MS);
        }

        }

		vTaskDelay(5000 / portTICK_PERIOD_MS);
*/

        vTaskDelay(100 / portTICK_PERIOD_MS);
	}
#endif
	for (;;)
	{
        vTaskDelay(1000 / portTICK_PERIOD_MS);
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
		}
	}
}


void app_main()
{
	dfplayer_init();

    xTaskCreate(user_task, "user_task", 2048, NULL, 10, NULL);

    xTaskCreate(button_task, "button_task", 2048, NULL, 10, NULL);
}
