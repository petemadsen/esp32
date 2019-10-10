/**
 * This code is public domain.
 */
#include "dht12.h"

#include "../my_i2c.h"


int dht12_get(uint8_t addr, double* temp, double* humidity)
{
	esp_err_t err;
	uint8_t datos[5];

	// read
	datos[0] = 0;
	err = i2c_master_write_slave(addr, datos, 1);
	if (err != ESP_OK)
		return 1;
	vTaskDelay(1 / portTICK_PERIOD_MS);
	err = i2c_master_read_slave(addr, datos, 5);
	if (err != ESP_OK)
		return 2;

	// check sum
	if (datos[4] != (datos[0]+datos[1]+datos[2]+datos[3]))
		return 3;

	// temp
	if (temp)
	{
		*temp = datos[2] + (double)datos[3] / 10.0;
	}

	// humidity
	if (humidity)
	{
		*humidity = datos[0] + (double)datos[1] / 10.0;
	}

	return 0;
}
