/**
 * This code is public domain.
 */
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>

#include "system/one_wire.h"


static const char* MY_TAG = "scaremole/ds18b20";


#define READ_ROM		0x33	// 8 byte reply?
#define SKIP_ROM		0xcc
#define CONVERT			0x44
#define READ_SCRATCHPAD	0xbe

#define FAMILY_CODE		0x28


static gpio_num_t m_pin = GPIO_NUM_32;
static float m_last_temp = -666;


static float read_temp();


void ds18b20_task(void* pvParameters)
{
	uint8_t data[8];

	for (;;)
	{
		esp_err_t ret = one_wire_reset(m_pin);
		ESP_LOGI(MY_TAG, "RESET: %s", esp_err_to_name(ret));

		if (ret == ESP_OK)
		{
			one_wire_write(m_pin, READ_ROM);

			for (int i=0; i<8; ++i)
				data[i] = one_wire_read(m_pin);

			if (data[0] != FAMILY_CODE)
				ESP_LOGW(MY_TAG, "Not my family: 0x%02x", data[0]);

			for (int i=0; i<8; ++i)
				ESP_LOGI(MY_TAG, "DATA[%d] = 0x%02x", i, data[i]);
		}

		m_last_temp = read_temp();
		ESP_LOGI(MY_TAG, "TEMP = %.2f", m_last_temp);

		vTaskDelay(60 * 1000 / portTICK_PERIOD_MS);
	};
}


static float read_temp()
{
	esp_err_t ret = one_wire_reset(m_pin);
	ESP_LOGI(MY_TAG, "tRESET: %s", esp_err_to_name(ret));

	one_wire_write(m_pin, SKIP_ROM);
	one_wire_write(m_pin, CONVERT);
//	vTaskDelay(1 * 1000 / portTICK_PERIOD_MS);
	vTaskDelay(750 / portTICK_RATE_MS);

	ret = one_wire_reset(m_pin);
	ESP_LOGI(MY_TAG, "tRESET: %s", esp_err_to_name(ret));

	one_wire_write(m_pin, SKIP_ROM);
	one_wire_write(m_pin, READ_SCRATCHPAD);
	int temp1 = one_wire_read(m_pin);
	int temp2 = one_wire_read(m_pin);

	ret = one_wire_reset(m_pin);
	ESP_LOGI(MY_TAG, "tRESET: %s (%d %d)", esp_err_to_name(ret), temp1, temp2);

	return ((float)temp1 + ((float)temp2 * 256.0)) / 16.0;
}


float ds18b20_last_temp()
{
	return m_last_temp;
}
