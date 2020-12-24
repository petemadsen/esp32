/**
 * This code is public domain.
 */
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <esp_system.h>
#include <esp_log.h>

#include <nvs.h>
#include <nvs_flash.h>

#include "config.h"
#include "common.h"
#include "system/wifi.h"
#include "system/my_settings.h"
#include "ota.h"


static const char* MY_TAG = PROJECT_TAG("main");


void app_main()
{
	ESP_ERROR_CHECK(settings_init());

	ota_init();
	ESP_LOGI(MY_TAG, "%s", ota_get_url());
	vTaskDelay(5000 / portTICK_PERIOD_MS);

	wifi_init(true);

	xTaskCreate(&ota_task, "ota_task", 16 * 1024, NULL, 5, NULL);

	xTaskCreate(&ota_nowifi_task, "ota_nowifi_task", 4096, NULL, 5, NULL);
}
