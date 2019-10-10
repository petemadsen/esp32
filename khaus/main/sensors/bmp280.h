/**
 * This code is public domain.
 */
#ifndef PROJECT_BMP280_H
#define PROJECT_BMP280_H

#include <freertos/FreeRTOS.h>


struct bmp280_device
{
	uint8_t addr;
	bool initialized;
	struct {
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
	} cdata;
};


void bmp280_init(struct bmp280_device* dev);


double bmp280_get_temp(struct bmp280_device* dev);


#endif
