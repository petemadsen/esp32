#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <esp_log.h>
#include <driver/gpio.h>

#include "sdkconfig.h"

#include "ws2812b.h"

extern void ws2812b_init();

#define min(a,b) ((a)<(b)?(a):(b))
#define max(a,b) ((a)>(b)?(a):(b))

#define CONFIG_BUTTON_PIN 0


SemaphoreHandle_t xButtonSem = NULL;
volatile bool btn_pressed = false;


static inline uint32_t rainbow(float height)
{
    height = max(0.f, min(1.f, height));

    if (height < 0.25f)
        return (0x00 << 16) | ((int)(4. * height * 255.0) << 8) | 0xff;
    else if (height < 0.5)
        return (0x00 << 16) | (0xff << 8) | ((int)((2.0f - 4 * height) * 255.0));
    else if (height < 0.75)
        return ((int)((-2. + 4. * height) * 255.0) << 16) | 0xff00;
    else
        return (0xff << 16) | (((int)((4. - 4. * height) * 255.0)) << 8);
}


static void fill_with_rainbow(struct leds_t* leds)
{
	for (int i=0; i<leds->num_leds; ++i)
		leds->leds[i] = rainbow((float)i/(float)leds->num_leds);
}


static void fill_with_color(struct leds_t* leds, int from, int to, uint32_t color)
{
	for (int i=from; i<=to; ++i)
		leds->leds[i] = color;
}


static void circles(struct leds_t* leds, uint32_t color, int num_run)
{
	// clear first
	fill_with_color(leds, 0, 59, 0x0);

	switch (num_run % 4)
	{
	case 0:
		fill_with_color(leds, 0, 7, color);
		break;
	case 1:
		fill_with_color(leds, 8, 19, color);
		break;
	case 2:
		fill_with_color(leds, 20, 35, color);
		break;
	case 3:
		fill_with_color(leds, 36, 59, color);
		break;
	}
}

static void single_led(struct leds_t* leds, int num_run)
{
	for (int i=0; i<8/*leds->num_leds*/; ++i)
		leds->leds[i] = ((num_run%8)==i ? 0xffffff : 0);
	for (int i=0; i<12/*leds->num_leds*/; ++i)
		leds->leds[8+i] = ((num_run%12)==i ? 0xffffff : 0);
	for (int i=0; i<16/*leds->num_leds*/; ++i)
		leds->leds[20+i] = ((num_run%16)==i ? 0xffffff : 0);
	for (int i=0; i<24/*leds->num_leds*/; ++i)
		leds->leds[36+i] = ((num_run%24)==i ? 0xffffff : 0);
}


static void user_task(void* arg)
{
    struct leds_t* leds = (struct leds_t*)arg;

    uint8_t num_run = 0;

    int type = 0;

	for (;;)
	{
		xSemaphoreTake(leds->sem, portMAX_DELAY);

		switch (type)
		{
		case 0: break;
		case 1: single_led(leds, num_run); break;
		case 2: circles(leds, 0xffffff, num_run); break;
		case 3: circles(leds, 0xff0000, num_run); break;
		case 4: circles(leds, 0x00ff00, num_run); break;
		case 5: circles(leds, 0x0000ff, num_run); break;
		case 6: fill_with_color(leds, 0, 60, 0xffffff); break;
		case 7: fill_with_color(leds, 0, 60, 0xff0000); break;
		case 8: fill_with_color(leds, 0, 60, 0x00ff00); break;
		case 9: fill_with_color(leds, 0, 60, 0x0000ff); break;
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
	leds.leds = malloc(sizeof(uint32_t) * leds.num_leds);
	memset(leds.leds, 0, sizeof(uint32_t) * leds.num_leds);

	ws2812b_init(&leds);

    xTaskCreate(user_task, "user_task", 2048, &leds, 10, NULL);

    xTaskCreate(button_task, "button_task", 2048, NULL, 10, NULL);
}
