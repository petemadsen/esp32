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
#include <esp_http_server.h>

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
#include "my_sleep.h"


void app_main()
{
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

	wifi_init();

	dfplayer_init();

	parser_init();

    xTaskCreate(light_btn_task, "light_btn_task", 2048, NULL, 5, NULL);

	xTaskCreate(shutters_task, "shutters_task", 4096, &parse_input, 5, NULL);

	xTaskCreate(my_sleep_task, "sleep_task", 4096, NULL, 5, NULL);

#if 0
	gpio_wakeup_enable(GPIO_NUM_19, GPIO_INTR_LOW_LEVEL);
	gpio_wakeup_enable(GPIO_NUM_21, GPIO_INTR_LOW_LEVEL);
	for (;;)
	{
		esp_sleep_enable_timer_wakeup(20 * 1000 * 1000);
		esp_sleep_enable_gpio_wakeup();

		int64_t t_enter = esp_timer_get_time();
		esp_light_sleep_start();
		int64_t t_end = esp_timer_get_time();

		printf("--diff-- %lld\n", (t_end - t_enter));
	}
#endif
}
