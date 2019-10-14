/**
 * This code is public domain.
 */
#include <stdio.h>
#include <stdlib.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>

#include <driver/gpio.h>
#include <driver/adc.h>
#include <driver/dac.h>

#include <esp_system.h>
#include <esp_adc_cal.h>
#include <esp_log.h>

#include "common.h"


static const adc1_channel_t ADC1_EXAMPLE_CHANNEL = ADC1_GPIO34_CHANNEL;
static const char* MY_TAG = PROJECT_TAG("voltage");


static double m_last_val = 0.0;


static void voltage_task(void* args)
{
	const int NUM_SAMPLES = 10;

	adc1_config_width(ADC_WIDTH_BIT_12);
	adc1_config_channel_atten( ADC1_EXAMPLE_CHANNEL, ADC_ATTEN_DB_11 );

    vTaskDelay(2 * portTICK_PERIOD_MS);

    printf("start conversion.\n");
	for (;;)
	{
		int read_raw = 0;
		for (int i=0; i<NUM_SAMPLES; ++i)
		{
			read_raw += adc1_get_raw(ADC1_EXAMPLE_CHANNEL);
		}
		read_raw /= NUM_SAMPLES;

		double pin_voltage = (double)read_raw / 4096.0 * 3.53;
		m_last_val = pin_voltage * (100.0 + 20.0) / 20.0; // r divider

		ESP_LOGI(MY_TAG, "New measurement: %.2f", m_last_val);

		// -- 60 secs
		vTaskDelay(60 * 1000 / portTICK_PERIOD_MS );
    }
}


double voltage_get()
{
	return m_last_val;
}


void voltage_init()
{
	xTaskCreate(voltage_task, "voltage_task", 2048, NULL, 10, NULL);
}
