/**
 * This code is public domain.
 */
#include <driver/adc.h>

#include <esp_system.h>
#include <esp_adc_cal.h>


static adc1_channel_t ADC1_EXAMPLE_CHANNEL;


void voltage_init(adc1_channel_t ch)
{
	ADC1_EXAMPLE_CHANNEL = ch;

	adc1_config_width(ADC_WIDTH_BIT_12);
	adc1_config_channel_atten(ADC1_EXAMPLE_CHANNEL, ADC_ATTEN_DB_11);
}


double voltage_get()
{
	const int NUM_SAMPLES = 10;

	int read_raw = 0;
	for (int i=0; i<NUM_SAMPLES; ++i)
	{
		read_raw += adc1_get_raw(ADC1_EXAMPLE_CHANNEL);
	}
	read_raw /= NUM_SAMPLES;

	double pin_voltage = (double)read_raw / 4096.0 * 3.53;
	pin_voltage *= (100.0 + 20.0) / 20.0; // r divider

	return pin_voltage;
}
