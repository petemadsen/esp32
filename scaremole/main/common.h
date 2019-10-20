/**
 * This code is public domain.
 */
#ifndef PROJECT_COMMON_H
#define PROJECT_COMMON_H

#include <freertos/FreeRTOS.h>


#define GEEKWORK_EASY_KIT_C1	1	// has two leds: 0 and 27
#define GEEKCREIT_DEVKIT_V1		2	// has one led: 2


#define PROJECT_NAME "scaremole"
#define PROJECT_VERSION "2019-10-20/0"
#define PROJECT_BOARD GEEKCREIT_DEVKIT_V1
#define PROJECT_TAG(x) PROJECT_NAME "/" x

#define PROJECT_SHUTTERS_ADDRESS "http://192.168.1.86:8080"
#define PROJECT_SHUTTERS_ADDRESS "http://192.168.1.51:8080"

#define PROJECT_LED_PIN GPIO_NUM_2
#define PIN_BUZZER 14
#define PIN_MOTOR 12

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
