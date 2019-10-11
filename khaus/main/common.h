/**
 * This code is public domain.
 */
#ifndef PROJECT_COMMON_H
#define PROJECT_COMMON_H

#include <freertos/FreeRTOS.h>


#define GEEKWORK_EASY_KIT_C1	1	// has two leds: 0 and 27
#define GEEKCREIT_DEVKIT_V1		2	// has one led: 2


#define PROJECT_NAME "khaus"
#define PROJECT_VERSION "2019-10-03/0"
#define PROJECT_BOARD GEEKCREIT_DEVKIT_V1
#define PROJECT_TAG(x) PROJECT_NAME "/" x

#define PROJECT_SHUTTERS_ADDRESS "http://192.168.1.51:8080"
#define PROJECT_SHUTTERS_ADDRESS "http://192.168.1.86:8080"


#define PROJECT_TONE_ONOFF_PIN		GPIO_NUM_32
#define PROJECT_BELL_BTN_PIN		GPIO_NUM_17
#define PROJECT_LIGHT_BTN_PIN		GPIO_NUM_19
#define PROJECT_LIGHT_RELAY_PIN		GPIO_NUM_23
#define PROJECT_LED_PIN				GPIO_NUM_2
#define PROJECT_VOLTAGE_PIN			ADC1_GPIO34_CHANNEL
#define PROJECT_WS2812B_PIN			GPIO_NUM_18 // FIXME


#define PROJECT_I2C_BOARD_BMP280	0x77 // FIXME: need to change
#define PROJECT_I2C_OUT_BMP280		0x76
#define PROJECT_I2C_OUT_DHT12		0x5c

//
#define MIN(a, b) \
	({ __typeof__(a) _a = (a); __typeof__(b) _b = (b); (_a < _b) ? _a : _b; })


#if PROJECT_BOARD == GEEKWORK_EASY_KIT_C1
#define PROJECT_LED_PIN		GPIO_NUM_0
#define PROJECT_LED_PIN_ON	0
#define PROJECT_LED_PIN_OFF	1
#elif PROJECT_BOARD == GEEKCREIT_DEVKIT_V1
#define PROJECT_LED_PIN		GPIO_NUM_2
#define PROJECT_LED_PIN_ON	1
#define PROJECT_LED_PIN_OFF	0
#else
#error Specify PROJECT_BOARD, please!
#endif



#endif
