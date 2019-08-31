/**
 * This code is public domain.
 */
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "sdkconfig.h"

#include "buttons.h"
#include "wifi.h"
#include "tone.h"
#include "light.h"
#include "shutters.h"
#include "ota.h"
#include "my_sleep.h"
#include "my_settings.h"
#include "my_i2c.h"
#include "bmp280.h"


RTC_DATA_ATTR uint32_t g_boot_count = 0;

void app_main()
{
	++g_boot_count;

	ESP_ERROR_CHECK(settings_init());

	ota_init();
	vTaskDelay(5000 / portTICK_PERIOD_MS);

	buttons_init();

	wifi_init(true);

	tone_init();

	i2c_master_init();

	bmp280_init();

    xTaskCreate(light_btn_task, "light_btn_task", 2048, NULL, 5, NULL);

	xTaskCreate(shutters_task, "shutters_task", 4096, NULL, 5, NULL);

	xTaskCreate(my_sleep_task, "sleep_task", 4096, NULL, 5, NULL);
}
