/**
 * This code is public domain.
 */
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <esp_system.h>
#include <esp_event.h>
#include <esp_log.h>

#include "my_i2c.h"


static const char* MY_TAG = "khaus/bmp280";


uint8_t m_i2c_read8(uint8_t addr, uint8_t reg);
uint16_t m_i2c_read16(uint8_t addr, uint8_t reg);
uint32_t m_i2c_read24(uint8_t addr, uint8_t reg);


static float m_last_value = 666.666f;


typedef struct {
	  uint16_t dig_T1;
	  int16_t dig_T2;
	  int16_t dig_T3;

	  uint16_t dig_P1;
	  int16_t dig_P2;
	  int16_t dig_P3;
	  int16_t dig_P4;
	  int16_t dig_P5;
	  int16_t dig_P6;
	  int16_t dig_P7;
	  int16_t dig_P8;
	  int16_t dig_P9;
} bmp280_calib_data;


static void bmp280_task()
{
	const uint8_t addr = 0x76;
	uint8_t bytes[24];

	bmp280_calib_data cdata;

	for (;;)
	{
		vTaskDelay(5000 / portTICK_PERIOD_MS);

		ESP_LOGI(MY_TAG, "Scanning...");
		esp_err_t ret = i2c_master_scan(addr);
		if (ret != ESP_OK)
		{
			ESP_LOGE(MY_TAG, "NOT FOUND: 0x%x", addr);
			continue;
		}

		// get id
		uint8_t id = m_i2c_read8(addr, 0xd0);
		ESP_LOGI(MY_TAG, "Device ID: 0x%x", (int)id);
		if (id != 0x58)
		{
			ESP_LOGE(MY_TAG, "WRONG DEVICE ID: 0x%x", addr);
			continue;
		}

		ESP_LOGI(MY_TAG, "Reading calib data...");
		cdata.dig_T1 = m_i2c_read16(addr, 0x88);
		cdata.dig_T2 = (int16_t)m_i2c_read16(addr, 0x8a);
		cdata.dig_T3 = (int16_t)m_i2c_read16(addr, 0x8c);
		cdata.dig_P1 = m_i2c_read16(addr, 0x8e);
		cdata.dig_P2 = (int16_t)m_i2c_read16(addr, 0x90);
		cdata.dig_P3 = (int16_t)m_i2c_read16(addr, 0x92);
		cdata.dig_P4 = (int16_t)m_i2c_read16(addr, 0x94);
		cdata.dig_P5 = (int16_t)m_i2c_read16(addr, 0x96);
		cdata.dig_P6 = (int16_t)m_i2c_read16(addr, 0x98);
		cdata.dig_P7 = (int16_t)m_i2c_read16(addr, 0x9a);
		cdata.dig_P8 = (int16_t)m_i2c_read16(addr, 0x9c);
		cdata.dig_P9 = (int16_t)m_i2c_read16(addr, 0x9e);

		ESP_LOGI(MY_TAG, "T1: 0x%x", cdata.dig_T1);
		ESP_LOGI(MY_TAG, "T2: 0x%x", cdata.dig_T2);
		ESP_LOGI(MY_TAG, "T3: 0x%x", cdata.dig_T3);

		// set settings
		ESP_LOGI(MY_TAG, "Setting settings...");
		bytes[0] = 0xf4;
		bytes[1] = 0x37;
		i2c_master_write_slave(addr, bytes, 2);

		for (;;)
		{
			if (false) {
				uint8_t a = 0xfa;
				uint8_t b1 = m_i2c_read8(addr, a+0);
				uint8_t b2 = m_i2c_read8(addr, a+1);
				uint8_t b3 = m_i2c_read8(addr, a+2);
				uint32_t d = m_i2c_read24(addr, a);
				ESP_LOGI(MY_TAG, " [ 0x%x | 0x%x | 0x%x | 0x%x ] ", b1, b2, b3, d);
			}

			// get temp
			int32_t adc_T = (int32_t)m_i2c_read24(addr, 0xfa);
			ESP_LOGI(MY_TAG, "=>raw=>0x%x", adc_T);
			adc_T >>= 4;

			int32_t var1 = ((((adc_T >> 3) - ((int32_t)cdata.dig_T1 << 1))) *
							((int32_t)cdata.dig_T2)) >> 11;

			int32_t var2 = (((((adc_T >> 4) - ((int32_t)cdata.dig_T1)) *
							  ((adc_T >> 4) - ((int32_t)cdata.dig_T1))) >> 12) *
							((int32_t)cdata.dig_T3)) >> 14;

			int32_t t_fine = var1 + var2;

			float T = (t_fine * 5 + 128) >> 8;
			T /= 100.0f;
			ESP_LOGI(MY_TAG, "Temp: %.2f", T);
//			return T / 100;

			m_last_value = T;

			vTaskDelay(60000 / portTICK_PERIOD_MS);
		}
	}
}


void bmp280_init(void)
{
	xTaskCreate(&bmp280_task, "bmp280_task", 2048, NULL, 5, NULL);
}


uint8_t m_i2c_read8(uint8_t addr, uint8_t reg)
{
	uint8_t bytes[1];

	bytes[0] = reg;
	i2c_master_write_slave(addr, bytes, 1);
	vTaskDelay(1 / portTICK_PERIOD_MS);
	i2c_master_read_slave(addr, bytes, 1);

	return bytes[0];
}


uint16_t m_i2c_read16(uint8_t addr, uint8_t reg)
{
	uint8_t bytes[2];

	bytes[0] = reg;
	i2c_master_write_slave(addr, bytes, 1);
	vTaskDelay(1 / portTICK_PERIOD_MS);
	i2c_master_read_slave(addr, bytes, 2);

	// FIXME order is wrong ???
	return (uint16_t)(bytes[1] << 8) | (uint16_t)(bytes[0] << 0);
}


uint32_t m_i2c_read24(uint8_t addr, uint8_t reg)
{
	uint8_t bytes[3];

	bytes[0] = reg;
	i2c_master_write_slave(addr, bytes, 1);
	vTaskDelay(1 / portTICK_PERIOD_MS);
	i2c_master_read_slave(addr, bytes, 3);

	// FIXME order is wrong ???
	return (uint32_t)(bytes[0] << 16) | (uint32_t)(bytes[1] << 8) | (uint32_t)(bytes[2] << 0);
}


float bmp280_get_temp()
{
	return m_last_value;
}
