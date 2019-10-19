/**
 * This code is public domain.
 */
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>

#include <driver/rmt.h>

#include "ws2812b.h"


static EventGroupHandle_t xEvents;
#define EVENT_RUN		1
#define EVENT_STOP_ANIM	2

static bool m_animation = false;
static uint32_t m_animation_color = 0x0;


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
static void clear(struct ws2812b_leds_t* leds);


static gpio_num_t m_power_pin = 0;


bool ws2812b_init(struct ws2812b_leds_t* leds, gpio_num_t pin, gpio_num_t power_pin)
{
	if (!rmt_init(pin))
		return false;

	xEvents = xEventGroupCreate();

	leds->mutex = xSemaphoreCreateMutex();
	xTaskCreate(tx_task, "ws2812b_tx", 2048, leds, 10, NULL);

	// -- on/off pin
	m_power_pin = power_pin;
	gpio_pad_select_gpio(m_power_pin);
	gpio_set_direction(m_power_pin, GPIO_MODE_OUTPUT);
	gpio_set_level(m_power_pin, 0);

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
	rmt_tx.tx_config.carrier_level = 0;
	rmt_tx.tx_config.idle_level = RMT_IDLE_LEVEL_LOW;
	rmt_tx.tx_config.idle_output_en = false;

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
	esp_err_t err;

	for (;;)
	{
		++nr_run;
		printf("--run %u\n", nr_run);

		EventBits_t bits = xEventGroupWaitBits(xEvents,
											   EVENT_RUN | EVENT_STOP_ANIM,
											   true,
											   false,
											   portMAX_DELAY);

		if (bits & EVENT_RUN)
		{
			// FIXME: actually, this is not required every time.
			xSemaphoreTake(leds->mutex, portMAX_DELAY);
			for (int i=0; i<leds->num_leds; ++i)
				set_led_color(items + i * 24, leds->leds[i]);
			xSemaphoreGive(leds->mutex);

			rmt_write_items(channel, items, item_num, true);
			rmt_wait_tx_done(channel, portMAX_DELAY);
		}

		if (bits & EVENT_STOP_ANIM)
		{
			m_animation = false;

			ws2812b_fill(leds, 0, leds->num_leds, 0x0);
			xEventGroupSetBits(xEvents, EVENT_RUN);
		}

		if (m_animation)
		{
			animation_next(leds, nr_run, m_animation_color);
			vTaskDelay(100 / portTICK_PERIOD_MS);
			xEventGroupSetBits(xEvents, EVENT_RUN);
		}
//		vTaskDelay(1099 / portTICK_PERIOD_MS);
	}
}


void ws2812b_fill(struct ws2812b_leds_t* leds, uint32_t from, uint32_t to, uint32_t color)
{
	xSemaphoreTake(leds->mutex, portMAX_DELAY);
	for (uint32_t i=from; i<to; ++i)
		leds->leds[i] = color;
	xSemaphoreGive(leds->mutex);

	xEventGroupSetBits(xEvents, EVENT_RUN);
}


void ws2812b_animation(bool b, uint32_t color)
{
	if (b != m_animation)
	{
		m_animation = b;
		m_animation_color = color;

		if (!b)
			xEventGroupSetBits(xEvents, EVENT_STOP_ANIM);
		else
			xEventGroupSetBits(xEvents, EVENT_RUN);

		gpio_set_level(m_power_pin, m_animation);
		if (m_animation)
			rmt_tx_start(RMT_TX_CHANNEL, true);
		else
			rmt_tx_stop(RMT_TX_CHANNEL);
	}
}


static void animation_next(struct ws2812b_leds_t* leds, uint32_t run,
						   uint32_t color)
{
	// clear first
	ws2812b_fill(leds, 0, leds->num_leds, 0x0);

	uint16_t rings[] = { 0, 8, 8, 20, 20, 36, 36, 60 };

	int mod = run % 4;
	ws2812b_fill(leds, rings[mod*2], rings[mod*2+1], color);
}
