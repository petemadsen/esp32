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

static const char* M_TAG = "HDT11";


#define RMT_RX_CHANNEL    0     /*!< RMT channel for receiver */
#define RMT_RX_GPIO_NUM  19     /*!< GPIO number for receiver */

#define RMT_CLK_DIV      100    /*!< RMT counter clock divider */
#define RMT_TICK_10_US    (80000000/RMT_CLK_DIV/100000)   /*!< RMT counter value for 10 us.(Source clock is APB clock) */

#define HCSR04_MAX_TIMEOUT_US  25000   /*!< RMT receiver timeout value(us) */


#define US2TICKS(us)	(us / 10 * RMT_TICK_10_US)
#define TICKS2US(ticks)	(ticks * 10 / RMT_TICK_10_US)


/**
 * @brief RMT receiver demo, this task will print each received NEC data.
 *
 */
static void rx_task()
{
	// -- init
    rmt_config_t rmt_rx;
    rmt_rx.channel = RMT_RX_CHANNEL;
    rmt_rx.gpio_num = RMT_RX_GPIO_NUM;
    rmt_rx.clk_div = RMT_CLK_DIV;
    rmt_rx.mem_block_num = 1;
    rmt_rx.rmt_mode = RMT_MODE_RX;
    rmt_rx.rx_config.filter_en = false;
    rmt_rx.rx_config.filter_ticks_thresh = 100;
    rmt_rx.rx_config.idle_threshold = US2TICKS(HCSR04_MAX_TIMEOUT_US);
    rmt_config(&rmt_rx);
    rmt_driver_install(rmt_rx.channel, 1000, 0);


	// -- ready to receive
    int channel = RMT_RX_CHANNEL;

    RingbufHandle_t rb = NULL;

    //get RMT RX ringbuffer
    rmt_get_ringbuf_handle(channel, &rb);
    rmt_rx_start(channel, 1);

	int rcv_bytes[5];

    while (rb)
	{
        size_t rx_size = 0;
        //try to receive data from ringbuffer.
        //RMT driver will push all the data it receives to its ringbuffer.
        //We just need to parse the value and return the spaces of ringbuffer.
        rmt_item32_t* items = (rmt_item32_t*) xRingbufferReceive(rb, &rx_size, 1000);
        if (items)
        {
			int nr_of_bits = rx_size / sizeof(rmt_item32_t);
			ESP_LOGI(M_TAG, "RCV: %d - @0x%x", nr_of_bits, (unsigned int)items);

			if (nr_of_bits == 43)
			{
				// ignore the first pulse: that's us
				// ignore the second pulse: that's ACK
				rmt_item32_t* item = &items[2];
				for (int i=0; i<5; ++i)
				{
					int b = 0;
					for (int k=0; k<8; ++k)
					{
						b |= (TICKS2US(item->duration1) > 40 ? 1 : 0);
						b <<= 1;
						++item;
					}
					b >>= 1;
					rcv_bytes[i] = b;
				}

				if ((rcv_bytes[0] + rcv_bytes[2]) == rcv_bytes[4])
				{
					ESP_LOGI(M_TAG, "T: %d C H: %d %%", rcv_bytes[2], rcv_bytes[0]);
				}
				else
				{
					ESP_LOGW(M_TAG, "CRC error");
				}

#if 0
				for (int i=0; i<rx_size / sizeof(rmt_item32_t); ++i)
					ESP_LOGI(M_TAG, "RMT RCV -- %d:%d | %d:%d | b:%d",
							items[i].level0, TICKS2US(items[i].duration0),
							items[i].level1, TICKS2US(items[i].duration1),
							items[i].duration1 > 40 ? 1 : 0);
#endif
			}
			else
			{
				ESP_LOGW(M_TAG, "wrong number of bits: %d", nr_of_bits);
			}

            //after parsing the data, return spaces to ringbuffer.
            vRingbufferReturnItem(rb, (void*) items);
        } else {
			vTaskDelay(10);
//            break;
        }
    }

    vTaskDelete(NULL);
}


/**
 * @brief Initialize the communication.
 *
 */
static void tx_task()
{
	for (;;)
	{
		ESP_LOGI(M_TAG, "ACTION");

		gpio_pad_select_gpio(RMT_RX_GPIO_NUM);
		gpio_set_direction(RMT_RX_GPIO_NUM, GPIO_MODE_OUTPUT);

		gpio_set_level(RMT_RX_GPIO_NUM, 0);
		// 2ms required
		vTaskDelay(20 / portTICK_PERIOD_MS);
		gpio_set_level(RMT_RX_GPIO_NUM, 1);

		gpio_set_direction(RMT_RX_GPIO_NUM, GPIO_MODE_INPUT);

		vTaskDelay(1000 / portTICK_PERIOD_MS);
	}

    vTaskDelete(NULL);
}


void dht11_init()
{
    xTaskCreate(rx_task, "rx_task", 2048, NULL, 10, NULL);
    xTaskCreate(tx_task, "tx_task", 2048, NULL, 10, NULL);
}
