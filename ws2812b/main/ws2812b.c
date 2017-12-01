/**
 *
 * Based on "NEC remote infrared RMT example"
 * https://github.com/espressif/esp-idf/tree/master/examples/peripherals/rmt_nec_tx_rx
 *
 */
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "esp_err.h"
#include "esp_log.h"
#include "driver/rmt.h"
#include "driver/periph_ctrl.h"
#include "soc/rmt_reg.h"

#include "sdkconfig.h"

static const char* NEC_TAG = "HCSR04";


#define RMT_TX_CHANNEL    1     /*!< RMT channel for transmitter */
#define RMT_TX_GPIO_NUM  18     /*!< GPIO number for transmitter signal */

#define RMT_CLK_DIV      100    /*!< RMT counter clock divider */
#define RMT_TICK_10_US    (80000000/RMT_CLK_DIV/100000)   /*!< RMT counter value for 10 us.(Source clock is APB clock) */

#define HCSR04_MAX_TIMEOUT_US  25000   /*!< RMT receiver timeout value(us) */


#define US2TICKS(us)	(us / 10 * RMT_TICK_10_US)
#define TICKS2US(ticks)	(ticks * 10 / RMT_TICK_10_US)


static inline void set_item_edge(rmt_item32_t* item, int low_us, int high_us)
{
    item->level0 = 0;
    item->duration0 = US2TICKS(low_us);
    item->level1 = 1;
    item->duration1 = US2TICKS(high_us);
}


/*
 * @brief RMT transmitter initialization
 */
static void nec_tx_init()
{
    rmt_config_t rmt_tx;
    rmt_tx.channel = RMT_TX_CHANNEL;
    rmt_tx.gpio_num = RMT_TX_GPIO_NUM;
    rmt_tx.mem_block_num = 1;
    rmt_tx.clk_div = RMT_CLK_DIV;
    rmt_tx.tx_config.loop_en = false;

    rmt_tx.tx_config.carrier_duty_percent = 50;
    rmt_tx.tx_config.carrier_freq_hz = 38000;
    rmt_tx.tx_config.carrier_level = 1;
    rmt_tx.tx_config.carrier_en = 0;	// off

    rmt_tx.tx_config.idle_level = 0;
    rmt_tx.tx_config.idle_output_en = true;
    rmt_tx.rmt_mode = 0;
    rmt_config(&rmt_tx);
    rmt_driver_install(rmt_tx.channel, 0, 0);
}


/**
 * @brief RMT transmitter demo, this task will periodically send NEC data. (100 * 32 bits each time.)
 *
 */
static void tx_task()
{
    vTaskDelay(10);
    nec_tx_init();

    int channel = RMT_TX_CHANNEL;

	int item_num = 32;
    for (;;)
	{
        ESP_LOGI(NEC_TAG, "RMT TX DATA");

		rmt_item32_t* items = malloc(item_num);
		for (int i=0; i<item_num; ++i)
		{
			set_item_edge(&items[i], 900, 350);
		}

        // To send data according to the waveform items.
        rmt_write_items(channel, items, item_num, true);
        // Wait until sending is done.
        rmt_wait_tx_done(channel, portMAX_DELAY);
        // before we free the data, make sure sending is already done.
		
		free(items);

        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }

    vTaskDelete(NULL);
}


void ws2812b_init()
{
    xTaskCreate(tx_task, "tx_task", 2048, NULL, 10, NULL);
}
