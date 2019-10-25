/**
 * This code is public domain.
 */
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "system/ota.h"
#include "system/wifi.h"
#include "system/my_settings.h"
#include "system/my_i2c.h"

#include "buttons.h"
#include "tone.h"
#include "my_shutters.h"
#include "my_sleep.h"
#include "my_sensors.h"
#include "my_lights.h"
#include "common.h"


void app_main()
{
	ESP_ERROR_CHECK(settings_init());

	ota_init();
//	vTaskDelay(5000 / portTICK_PERIOD_MS);

	buttons_init();

	wifi_init(true);

	tone_init();

	i2c_master_init();

	xTaskCreate(my_sensors_task, "sensors_task", 4096, NULL, 5, NULL);

	xTaskCreate(my_lights_task, "lights_task", 4096, NULL, 5, NULL);

	xTaskCreate(shutters_task, "shutters_task", 4096, NULL, 5, NULL);

	xTaskCreate(my_sleep_task, "sleep_task", 4096, NULL, 5, NULL);
}
