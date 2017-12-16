/**
 *
 * Wifi:
 * https://github.com/cmmakerclub/esp32-webserver/tree/master
 *
 *
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


esp_err_t event_handler(void *ctx, system_event_t *event)
{
    return ESP_OK;
}


static int16_t ina219_get(uint8_t dev_addr, uint8_t feature, uint16_t data_byte)
{
	uint8_t data[3];

	data[0] = feature;
	data[1] = data_byte >> 8; // high byte
	data[2] = data_byte; // low byte
	i2c_master_write_slave(dev_addr, data, 3);

	vTaskDelay(1 / portTICK_RATE_MS);

	i2c_master_read_slave(dev_addr, data, 2);

	uint16_t ret = (data[0] << 8) | data[1];
	return ret;
}

#define INA219_CMD_CONFIG			0x0
#define INA219_CMD_SHUNT_VOLTAGE	0x1
#define INA219_CMD_BUS_VOLTAGE		0x2
#define INA219_CMD_SHUNT_CURRENT	0x4
#define INA219_CMD_CALIBRATION		0x5

static void ina219_i2c_task()
{
	const uint8_t addr = 0x40;

	// configuration
	uint16_t config = 0;
	config |= (0x1 << 13);	// BRNG=1 => 32v
	config |= (0x3 << 11);	// PG1=1, PG0=1 => +-80mV
	config |= (0x3 << 7);	// BADC2=1 BADC1=1 => 10bit
	config |= (0x3 << 3);	// SADC2=1 SADC1=1 => 10bit
	config |= (0x7 << 0);	// MODE => shunt, bus, continous
	ESP_LOGI(M_TAG, "CONFIG: %u", config);
	config = ina219_get(addr, INA219_CMD_CONFIG, config);
	ESP_LOGI(M_TAG, "CONFIG: %u", config);

	// calibration
	const float r_shunt = 0.1;
	const float d_bus_vmax = 32.0;
	const float d_shut_vmax = 0.2;
	const float d_max_iexpected = 2.0;

	float current_lsb = d_max_iexpected / 32768.0;
	uint16_t calibration = 0.04096 / (current_lsb * r_shunt);
	calibration = 5851;
	ESP_LOGI(M_TAG, "CALIBRATION: %u", calibration);
	calibration = ina219_get(addr, INA219_CMD_CALIBRATION, calibration);
	ESP_LOGI(M_TAG, "CALIBRATION: %u", calibration);

	vTaskDelay(5000 / portTICK_PERIOD_MS);

	for (;;)
	{
		int16_t shunt_current_reg = ina219_get(addr, INA219_CMD_SHUNT_CURRENT, 0);
		int16_t shunt_voltage_reg = ina219_get(addr, INA219_CMD_SHUNT_VOLTAGE, 0);
		int16_t bus_voltage_reg = ina219_get(addr, INA219_CMD_BUS_VOLTAGE, 0);

		float shunt_voltage = (float)shunt_voltage_reg * 0.000010; // 10uV
		float bus_voltage = (float)(bus_voltage_reg >> 3) * 0.004; // 4mV
		float shunt_current_estimated = shunt_voltage / r_shunt;
		float shunt_current = shunt_current_reg * current_lsb;

		ESP_LOGI(M_TAG, "=>");
		ESP_LOGI(M_TAG, "..shunt_voltage..: %u / %.3f", shunt_voltage_reg, shunt_voltage);
		ESP_LOGI(M_TAG, "..bus_voltage....: %u / %.3f V", bus_voltage_reg, bus_voltage);
		ESP_LOGI(M_TAG, "..shunt_current..: %u / %.3f A / %.3f A", shunt_current_reg, shunt_current_estimated, shunt_current);

		vTaskDelay(5000 / portTICK_PERIOD_MS);
	}
}


void app_main(void)
{
	ESP_ERROR_CHECK(i2c_master_init(GPIO_NUM_19, GPIO_NUM_21));
	xTaskCreate(&ina219_i2c_task, "ina219_i2c_task", 2048, NULL, 5, NULL);
}

