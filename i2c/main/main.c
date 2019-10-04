/**
 * Bluetooth:
 * https://github.com/espressif/esp-idf/blob/master/examples/bluetooth/ble_adv
 *
 */
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <esp_wifi.h>
#include <esp_system.h>
#include <esp_event.h>
#include <esp_event_loop.h>
#include <esp_log.h>
#include <nvs_flash.h>
#include <driver/gpio.h>

#include <lwip/netdb.h>

#include "sdkconfig.h"

#include "config.h"
#include "i2c.h"


int led_on = 1;

static const char* M_TAG = "my_i2c";


static void task_i2c_scan()
{
	for (;;)
	{
		ESP_LOGI(M_TAG, "Scanning...");
		for (int addr=0x00; addr<0xff; ++addr)
		{
			esp_err_t ret = i2c_master_scan(addr);
			if (ret == ESP_OK)
				ESP_LOGI(M_TAG, "FOUND: 0x%x", addr);
		}
		vTaskDelay(1000 / portTICK_PERIOD_MS);
	}
}


void app_main(void)
{
	ESP_ERROR_CHECK(i2c_master_init(GPIO_NUM_21, GPIO_NUM_22));
	xTaskCreate(&task_i2c_scan, "task_i2c_scan", 2048, NULL, 5, NULL);
}

