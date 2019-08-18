/**
 * This code is public domain.
 */
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <esp_log.h>
#include <driver/gpio.h>

#include "sdkconfig.h"

#include "buttons.h"
#include "wifi.h"
#include "dfplayer.h"
#include "light.h"
#include "telnet.h"
#include "parser.h"

static const char* MY_TAG = "DFPLAYER/main";


void app_main()
{
	buttons_init();

	wifi_init();

	dfplayer_init();

	parser_init();

    xTaskCreate(light_btn_task, "light_btn_task", 2048, NULL, 5, NULL);

	xTaskCreate(telnet_server, "telnet_task", 4096, &parse_input, 5, NULL);
}
