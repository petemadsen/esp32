/**
 * This code is public domain.
 */
#include "sdkconfig.h"

#include <string.h>
#include <sys/socket.h>
#include <netdb.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <esp_system.h>
#include <esp_event_loop.h>
#include <esp_log.h>
#include <esp_flash_partitions.h>
#include <esp_partition.h>
#include <esp_sleep.h>

#include <nvs.h>
#include <nvs_flash.h>

#include "config.h"
#include "common.h"
#include "system/wifi.h"
#include "ota.h"


static const char* MY_TAG = PROJECT_TAG("main");



#if 0
// Event group to signal when we are connected & ready to make a request */
static EventGroupHandle_t wifi_event_group;
static const EventBits_t CONNECTED_BIT = BIT0;


static esp_err_t event_handler(void *ctx, system_event_t *event)
{
    switch (event->event_id)
	{
    case SYSTEM_EVENT_STA_START:
        esp_wifi_connect();
        break;
    case SYSTEM_EVENT_STA_GOT_IP:
        xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
        /* This is a workaround as ESP32 WiFi libs don't currently
           auto-reassociate. */
        esp_wifi_connect();
        xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
        break;
    default:
        break;
    }
    return ESP_OK;
}


static void initialise_wifi(void)
{
    tcpip_adapter_init();
    wifi_event_group = xEventGroupCreate();
	ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&cfg) );
	ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    wifi_config_t wifi_config = {
        .sta = {
			.ssid = CONFIG_SSID,
			.password = CONFIG_PASS,
        },
    };
	ESP_LOGI(MY_TAG, "Setting WiFi configuration SSID %s...", wifi_config.sta.ssid);
	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
	ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
	ESP_ERROR_CHECK(esp_wifi_start());
}
#endif


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

	wifi_init(true);

	xTaskCreate(&ota_task, "ota_task", 8192, NULL, 5, NULL);

//	xTaskCreate(&nowifi_watch_task, "nowifi_watch_task", 4096, NULL, 5, NULL);
}
