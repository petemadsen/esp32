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
    struct leds_t* leds = (struct leds_t*)arg;

    uint8_t num_run = 0;

    int type = 0;

	uint32_t blue_and_black[] = { 0x0000ff, 0x000000, 0xffffffff };
	uint32_t black_and_blue[] = { 0x000000, 0x0000ff, 0xffffffff };

	for (;;)
	{
//		fill_one_by_one(leds);
//		continue;

		int count = 150;
		xSemaphoreTake(leds->sem, portMAX_DELAY);
		fill_with_color(leds, 0, 3, 0xffffff);
		xSemaphoreGive(leds->sem);

        vTaskDelay(2000 / portTICK_PERIOD_MS);

		xSemaphoreTake(leds->sem, portMAX_DELAY);
		fill_with_color(leds, 0, count, 0xff0000);
		xSemaphoreGive(leds->sem);

        vTaskDelay(1000 / portTICK_PERIOD_MS);

		xSemaphoreTake(leds->sem, portMAX_DELAY);
		fill_with_color(leds, 0, count, 0x00ff00);
		xSemaphoreGive(leds->sem);

        vTaskDelay(1000 / portTICK_PERIOD_MS);

		xSemaphoreTake(leds->sem, portMAX_DELAY);
		fill_with_color(leds, 0, count, 0x0000ff);
		xSemaphoreGive(leds->sem);

        vTaskDelay(1000 / portTICK_PERIOD_MS);

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

		vTaskDelay(1000 / portTICK_PERIOD_MS);

		fill_with_color(leds, 0, count, 0x000000);
		for (int k=0; k<6; ++k)
		{
			for (int i=16; i<=64; ++i)
			{
				xSemaphoreTake(leds->sem, portMAX_DELAY);
				fill_with_color(leds, 0, count, (i<<16));
				xSemaphoreGive(leds->sem);

				vTaskDelay(25 / portTICK_PERIOD_MS);
			}
			for (int i=64; i>=16; --i)
			{
				xSemaphoreTake(leds->sem, portMAX_DELAY);
				fill_with_color(leds, 0, count, (i<<16));
				xSemaphoreGive(leds->sem);

				vTaskDelay(25 / portTICK_PERIOD_MS);
			}
		}

		vTaskDelay(5000 / portTICK_PERIOD_MS);

/*
		fill_with_color(leds, 0, count, 0x000000);
        for (int i=0; i<count; ++i)
        {
			xSemaphoreTake(leds->sem, portMAX_DELAY);
			fill_with_color(leds, 0, i, 0xffffff);
			xSemaphoreGive(leds->sem);

			vTaskDelay(50 / portTICK_PERIOD_MS);
        }

		vTaskDelay(5000 / portTICK_PERIOD_MS);
*/

#if 0
		xSemaphoreTake(leds->sem, portMAX_DELAY);

		switch (type)
		{
		case 0: break;
		case 1: single_led(leds, num_run); break;
		case 2: circles(leds, 0xffffff, num_run); break;
		case 3: circles(leds, 0xff0000, num_run); break;
		case 4: circles(leds, 0x00ff00, num_run); break;
		case 5: circles(leds, 0x0000ff, num_run); break;
		case 6: fill_with_color(leds, 0, 2, 0xffffff); break;
		case 7: fill_with_color(leds, 0, 4, 0xff0000); break;
		case 8: fill_with_color(leds, 0, 6, 0x00ff00); break;
		case 9: fill_with_color(leds, 0, 8, 0x0000ff); break;
		case 10: fill_with_rainbow(leds); break;
		default:
			type = 0;
		}

		++num_run;

		if (btn_pressed)
		{
			btn_pressed = false;
			++type;
		}

		xSemaphoreGive(leds->sem);
#endif

        vTaskDelay(100 / portTICK_PERIOD_MS);
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


static struct leds_t leds;
void app_main()
{
	leds.sem = xSemaphoreCreateBinary();
	leds.num_leds = 60;
	leds.num_leds = 150;
	leds.leds = malloc(sizeof(uint32_t) * leds.num_leds);
	memset(leds.leds, 0, sizeof(uint32_t) * leds.num_leds);

	dfplayer_init(&leds);

    xTaskCreate(user_task, "user_task", 2048, &leds, 10, NULL);

    xTaskCreate(button_task, "button_task", 2048, NULL, 10, NULL);
}
