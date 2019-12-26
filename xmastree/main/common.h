/**
 * This code is public domain.
 */
#ifndef PROJECT_COMMON_H
#define PROJECT_COMMON_H

#include <freertos/FreeRTOS.h>
#include "config.h"


#define GEEKWORK_EASY_KIT_C1	1	// has two leds: 0 and 27
#define GEEKCREIT_DEVKIT_V1		2	// has one led: 2


#define PROJECT_NAME "xmastree"
#define PROJECT_VERSION "2019-12-26/0"
#define PROJECT_BOARD GEEKCREIT_DEVKIT_V1
#define PROJECT_TAG(x) PROJECT_NAME "/" x

#ifdef PROJECT_DEBUG
#define PROJECT_SHUTTERS_ADDRESS "http://192.168.1.86:8080"
#else
#define PROJECT_SHUTTERS_ADDRESS "http://192.168.1.51:8080"
#endif

extern unsigned int g_wifi_reconnects;


#define PROJECT_LED_PIN		GPIO_NUM_2


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
