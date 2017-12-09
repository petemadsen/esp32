/**
 * ws2812b.c
 */
#include "ws2812b.h"

#include <stdio.h>
#include <string.h>
#include "esp_err.h"
#include "esp_log.h"
#include "driver/rmt.h"
#include "driver/periph_ctrl.h"
#include "soc/rmt_reg.h"

#include "sdkconfig.h"

static const char* MY_TAG = "WS2812B";


#define RMT_TX_CHANNEL    1     // RMT channel for transmitter
#define RMT_TX_GPIO_NUM  18     // GPIO number for transmitter signal
#define RMT_CLK_DIV      8    // RMT clock divider => 10MHz => 100ns


// WS2812 timings, multitude of 100ns, see RMT_CLK_DIV
#define WS2812B_BIT_1_HIGH	9 // 900ns (900ns +/- 150ns)
#define WS2812B_BIT_1_LOW	3 // 300ns (350ns +/- 150ns)
#define WS2812B_BIT_0_HIGH	3 // 300ns (350ns +/- 150ns)
#define WS2812B_BIT_0_LOW	9 // 900ns (900ns +/- 150ns)


/**
 *
 */
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


/*
 * @brief RMT transmitter initialization
 */
static bool init_rmt()
{
    rmt_config_t rmt_tx;

    rmt_tx.rmt_mode = RMT_MODE_TX;
    rmt_tx.channel = RMT_TX_CHANNEL;
    rmt_tx.gpio_num = RMT_TX_GPIO_NUM;
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


/**
 * @brief RMT transmitter demo, this task will periodically send NEC data. (100 * 32 bits each time.)
 *
 */
static void tx_task(void* arg)
{
    vTaskDelay(10);

    if (!init_rmt())
    {
    	ESP_LOGE(MY_TAG, "RMT init failed.");
    	return;
    }

    struct leds_t* leds = (struct leds_t*)arg;
	xSemaphoreGive(leds->sem);

    int channel = RMT_TX_CHANNEL;

	int item_num = leds->num_leds * 24;
	rmt_item32_t* items = malloc(sizeof(rmt_item32_t) * item_num);

    for (;;)
	{
    	xSemaphoreTake(leds->sem, portMAX_DELAY);
//        ESP_LOGI(MY_TAG, "RMT TX DATA);

		for (int i=0; i<leds->num_leds; ++i)
		{
			set_led_color(items + i * 24, leds->leds[i]);
		}

        // To send data according to the waveform items.
        rmt_write_items(channel, items, item_num, true);
        // Wait until sending is done.
        rmt_wait_tx_done(channel, portMAX_DELAY);

    	xSemaphoreGive(leds->sem);

        vTaskDelay(100 / portTICK_PERIOD_MS);
    }

    // before we free the data, make sure sending is already done.
    free(items);

    vTaskDelete(NULL);
}


void ws2812b_init(struct leds_t* leds)
{
    xTaskCreate(tx_task, "tx_task", 2048, leds, 10, NULL);
}
