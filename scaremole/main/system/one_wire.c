/**
 * This code is public domain.
 */
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "one_wire.h"


static void send_bit(gpio_num_t pin, int bit);
static int read_bit(gpio_num_t pin);


esp_err_t one_wire_reset(gpio_num_t pin)
{
	gpio_pad_select_gpio(pin);

	// start init
	gpio_set_direction(pin, GPIO_MODE_OUTPUT);
	gpio_set_level(pin, 0);
	ets_delay_us(500);
	gpio_set_level(pin, 1);
	gpio_set_direction(pin, GPIO_MODE_INPUT);

	// wait for reply
	ets_delay_us(30);
	int itsme = 0;
	itsme += (gpio_get_level(pin) == 0) ? 1 : 0;	// device pulls down
	ets_delay_us(470);
	itsme += (gpio_get_level(pin) == 1) ? 1 : 0;	// device pulls up

	if (itsme == 2)
		return ESP_OK;

	return ESP_ERR_NOT_FOUND;
}


esp_err_t one_wire_write(gpio_num_t pin, int data)
{
	gpio_set_direction(pin, GPIO_MODE_OUTPUT);

	for (int i=0; i<8; ++i)
	{
		int bit = data & 1;
		data >>= 1;
		send_bit(pin, bit);
	}
	ets_delay_us(100);

	return ESP_OK;
}


int one_wire_read(gpio_num_t pin)
{
	int bits = 0;

	for (int i=0; i<8; ++i)
	{
		bits |= (read_bit(pin) << i);
		ets_delay_us(15);
	}

	return bits;
}


static void send_bit(gpio_num_t pin, int bit)
{
	gpio_set_direction(pin, GPIO_MODE_OUTPUT);
	gpio_set_level(pin, 0);
	ets_delay_us(5);
	gpio_set_level(pin, bit);
	ets_delay_us(80);
	gpio_set_level(pin, 1);
}


static int read_bit(gpio_num_t pin)
{
	gpio_set_direction(pin, GPIO_MODE_OUTPUT);
	gpio_set_level(pin, 0);
	ets_delay_us(2);
	gpio_set_level(pin, 1);
	ets_delay_us(15);

	gpio_set_direction(pin, GPIO_MODE_INPUT);
	return gpio_get_level(pin);
}
