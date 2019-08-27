/**
 *
 * Wifi:
 * https://github.com/cmmakerclub/esp32-webserver/tree/master
 *
 *
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
#include "ina219.h"
#include "i2c.h"


static const char* M_TAG = "ina219/main";


void app_main(void)
{
	ESP_ERROR_CHECK(i2c_master_init(GPIO_NUM_19, GPIO_NUM_21));
	xTaskCreate(&ina219_task, "ina219_task", 2048, NULL, 5, NULL);
}
