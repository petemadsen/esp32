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
#include "ota.h"


static const char* MY_TAG = PROJECT_TAG("main");


void app_main()
{
    // Initialize NVS.
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES)
	{
        // OTA app partition table has a smaller NVS partition size than the
		// non-OTA partition table. This size mismatch may cause NVS
		// initialization to fail.
		// If this happens, we erase NVS partition and initialize NVS again.
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
		ESP_LOGW(MY_TAG, "Initializing VNS.");
    }
    ESP_ERROR_CHECK(err);

	ota_init();
	vTaskDelay(5000 / portTICK_PERIOD_MS);

	wifi_init(false);

	xTaskCreate(&ota_task, "ota_task", 8192, NULL, 5, NULL);

//	xTaskCreate(&nowifi_watch_task, "nowifi_watch_task", 4096, NULL, 5, NULL);
}
