/**
 * This code is public domain.
 */
#include "sdkconfig.h"

#include <string.h>
#include <sys/socket.h>
#include <netdb.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>

#include <esp_system.h>
#include <esp_log.h>

#include <nvs_flash.h>

#include "config.h"
#include "wifi.h"
#include "shutters.h"
#include "my_sleep.h"
#include "ota.h"
#include "voltage.h"
#include "scaremole.h"


static const char* MY_TAG = "scaremole/main";


void app_main()
{
	scaremole_run();

	ESP_ERROR_CHECK(nvs_flash_init());
	wifi_init(true);

	ota_init();

	voltage_init();

	xTaskCreate(shutters_task, "shutters_task", 4096, NULL, 5, NULL);

	xTaskCreate(my_sleep_task, "sleep_task", 4096, NULL, 5, NULL);
}
