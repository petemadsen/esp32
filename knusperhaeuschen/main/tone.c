/**
 * This code is public domain.
 */
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>

#include <esp_log.h>

#include <driver/rmt.h>
#include <driver/i2s.h>

static const char* MY_TAG = "knusperhaeuschen/note";

static EventGroupHandle_t x_events;
#define EVENT_BELL		BIT0


#ifdef USE_RMT
#define RMT_TX_CHANNEL RMT_CHANNEL_0
#define RMT_TX_GPIO	GPIO_NUM_13 // 13

/*
 * Prepare a raw table with a message in the Morse code
 *
 * The message is "ESP" : . ... .--.
 *
 * The table structure represents the RMT item structure:
 * {duration, level, duration, level}
 *
 */
static rmt_item32_t items_esp[] = {
    // E : dot
    {{{ 32767, 1, 32767, 0 }}}, // dot
    //
    {{{ 32767, 0, 32767, 0 }}}, // SPACE
    // S : dot, dot, dot
    {{{ 32767, 1, 32767, 0 }}}, // dot
    {{{ 32767, 1, 32767, 0 }}}, // dot
    {{{ 32767, 1, 32767, 0 }}}, // dot
    //
    {{{ 32767, 0, 32767, 0 }}}, // SPACE
    // P : dot, dash, dash, dot
    {{{ 32767, 1, 32767, 0 }}}, // dot
    {{{ 32767, 1, 32767, 1 }}},
    {{{ 32767, 1, 32767, 0 }}}, // dash
    {{{ 32767, 1, 32767, 1 }}},
    {{{ 32767, 1, 32767, 0 }}}, // dash
    {{{ 32767, 1, 32767, 0 }}}, // dot

    // RMT end marker
    {{{ 0, 1, 0, 0 }}}
};

static rmt_item32_t items_bell[] = {
	{{{ 32767, 1, 32767, 0 }}}, // dot
	{{{ 32767, 0, 32767, 0 }}}, // SPACE
	{{{ 16000, 1, 16000, 0 }}}, // dot
	{{{ 32767, 0, 32767, 0 }}}, // SPACE
	{{{ 32767, 1, 32767, 0 }}}, // dot
	{{{ 32767, 0, 32767, 0 }}}, // SPACE

	// RMT end marker
	{{{ 0, 1, 0, 0 }}}
};


/*
 * Initialize the RMT Tx channel
 */
static void rmt_tx_int()
{
    rmt_config_t config;
    config.rmt_mode = RMT_MODE_TX;
    config.channel = RMT_TX_CHANNEL;
    config.gpio_num = RMT_TX_GPIO;
    config.mem_block_num = 1;
    config.tx_config.loop_en = 0;
    // enable the carrier to be able to hear the Morse sound
    // if the RMT_TX_GPIO is connected to a speaker
    config.tx_config.carrier_en = 1;
    config.tx_config.idle_output_en = 1;
    config.tx_config.idle_level = 0;
    config.tx_config.carrier_duty_percent = 50;
    // set audible career frequency of 611 Hz
    // actually 611 Hz is the minimum, that can be set
    // with current implementation of the RMT API
    config.tx_config.carrier_freq_hz = 611;
    config.tx_config.carrier_level = 1;
    // set the maximum clock divider to be able to output
    // RMT pulses in range of about one hundred milliseconds
    config.clk_div = 255;

    ESP_ERROR_CHECK(rmt_config(&config));
    ESP_ERROR_CHECK(rmt_driver_install(config.channel, 0, 0));
}


static void rmt_play()
{
	int number_of_items = sizeof(items_bell) / sizeof(items_bell[0]);

		ESP_LOGI(MY_TAG, "Play bell.");
		ESP_ERROR_CHECK(rmt_write_items(RMT_TX_CHANNEL, items_bell, number_of_items, true));
		ESP_LOGI(MY_TAG, "Transmission complete");
}
#else


#define EXAMPLE_I2S_NUM 0
#define EXAMPLE_I2S_SAMPLE_RATE 16000
#define EXAMPLE_I2S_SAMPLE_BITS 16
#define EXAMPLE_I2S_FORMAT        (I2S_CHANNEL_FMT_RIGHT_LEFT)
#define EXAMPLE_I2S_READ_LEN      (16 * 1024)
#define I2S_ADC_UNIT              ADC_UNIT_1
#define I2S_ADC_CHANNEL           ADC1_CHANNEL_0


static void i2s_init()
{
	int i2s_num = EXAMPLE_I2S_NUM;
	i2s_config_t i2s_config = {
	   .mode = I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_TX | I2S_MODE_DAC_BUILT_IN | I2S_MODE_ADC_BUILT_IN,
	   .sample_rate =  EXAMPLE_I2S_SAMPLE_RATE,
	   .bits_per_sample = EXAMPLE_I2S_SAMPLE_BITS,
	   .communication_format = I2S_COMM_FORMAT_I2S_MSB,
	   .channel_format = EXAMPLE_I2S_FORMAT,
	   .intr_alloc_flags = 0,
	   .dma_buf_count = 2,
	   .dma_buf_len = 1024
	};
	//install and start i2s driver
	i2s_driver_install(i2s_num, &i2s_config, 0, NULL);
	//init DAC pad
	i2s_set_dac_mode(I2S_DAC_CHANNEL_BOTH_EN);
	//init ADC pad
	i2s_set_adc_mode(I2S_ADC_UNIT, I2S_ADC_CHANNEL);

}


/**
 * @brief Scale data to 16bit/32bit for I2S DMA output.
 *        DAC can only output 8bit data value.
 *        I2S DMA will still send 16 bit or 32bit data, the highest 8bit contain>>>s DAC data.
 */
int example_i2s_dac_data_scale(uint8_t* d_buff, uint8_t* s_buff, uint32_t len)
{
	uint32_t j = 0;
#if (EXAMPLE_I2S_SAMPLE_BITS == 16)
	for (int i = 0; i < len; i++) {
		d_buff[j++] = 0;
		d_buff[j++] = s_buff[i];
	}
	return (len * 2);
#else
	for (int i = 0; i < len; i++) {
		d_buff[j++] = 0;
		d_buff[j++] = 0;
		d_buff[j++] = 0;
		d_buff[j++] = s_buff[i];
	}
	return (len * 4);
#endif
}



#include "audio_example_file.h"
static void i2s_play()
{
	// set file play mode
	i2s_set_clk(EXAMPLE_I2S_NUM, 16000, EXAMPLE_I2S_SAMPLE_BITS, 1);

	int i2s_read_len = EXAMPLE_I2S_READ_LEN;
	size_t bytes_read, bytes_written;

	uint8_t* i2s_write_buff = (uint8_t*) calloc(i2s_read_len, sizeof(char));

	//4. Play an example audio file(file format: 8bit/16khz/single channel)
	printf("Playing file example: \n");
	int offset = 0;
	int tot_size = sizeof(audio_table);
//	example_set_file_play_mode();
	while (offset < tot_size) {
		int play_len = ((tot_size - offset) > (4 * 1024)) ? (4 * 1024) : (tot_size - offset);
		int i2s_wr_len = example_i2s_dac_data_scale(i2s_write_buff, (uint8_t*)(audio_table + offset), play_len);
		i2s_write(EXAMPLE_I2S_NUM, i2s_write_buff, i2s_wr_len, &bytes_written, portMAX_DELAY);
		offset += play_len;
//		example_disp_buf((uint8_t*) i2s_write_buff, 32);
	}

	free(i2s_write_buff);
}



#endif


static void tone_task(void* ignore)
{
	ESP_LOGI(MY_TAG, "Configuring transmitter");
#ifdef USE_RMT
	rmt_tx_init();
#else
	i2s_init();
#endif

	for (;;)
	{
		EventBits_t bits = xEventGroupWaitBits(x_events,
											   EVENT_BELL,
											   true, false, portMAX_DELAY);
#ifdef USE_RMT
		rmt_play();
#else
		i2s_play();
#endif
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
