/* based on "RMT transmit example"

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>

#include <esp_log.h>
#include <driver/rmt.h>

static const char* MY_TAG = "knusperhaeuschen/note";

static EventGroupHandle_t x_events;
#define EVENT_BELL		BIT0


#define RMT_TX_CHANNEL RMT_CHANNEL_0
#define RMT_TX_GPIO	GPIO_NUM_13 // 13
#define SAMPLE_CNT  (10)


#define FREQ_TO_TICKS(f) (uint16_t)(1./f * 1000.0 * 1000.0 / 2.0)
#define A2 FREQ_TO_TICKS(110.0) // 4544
#define A3 FREQ_TO_TICKS(220.0) // 2272
#define A4 FREQ_TO_TICKS(440.0) // 1136
#define C4 FREQ_TO_TICKS(262.0)
#define A5 FREQ_TO_TICKS(880.0) // 568
#define A6 FREQ_TO_TICKS(1760.0)

//Convert uint8_t type of data to rmt format data.
static void IRAM_ATTR u8_to_rmt(const void* src, rmt_item32_t* dest, size_t src_size, 
                         size_t wanted_num, size_t* translated_size, size_t* item_num)
{
	if(src == NULL || dest == NULL)
	{
        *translated_size = 0;
        *item_num = 0;
        return;
    }
	const rmt_item32_t bit0 = {{{ 32767, 1, 15000, 0 }}}; // Logical 0
	const rmt_item32_t bit1 = {{{ 32767, 1, 32767, 0 }}}; // Logical 1
    size_t size = 0;
    size_t num = 0;
    uint8_t *psrc = (uint8_t *)src;
    rmt_item32_t* pdest = dest;
	while (size < src_size && num < wanted_num)
	{
		for(int i = 0; i < 8; i++)
		{
			if(*psrc & (0x1 << i))
			{
                pdest->val =  bit1.val; 
			}
			else
			{
                pdest->val =  bit0.val;
            }
            num++;
            pdest++;
        }
        size++;
        psrc++;
    }
    *translated_size = size;
    *item_num = num;
}

/*
 * Initialize the RMT Tx channel
 */
void rmt_tx_int()
{
    rmt_config_t config;
    config.rmt_mode = RMT_MODE_TX;
    config.channel = RMT_TX_CHANNEL;
    config.gpio_num = RMT_TX_GPIO;
    config.mem_block_num = 1;
    config.tx_config.loop_en = 0;
	config.tx_config.loop_en = 1;
	// enable the carrier to be able to hear the Morse sound
    // if the RMT_TX_GPIO is connected to a speaker
	config.tx_config.carrier_en = 0;
	config.tx_config.idle_output_en = 1;
	config.tx_config.idle_level = RMT_IDLE_LEVEL_LOW;
    config.tx_config.carrier_duty_percent = 50;
    // set audible career frequency of 611 Hz
    // actually 611 Hz is the minimum, that can be set
    // with current implementation of the RMT API
	config.tx_config.carrier_freq_hz = 880;
    config.tx_config.carrier_level = 1;
    // set the maximum clock divider to be able to output
    // RMT pulses in range of about one hundred milliseconds
    config.clk_div = 255;
	config.clk_div = 80;

    ESP_ERROR_CHECK(rmt_config(&config));
    ESP_ERROR_CHECK(rmt_driver_install(config.channel, 0, 0));
    ESP_ERROR_CHECK(rmt_translator_init(config.channel, u8_to_rmt));
}

static void tone_task(void *ignore)
{
	ESP_LOGI(MY_TAG, "Configuring transmitter");
    rmt_tx_int();

	rmt_item32_t items[1];
	items[0].duration0 = 3;
	items[0].level0 = 1;
	items[0].duration1 = 0;
	items[0].level1 = 0;

	items[0].duration0 = C4;
	items[0].duration1 = C4;

	uint16_t notes[] = {
//		A2, 1000,
//		A3, 1000,
		A4, 1000,
		A5, 1000,
//		A6, 1000,
		0, 0,
	};

	ESP_LOGI(MY_TAG, "A2 = %d (4544)", FREQ_TO_TICKS(110.0));
	ESP_LOGI(MY_TAG, "A3 = %d (2272)", FREQ_TO_TICKS(220.0));
	ESP_LOGI(MY_TAG, "A4 = %d (1136)", FREQ_TO_TICKS(440.0));
	ESP_LOGI(MY_TAG, "A5 = %d (568)", FREQ_TO_TICKS(880.0));
	ESP_LOGI(MY_TAG, "A6 = %d (284)", FREQ_TO_TICKS(1760.0));
	vTaskDelay(5000 / portTICK_PERIOD_MS);

	for (;;)
	{
		int pos = 0;
		do {
			ESP_LOGI(MY_TAG, "=> %u / %u", notes[pos+0], notes[pos+1]);
			items[0].duration0 = notes[pos+0];
			items[0].duration1 = notes[pos+0];

			rmt_write_items(RMT_TX_CHANNEL, items, 1, 0);
			vTaskDelay(notes[pos+1] / portTICK_PERIOD_MS);

			pos += 2;
		} while (notes[pos]);

#if 0
		EventBits_t bits = xEventGroupWaitBits(x_events,
											   EVENT_BELL,
											   true, false, portMAX_DELAY);
		if (bits & EVENT_BELL)
		{
			ESP_ERROR_CHECK(rmt_write_items(RMT_TX_CHANNEL, items, number_of_items, true));
			ESP_LOGI(MY_TAG, "Transmission complete");
		}
#endif
#if 0
        ESP_ERROR_CHECK(rmt_write_sample(RMT_TX_CHANNEL, sample, SAMPLE_CNT, true));
		ESP_LOGI(MY_TAG, "Sample transmission complete");
#endif
//		vTaskDelay(1000 / portTICK_PERIOD_MS);
	}
    vTaskDelete(NULL);
}


void tone_init()
{
	x_events = xEventGroupCreate();
	xTaskCreate(tone_task, "tone_task", 2048, NULL, 5, NULL);
}


void tone_bell()
{
	xEventGroupSetBits(x_events, EVENT_BELL);
}
