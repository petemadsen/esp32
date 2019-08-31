/**
 * Bluetooth:
 * https://github.com/espressif/esp-idf/blob/master/examples/bluetooth/ble_adv
 *
 */
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <esp_wifi.h>
#include <esp_system.h>
#include <esp_event.h>
#include <esp_event_loop.h>
#include <esp_log.h>
#include <nvs_flash.h>
#include <driver/gpio.h>

#include <lwip/netdb.h>

#include "sdkconfig.h"

#include "config.h"
#include "i2c.h"


int led_on = 1;

static const char* M_TAG = "my_i2c";


typedef struct {
	  uint16_t dig_T1; /**< dig_T1 cal register. */
	  int16_t dig_T2;  /**<  dig_T2 cal register. */
	  int16_t dig_T3;  /**< dig_T3 cal register. */

	  uint16_t dig_P1; /**< dig_P1 cal register. */
	  int16_t dig_P2;  /**< dig_P2 cal register. */
	  int16_t dig_P3;  /**< dig_P3 cal register. */
	  int16_t dig_P4;  /**< dig_P4 cal register. */
	  int16_t dig_P5;  /**< dig_P5 cal register. */
	  int16_t dig_P6;  /**< dig_P6 cal register. */
	  int16_t dig_P7;  /**< dig_P7 cal register. */
	  int16_t dig_P8;  /**< dig_P8 cal register. */
	  int16_t dig_P9;  /**< dig_P9 cal register. */
} bmp280_calib_data;


static void bmp280_task()
{
	const uint8_t addr = 0x76;
	uint8_t bytes[24];

	bmp280_calib_data cdata;

	for (;;)
	{
		vTaskDelay(5000 / portTICK_PERIOD_MS);

		ESP_LOGI(M_TAG, "Scanning...");
		esp_err_t ret = i2c_master_scan(addr);
		if (ret != ESP_OK)
		{
			ESP_LOGE(M_TAG, "NOT FOUND: 0x%x", addr);
			continue;
		}

		// get id
		uint8_t id = m_i2c_read8(addr, 0xd0);
		ESP_LOGI(M_TAG, "Device ID: 0x%x", (int)id);
		if (id != 0x58)
		{
			ESP_LOGE(M_TAG, "WRONG DEVICE ID: 0x%x", addr);
			continue;
		}

		ESP_LOGI(M_TAG, "Reading calib data...");
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

		ESP_LOGI(M_TAG, "T1: 0x%x", cdata.dig_T1);
		ESP_LOGI(M_TAG, "T2: 0x%x", cdata.dig_T2);
		ESP_LOGI(M_TAG, "T3: 0x%x", cdata.dig_T3);

		// set settings
		ESP_LOGI(M_TAG, "Setting settings...");
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
				ESP_LOGI(M_TAG, " [ 0x%x | 0x%x | 0x%x | 0x%x ] ", b1, b2, b3, d);
			}

			// get temp
			int32_t adc_T = (int32_t)m_i2c_read24(addr, 0xfa);
			ESP_LOGI(M_TAG, "=>raw=>0x%x", adc_T);
			adc_T >>= 4;

			int32_t var1 = ((((adc_T >> 3) - ((int32_t)cdata.dig_T1 << 1))) *
							((int32_t)cdata.dig_T2)) >> 11;

			int32_t var2 = (((((adc_T >> 4) - ((int32_t)cdata.dig_T1)) *
							  ((adc_T >> 4) - ((int32_t)cdata.dig_T1))) >> 12) *
							((int32_t)cdata.dig_T3)) >> 14;

			int32_t t_fine = var1 + var2;

			float T = (t_fine * 5 + 128) >> 8;
			T /= 100.0f;
			ESP_LOGI(M_TAG, "Temp: %.2f", T);
//			return T / 100;

			vTaskDelay(5000 / portTICK_PERIOD_MS);
		}
	}
}


void app_main(void)
{
	ESP_ERROR_CHECK(i2c_master_init(GPIO_NUM_21, GPIO_NUM_22));
	xTaskCreate(&bmp280_task, "bmp280_task", 2048, NULL, 5, NULL);
}

