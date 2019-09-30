/**
 * This code is public domain.
 */
#ifndef PROJECT_COMMON_H
#define PROJECT_COMMON_H


#define PROJECT_VERSION "2019-09-30/0"
#define PROJECT_NAME = "khaus"


#define PROJECT_TONE_ONOFF_PIN		GPIO_NUM_32
#define PROJECT_BELL_BTN_PIN		GPIO_NUM_18
#define PROJECT_LIGHT_BTN_PIN		GPIO_NUM_19
#define PROJECT_LIGHT_RELAY_PIN		GPIO_NUM_23
#define PROJECT_LED_PIN		GPIO_NUM_2


#define PROJECT_I2C_BOARD_TEMP	0x76

//
#define MIN(a, b) \
	({ __typeof__(a) _a = (a); __typeof__(b) _b = (b); (_a < _b) ? _a : _b; })



#endif
