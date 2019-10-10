/**
 * This code is public domain.
 */
#include "dht12.h"

#include "../my_i2c.h"


int dht12_get(uint8_t addr, double* temp, double* humidity)
{
	esp_err_t err;
	uint8_t raw[5];

	// read
	raw[0] = 0;
	err = i2c_master_write_slave(addr, raw, 1);
	if (err != ESP_OK)
		return 1;
	vTaskDelay(1 / portTICK_PERIOD_MS);
	err = i2c_master_read_slave(addr, raw, 5);
	if (err != ESP_OK)
		return 2;

	// check sum
	if (raw[4] != (raw[0]+raw[1]+raw[2]+raw[3]))
		return 3;

	// temp
	if (temp)
	{
		*temp = raw[2] + (double)raw[3] / 10.0;
	}

	// humidity
	if (humidity)
	{
		*humidity = raw[0] + (double)raw[1] / 10.0;
	}

	return 0;
}
