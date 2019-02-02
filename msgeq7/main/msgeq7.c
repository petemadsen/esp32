/**
 * msgeq7.c
 */
#include <stdio.h>
#include <string.h>

#include <esp_err.h>
#include <esp_log.h>
#include <driver/gpio.h>
#include <driver/adc.h>
#include <esp_adc_cal.h>

#include "sdkconfig.h"
#include "msgeq7.h"
#include "config.h"


#define DEFAULT_VREF 1100


static esp_adc_cal_characteristics_t* adc_chars;
static const adc_channel_t channel = ADC_CHANNEL_6;
static const int NO_OF_SAMPLES = 64;
static const adc_atten_t atten;
static const adc_unit_t unit = ADC_UNIT_1;


#define RESET_PIN	26
#define STROBE_PIN	25


static void msgeq7_task(void* arg)
{
	// configure gpio
	gpio_pad_select_gpio(RESET_PIN);
	gpio_set_pull_mode(RESET_PIN, GPIO_PULLUP_ONLY);
	gpio_set_direction(RESET_PIN, GPIO_MODE_OUTPUT);

	gpio_pad_select_gpio(STROBE_PIN);
	gpio_set_pull_mode(STROBE_PIN, GPIO_PULLUP_ONLY);
	gpio_set_direction(STROBE_PIN, GPIO_MODE_OUTPUT);

	// configure ADC
	adc1_config_width(ADC_WIDTH_BIT_12);
	adc1_config_channel_atten(channel, atten);

	adc_chars = calloc(1, sizeof(esp_adc_cal_characteristics_t));
	esp_adc_cal_value_t val_type = esp_adc_cal_characterize(unit, atten, ADC_WIDTH_BIT_12, DEFAULT_VREF, adc_chars);

    for (;;)
	{
		printf("\n");

		// reset
		gpio_set_level(RESET_PIN, 1);
        vTaskDelay(100 / portTICK_PERIOD_MS);
		gpio_set_level(RESET_PIN, 0);
		gpio_set_level(STROBE_PIN, 1);
        vTaskDelay(100 / portTICK_PERIOD_MS);

		for (int k=0; k<7; ++k)
		{
			gpio_set_level(STROBE_PIN, 0);
			vTaskDelay(100 / portTICK_PERIOD_MS);

			uint32_t adc_reading = 0;
			for (int i=0; i<NO_OF_SAMPLES; ++i)
			{
				adc_reading += adc1_get_raw((adc1_channel_t)channel);
			}
			adc_reading /= NO_OF_SAMPLES;

			gpio_set_level(STROBE_PIN, 1);
			vTaskDelay(100 / portTICK_PERIOD_MS);

			uint32_t voltage = esp_adc_cal_raw_to_voltage(adc_reading, adc_chars);
			printf("[%d] raw: %d\tvol: %dmV\n", k, adc_reading, voltage);
		}

#if 0
		gpio_set_level(RESET_PIN, 1);
		gpio_set_level(STROBE_PIN, 1);

        vTaskDelay(1000 / portTICK_PERIOD_MS);

		gpio_set_level(RESET_PIN, 0);
		gpio_set_level(STROBE_PIN, 0);
#endif

        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }

    vTaskDelete(NULL);
}


void msgeq7_init()
{
    xTaskCreate(msgeq7_task, "tx_task", 2048, NULL, 10, NULL);
}
