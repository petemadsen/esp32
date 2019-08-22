/**
 * This code is public domain.
 */
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <stdio.h>
#include <stdlib.h>

#include <esp_log.h>
#include <esp_http_server.h>

//#include <esp_bt.h>
#include <driver/adc.h>

#include <driver/gpio.h>

#include "sdkconfig.h"

#include "buttons.h"
#include "wifi.h"
#include "dfplayer.h"
#include "light.h"
#include "shutters.h"
#include "ota.h"
#include "my_sleep.h"
#include "my_settings.h"


static const char* MY_TAG = "knusperhaeuschen/main";

RTC_DATA_ATTR uint32_t g_boot_count = 0;

void app_main()
{
	++g_boot_count;

	ESP_ERROR_CHECK(settings_init());

	ESP_LOGI(MY_TAG, "Init OTA.");
	ota_init();
	ESP_LOGI(MY_TAG, "Done.");
	vTaskDelay(5000 / portTICK_PERIOD_MS);

	// save power
//	esp_bt_controller_disable();
	// save power
//	adc_power_off();

	buttons_init();

	wifi_init(true);

	dfplayer_init();

    xTaskCreate(light_btn_task, "light_btn_task", 2048, NULL, 5, NULL);

	xTaskCreate(shutters_task, "shutters_task", 4096, NULL, 5, NULL);

	xTaskCreate(my_sleep_task, "sleep_task", 4096, NULL, 5, NULL);
}
