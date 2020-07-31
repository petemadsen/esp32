/**
 * This code is public domain.
 */
#include <freertos/FreeRTOS.h>

#include "one_wire.h"


esp_err_t one_wire_reset(gpio_num_t pin)
{
	gpio_pad_select_gpio(pin);
	gpio_set_direction(pin, GPIO_MODE_OUTPUT);
//	gpio_hold_dis(PIN_BUZZER);

	gpio_set_level(pin, 1);
	vTaskDelay(100 / portTICK_PERIOD_MS);
	gpio_set_level(pin, 0);
	vTaskDelay(500 / portTICK_PERIOD_MS);
	gpio_set_level(pin, 1);

	return ESP_OK;
}


esp_err_t one_wire_write(gpio_num_t pin, uint8_t data)
{
	for (int i=0; i<8; ++i)
	{
		int is_zero = data & 1;
		data >>= 1;

		if (is_zero)
		{
			gpio_set_level(pin, 1);
			vTaskDelay(100 / portTICK_PERIOD_MS);
			gpio_set_level(pin, 0);
			vTaskDelay(500 / portTICK_PERIOD_MS);
			gpio_set_level(pin, 1);
		}
		else
		{
			gpio_set_level(pin, 1);
			vTaskDelay(400 / portTICK_PERIOD_MS);
			gpio_set_level(pin, 0);
			vTaskDelay(100 / portTICK_PERIOD_MS);
			gpio_set_level(pin, 1);
		}
	}

	return ESP_OK;
}


esp_err_t one_wire_read(gpio_num_t pin, uint8_t* reply)
{
	*reply = 0;

	for (int i=0; i<8; ++i)
	{
		int level = gpio_get_level(pin);
		*reply |= level;
		*reply <<= 1;
	}

	return ESP_OK;
}
