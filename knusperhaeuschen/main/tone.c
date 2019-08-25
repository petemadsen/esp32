/**
 * This code is public domain.
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


static void tone_task(void* ignore)
{
	ESP_LOGI(MY_TAG, "Configuring transmitter");
    rmt_tx_int();

	int number_of_items = sizeof(items_bell) / sizeof(items_bell[0]);

	for (;;)
	{
		EventBits_t bits = xEventGroupWaitBits(x_events,
											   EVENT_BELL,
											   true, false, portMAX_DELAY);

		ESP_LOGI(MY_TAG, "Play bell.");
		ESP_ERROR_CHECK(rmt_write_items(RMT_TX_CHANNEL, items_bell, number_of_items, true));
		ESP_LOGI(MY_TAG, "Transmission complete");
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
