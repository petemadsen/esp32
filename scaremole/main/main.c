/**
 * This code is public domain.
 */
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <esp_system.h>
#include <esp_log.h>

#include <nvs_flash.h>

#include "system/my_settings.h"

#include "config.h"
#include "wifi.h"
#include "my_shutters.h"
#include "my_sleep.h"
#include "ota.h"
#include "voltage.h"
#include "scaremole.h"
#include "oled.h"
#include "ds18b20.h"


#define MODE_TEMP


static const char* MY_TAG = "scaremole/main";


static void main_task(void* params)
{
	uint8_t addr = OLED_INVALID_DEVICE;

	for (;;)
	{
		addr = oled_init();
		if (addr != OLED_INVALID_DEVICE)
			break;

		ESP_LOGE(MY_TAG, "Could not find an OLED display.");
		vTaskDelay(60 * 1000 / portTICK_PERIOD_MS);
	}

	ESP_LOGI(MY_TAG, "OLED at: 0x%02x", addr);

	char buffer[20];

	for (;;)
	{
		ESP_LOGI(MY_TAG, "--RUN");

		float temp = ds18b20_last_temp();
		snprintf(buffer, sizeof(buffer), "%.2f °C", temp);
		ESP_LOGI(MY_TAG, "[DISPLAY] %s", buffer);

		oled_clear(addr);
		oled_print(buffer);
		oled_flush(addr);

		vTaskDelay(60 * 1000 / portTICK_PERIOD_MS);
	}
}


void app_main()
{
	ESP_ERROR_CHECK(settings_init());

#ifdef MODE_TEMP
	wifi_init(true);

	voltage_init();

//	xTaskCreate(shutters_task, "shutters_task", 4096, NULL, 5, NULL);

	xTaskCreate(main_task, "main_task", 4096, NULL, 5, NULL);

	xTaskCreate(ds18b20_task, "ds18b20_task", 4096, NULL, 5, NULL);

#else
	scaremole_run();

	// we got business to do
	if ((settings_boot_counter() % 6) == 0)
	{
		wifi_init(true);

		ota_init();

		voltage_init();

		xTaskCreate(shutters_task, "shutters_task", 4096, NULL, 5, NULL);

		xTaskCreate(my_sleep_task, "sleep_task", 4096, NULL, 5, NULL);
	}
	else
	{
		my_sleep_now("next-time");
	}
#endif
}
