/**
 * This code is public domain.
 */
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>

#include <driver/rmt.h>

#include "ws2812b.h"


static EventGroupHandle_t xEvents;
#define EVENT_RUN	1

static bool m_animation = false;


#define RMT_TX_CHANNEL   1    // RMT channel for transmitter
#define RMT_CLK_DIV      8    // RMT clock divider => 10MHz => 100ns


// WS2812 timings, multitude of 100ns, see RMT_CLK_DIV
#define WS2812B_BIT_1_HIGH  9 // 900ns (900ns +/- 150ns)
#define WS2812B_BIT_1_LOW   3 // 300ns (350ns +/- 150ns)
#define WS2812B_BIT_0_HIGH  3 // 300ns (350ns +/- 150ns)
#define WS2812B_BIT_0_LOW   9 // 900ns (900ns +/- 150ns)


static void tx_task(void* args);
static bool rmt_init(gpio_num_t pin);
static void animation_next(struct ws2812b_leds_t* leds, uint32_t run,
						   uint32_t color);



bool ws2812b_init(struct ws2812b_leds_t* leds, gpio_num_t pin)
{
	if (!rmt_init(pin))
		return false;

	xEvents = xEventGroupCreate();

	leds->mutex = xSemaphoreCreateMutex();
	xTaskCreate(tx_task, "ws2812b_tx", 2048, leds, 10, NULL);

	return true;
}

/*
 * @brief RMT transmitter initialization
 */
static bool rmt_init(gpio_num_t pin)
{
	rmt_config_t rmt_tx;

	rmt_tx.rmt_mode = RMT_MODE_TX;
	rmt_tx.channel = RMT_TX_CHANNEL;
	rmt_tx.gpio_num = pin;
	rmt_tx.mem_block_num = 1;
	rmt_tx.clk_div = RMT_CLK_DIV;

	// don't need but must be initialized!
	rmt_tx.tx_config.loop_en = false;
	rmt_tx.tx_config.carrier_en = false;
	rmt_tx.tx_config.carrier_duty_percent = 50;
	rmt_tx.tx_config.carrier_freq_hz = 38000;
	rmt_tx.tx_config.carrier_level = 1;
	rmt_tx.tx_config.idle_level = RMT_IDLE_LEVEL_LOW;
	rmt_tx.tx_config.idle_output_en = true;

	esp_err_t err = rmt_config(&rmt_tx);
	if (err != ESP_OK)
		return false;

	err = rmt_driver_install(rmt_tx.channel, 0, 0);
	if (err != ESP_OK)
		return false;

	return true;
}




static inline void set_led_color(rmt_item32_t* item, uint32_t rgb)
{
	uint32_t grb = ((rgb & 0xff0000) >> 8) | ((rgb & 0xff00) << 8) | (rgb & 0xff);

	for (uint32_t m=(1<<23); m!=0; m>>=1)
	{
		item->level0 = 1;
		item->duration0 = (m & grb ? WS2812B_BIT_1_HIGH : WS2812B_BIT_0_HIGH);
		item->level1 = 0;
		item->duration1 = (m & grb ? WS2812B_BIT_1_LOW : WS2812B_BIT_0_LOW);

		++item;
	}
}


static void tx_task(void* args)
{
	struct ws2812b_leds_t* leds = (struct ws2812b_leds_t*)args;

	int item_num = leds->num_leds * 24;
	rmt_item32_t* items = malloc(sizeof(rmt_item32_t) * item_num);

	rmt_channel_t channel = RMT_TX_CHANNEL;

	uint32_t nr_run = 0;

	for (;;)
	{
		++nr_run;
		printf("--run %u\n", nr_run);

		xEventGroupWaitBits(xEvents, EVENT_RUN, true, false, portMAX_DELAY);

		xSemaphoreTake(leds->mutex, portMAX_DELAY);

		for (int i=0; i<leds->num_leds; ++i)
			set_led_color(items + i * 24, leds->leds[i]);

		rmt_write_items(channel, items, item_num, true);
		rmt_wait_tx_done(channel, portMAX_DELAY);

		xSemaphoreGive(leds->mutex);

		if (m_animation)
		{
			animation_next(leds, nr_run, 0x1f0000);
			vTaskDelay(100 / portTICK_PERIOD_MS);
			xEventGroupSetBits(xEvents, EVENT_RUN);
		}
		else
		{
			// stop animation
			ws2812b_fill(leds, 0, leds->num_leds, 0x0);
			xEventGroupSetBits(xEvents, EVENT_RUN);
		}
	}
}


void ws2812b_fill(struct ws2812b_leds_t* leds, int from, int to, uint32_t color)
{
	xSemaphoreTake(leds->mutex, portMAX_DELAY);
	for (int i=from; i<to; ++i)
		leds->leds[i] = color;
	xSemaphoreGive(leds->mutex);

	xEventGroupSetBits(xEvents, EVENT_RUN);
}


void ws2812b_animation(bool b)
{
	if (b != m_animation)
	{
		m_animation = b;
		xEventGroupSetBits(xEvents, EVENT_RUN);
	}
}


static void animation_next(struct ws2812b_leds_t* leds, uint32_t run,
						   uint32_t color)
{
	// clear first
	ws2812b_fill(leds, 0, leds->num_leds, 0x0);

	switch (run % 4)
	{
	case 0:
		ws2812b_fill(leds, 0, 8, color);
		break;
	case 1:
		ws2812b_fill(leds, 8, 20, color);
		break;
	case 2:
		ws2812b_fill(leds, 20, 36, color);
		break;
	case 3:
		ws2812b_fill(leds, 36, 60, color);
		break;
	}
}
