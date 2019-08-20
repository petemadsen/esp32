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

#include <nvs.h>
#include <nvs_flash.h>


#include "sdkconfig.h"

#include "buttons.h"
#include "wifi.h"
#include "dfplayer.h"
#include "light.h"
#include "parser.h"
#include "shutters.h"


static const char* MY_TAG = "knusperhaeuschen/main";
#include "ota.h"
#include "sntp.h"


void app_main()
{
	static httpd_handle_t server = NULL;

    // -- initialize nvs.
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES)
	{
		ESP_LOGE(MY_TAG, "NVC no free pages.");
        // OTA app partition table has a smaller NVS partition size than the
		// non-OTA partition table. This size mismatch may cause NVS
		// initialization to fail. If this happens, we erase NVS partition
		// and initialize NVS again.
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

	ESP_LOGI(MY_TAG, "Init OTA.");
	ota_init();
	ESP_LOGI(MY_TAG, "Done.");
	vTaskDelay(5000 / portTICK_PERIOD_MS);

	buttons_init();

	wifi_init(&server);

	dfplayer_init();

	parser_init();

    xTaskCreate(light_btn_task, "light_btn_task", 2048, NULL, 5, NULL);

	xTaskCreate(shutters_task, "shutters_task", 4096, &parse_input, 5, NULL);
}
