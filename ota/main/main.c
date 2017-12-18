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
#include "ota.h"


static const char* M_TAG = "[aw-ota]";


esp_err_t event_handler(void *ctx, system_event_t *event)
{
    return ESP_OK;
}


void app_main(void)
{
}

