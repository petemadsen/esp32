/**
 * This code is public domain.
 */
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <esp_log.h>

#include "my_sensors.h"

#include "sensors/voltage.h"
#include "sensors/bmp280.h"
#include "sensors/dht12.h"

#include "common.h"


static const char* MY_TAG = PROJECT_TAG("sensors");


static double m_voltage = 0.0;
static double m_board_temp = 666.6;
static double m_out_temp = 666.6;
static double m_out_humidity = 0;


void my_sensors_task(void* args)
{
	voltage_init(PROJECT_VOLTAGE_PIN);

	struct bmp280_device board_temp;
	board_temp.addr = PROJECT_I2C_BOARD_BMP280;
	bmp280_init(&board_temp);

	struct bmp280_device out_temp;
	out_temp.addr = PROJECT_I2C_OUT_BMP280;
	bmp280_init(&out_temp);

	for (;;)
	{
		m_voltage = voltage_get();
		ESP_LOGI(MY_TAG, "Voltage: %.2f", m_voltage);

		if (board_temp.initialized)
		{
			m_board_temp = bmp280_get_temp(&board_temp);
			ESP_LOGI(MY_TAG, "BoardTemp: %.2f", m_board_temp);
		}
		else
		{
			ESP_LOGI(MY_TAG, "BoardTemp: init");
			bmp280_init(&board_temp);
		}

		if (out_temp.initialized)
		{
			m_out_temp = bmp280_get_temp(&out_temp);
			ESP_LOGI(MY_TAG, "OutTemp: %.2f", m_out_temp);
		}
		else
		{
			ESP_LOGI(MY_TAG, "OutTemp: init");
			bmp280_init(&out_temp);
		}

		int ret = dht12_get(PROJECT_I2C_OUT_DHT12, NULL, &m_out_humidity);
		if (ret != 0)
			ESP_LOGE(MY_TAG, "DHT12: error %d", ret);

		// -- 60 secs
		vTaskDelay(60 * 1000 / portTICK_PERIOD_MS );
	}
}


double my_sensors_board_voltage()
{
	return m_voltage;
}


double my_sensors_board_temp()
{
	return m_board_temp;
}


double my_sensors_out_temp()
{
	return m_out_temp;
}


double my_sensors_out_humidity()
{
	return m_out_humidity;
}
