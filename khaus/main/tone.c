/**
 * This code is public domain.
 */
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>

#include <esp_log.h>

#include <driver/rmt.h>
#include <driver/i2s.h>

#include <esp_spiffs.h>

#include "common.h"
#include "my_settings.h"
#include "read_wav.h"


static const char* MY_TAG = "khaus/note";

static EventGroupHandle_t x_events;
#define EVENT_BELL		BIT0

static int m_bell_num = 0;
static uint8_t* m_bell = NULL;
static size_t m_bell_len = 0;


static bool read_file();


#define SETTING_BELL "tone.bell"


#define USE_RMT
#undef USE_RMT


#ifdef USE_RMT
#define RMT_TX_CHANNEL RMT_CHANNEL_0
#define RMT_TX_GPIO	GPIO_NUM_25 // 13

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
static void rmt_tx_init()
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
#define EXAMPLE_I2S_FORMAT        (I2S_CHANNEL_FMT_ONLY_RIGHT)
#define EXAMPLE_I2S_READ_LEN      (16 * 1024)
#define I2S_ADC_UNIT              ADC_UNIT_1
#define I2S_ADC_CHANNEL           ADC1_CHANNEL_0
//I2S channel number
#define EXAMPLE_I2S_CHANNEL_NUM   ((EXAMPLE_I2S_FORMAT < I2S_CHANNEL_FMT_ONLY_RIGHT) ? (2) : (1))



static void i2s_init()
{
	// GPIO_NUM_25 (right) + GPIO_NUM_26 (left)
	i2s_config_t i2s_config = {
		.mode = I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_DAC_BUILT_IN,
		.sample_rate =  EXAMPLE_I2S_SAMPLE_RATE,
		.bits_per_sample = EXAMPLE_I2S_SAMPLE_BITS,
		.communication_format = I2S_COMM_FORMAT_I2S_MSB,
		.channel_format = EXAMPLE_I2S_FORMAT,
		.intr_alloc_flags = 0, // default interrupt priority
//		.intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
		.dma_buf_count = 2,
		.dma_buf_len = 1024,
		.use_apll = true,
	};
//	const i2s_pin_config_t pin_config = {
//		.bck_io_num = -1, // I2S_PIN_NO_CHANGE,// 26,
//		.ws_io_num = GPIO_NUM_13,// I2S_PIN_NO_CHANGE,// 25,
//		.data_out_num = -1, // I2S_PIN_NO_CHANGE,// 22,
//		.data_in_num = -1, // I2S_PIN_NO_CHANGE
//	};

	gpio_set_direction(GPIO_NUM_13, GPIO_MODE_OUTPUT);

	i2s_driver_install(EXAMPLE_I2S_NUM, &i2s_config, 0, NULL);

//	i2s_set_pin(i2s_num, &pin_config);

	i2s_set_dac_mode(I2S_DAC_CHANNEL_RIGHT_EN);
}


/**
 * @brief Scale data to 16bit/32bit for I2S DMA output.
 *        DAC can only output 8bit data value.
 *        I2S DMA will still send 16 bit or 32bit data, the highest 8bit contains DAC data.
 */
size_t example_i2s_dac_data_scale(uint8_t* d_buff, uint8_t* s_buff, size_t len)
{
	uint32_t j = 0;
#if (EXAMPLE_I2S_SAMPLE_BITS == 16)
	for (size_t i = 0; i < len; i++) {
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
static void i2s_play(uint8_t* data, size_t data_len)
{
	size_t i2s_read_len = EXAMPLE_I2S_READ_LEN;
	size_t bytes_written;

	i2s_start(EXAMPLE_I2S_NUM);

	uint8_t* i2s_write_buff = (uint8_t*) calloc(i2s_read_len, sizeof(char));

	//4. Play an example audio file(file format: 8bit/16khz/single channel)
	printf("Playing file example: len=%zu\n", data_len);
	size_t offset = 0;
	size_t tot_size = data_len;

	// set file play mode
	i2s_set_clk(EXAMPLE_I2S_NUM, 16000, EXAMPLE_I2S_SAMPLE_BITS, 1);

	while (offset < tot_size)
	{
		size_t play_len = ((tot_size - offset) > (4 * 1024)) ? (4 * 1024) : (tot_size - offset);
		size_t i2s_wr_len = example_i2s_dac_data_scale(i2s_write_buff, (uint8_t*)(data + offset), play_len);
		i2s_write(EXAMPLE_I2S_NUM, i2s_write_buff, i2s_wr_len, &bytes_written, portMAX_DELAY);
		offset += play_len;
//		example_disp_buf((uint8_t*) i2s_write_buff, 32);
	}
	i2s_set_clk(EXAMPLE_I2S_NUM, EXAMPLE_I2S_SAMPLE_RATE, EXAMPLE_I2S_SAMPLE_BITS, EXAMPLE_I2S_CHANNEL_NUM);

	i2s_stop(EXAMPLE_I2S_NUM);

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

	// -- on/off pin
	gpio_pad_select_gpio(PROJECT_TONE_ONOFF_PIN);
	gpio_set_direction(PROJECT_TONE_ONOFF_PIN, GPIO_MODE_OUTPUT);
	gpio_set_level(PROJECT_TONE_ONOFF_PIN, 0);

	if (!read_file())
	{
		if (m_bell)
			free(m_bell);
		m_bell = NULL;
	}

	for (;;)
	{
		settings_get_int32(SETTING_BELL, &m_bell_num, true);

		xEventGroupWaitBits(x_events, EVENT_BELL, true, false, portMAX_DELAY);

		gpio_set_level(PROJECT_TONE_ONOFF_PIN, 1);
#ifdef USE_RMT
		rmt_play();
#else
		if (m_bell)
			i2s_play(m_bell, m_bell_len);
		else
			i2s_play(audio_table, sizeof(audio_table));
#endif
		gpio_set_level(PROJECT_TONE_ONOFF_PIN, 0);

		ESP_LOGI(MY_TAG, "--done");
	}

    vTaskDelete(NULL);
}


static bool read_file()
{
	// -- open
	esp_vfs_spiffs_conf_t conf = {
			.base_path = "/spiffs",
			.partition_label = NULL,
			.max_files = 1,
			.format_if_mount_failed = true
	};
	esp_err_t ret = esp_vfs_spiffs_register(&conf);
	if (ret != ESP_OK)
	{
		if (ret == ESP_FAIL)
			ESP_LOGE(MY_TAG, "Failed to mount or format filesystem");
		else if (ret == ESP_ERR_NOT_FOUND)
			ESP_LOGE(MY_TAG, "Failed to find SPIFFS partition");
		else
			ESP_LOGE(MY_TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));

		return false;
	}

	char filename[40];
	sprintf(filename, "/spiffs/bell%d.wav", m_bell_num);
	FILE* file = fopen(filename, "r");
	if (!file)
	{
		ESP_LOGE(MY_TAG, "Could not open file");
		esp_vfs_spiffs_unregister(NULL);
		return false;
	}

	m_bell = read_wav(file, &m_bell_len);

#if 0
	// get file len
	fseek(file, 0L, SEEK_END);
	long len = ftell(file);
	rewind(file);
	ESP_LOGI(MY_TAG, "Loading file with size: %ld", len);

	//
	if (m_bell)
		free(m_bell);
	m_bell = malloc(len);
	if (!m_bell)
	{
		ESP_LOGE(MY_TAG, "Could not allocate memory: %ld", len);
		fclose(file);
		return false;
	}
	m_bell_len = (size_t)len;
	ESP_LOGI(MY_TAG, "m_bell is loaded: %d", (m_bell == NULL));

	// read
	char* p = (char*)m_bell;
	while (len)
	{
		ESP_LOGI(MY_TAG, "Remaining: %ld", len);
		long read = fread(p, sizeof(char), len, file);
		ESP_LOGI(MY_TAG, "read: %ld", read);
		if (read < 0)
		{
			ESP_LOGE(MY_TAG, "Could not read file: %ld", read);
			esp_vfs_spiffs_unregister(NULL);
			free(m_bell);
			m_bell = NULL;
			return false;
		}

		len -= read;
		p += read;
	}

	printf("==\n");
	bool show_ascii = true;
	for (size_t i = 0; i < m_bell_len; ++i)
	{
		if (show_ascii)
		{
			char ch = m_bell[i];
//			char ch = (m_bell[i] >= 0 && m_bell[i] <= 'z' ? m_bell[i] : '.');
			printf("%c", ch);
		}
		else
		{
			printf(" 0x%02x", m_bell[i]);
			if (i!=0 && (i%8)==0)
				printf("\n");
		}
	}
	printf("--\n");

	// done
	fclose(file);
#endif

	esp_vfs_spiffs_unregister(NULL);
	return (m_bell != NULL);// true;
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


bool tone_set(int num)
{
	m_bell_num = num;

	bool ok = read_file();
	if (ok)
	{
		settings_set_int32(SETTING_BELL, m_bell_num, false);
		tone_bell();
	}

	ESP_LOGI(MY_TAG, "Tone set: %d", ok);
	return ok;
}


int tone_get()
{
	return m_bell_num;
}
