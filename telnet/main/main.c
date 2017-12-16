/**
 *
 * Wifi:
 * https://github.com/cmmakerclub/esp32-webserver/tree/master
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

#include "config.h"
#include "telnet.h"


int led_on = 1;

static const char* tag = "MY_HTTP";


esp_err_t event_handler(void *ctx, system_event_t *event)
{
    return ESP_OK;
}


static void wifi_init()
{
    tcpip_adapter_init();

	tcpip_adapter_dhcpc_stop(TCPIP_ADAPTER_IF_STA); // no DHCP

	tcpip_adapter_ip_info_t ipInfo;
	inet_pton(AF_INET, CONFIG_ADDRESS, &ipInfo.ip);
	inet_pton(AF_INET, CONFIG_GATEWAY, &ipInfo.gw);
	inet_pton(AF_INET, CONFIG_NETMASK, &ipInfo.netmask);
	tcpip_adapter_set_ip_info(TCPIP_ADAPTER_IF_STA, &ipInfo);

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
    ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
    ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );
    wifi_config_t sta_config = {
        .sta = {
            .ssid = CONFIG_SSID,
            .password = CONFIG_PASSWORD,
            .bssid_set = false
        }
    };
    ESP_ERROR_CHECK( esp_wifi_set_config(WIFI_IF_STA, &sta_config) );
    ESP_ERROR_CHECK( esp_wifi_start() );
    ESP_ERROR_CHECK( esp_wifi_connect() );

	ESP_LOGI(tag, "Wifi ready.");
}



void read_settings()
{
	nvs_handle my_handle;
	const char* CFG_BOOT_CNT = "bootcnt";
	int8_t bootcnt = 0;
	esp_err_t err;

	err = nvs_open("storage", NVS_READWRITE, &my_handle);
	if (err != ESP_OK)
	{
		ESP_LOGI(tag, "[nvs] could not open storage");
		return;
	}

	err = nvs_get_i8(my_handle, CFG_BOOT_CNT, &bootcnt);
	if (err == ESP_OK)
		++bootcnt;

	err = nvs_set_i8(my_handle, CFG_BOOT_CNT, bootcnt);
	if (err != ESP_OK)
	{
		ESP_LOGI(tag, "[nvs] could not write");
		return;
	}

	err = nvs_commit(my_handle);
	if (err != ESP_OK)
	{
		ESP_LOGE(tag, "[nvs] coult not commit");
		return;
	}

	ESP_LOGI(tag, "[nvs] boot cnt: %d", bootcnt);
}


void app_main(void)
{
	esp_err_t err = nvs_flash_init();
	if (err == ESP_OK)
	{
		read_settings();
	}
	else
		ESP_LOGI(tag, "nvs returned with an error.");

    ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));

	/*
	hcsr04_init();

	for(;;);
	*/
	wifi_init();


	xTaskCreate(&telnet_server, "telnet_server", 2048, NULL, 5, NULL);
//	xTaskCreate(&task_blinking_led, "blinking_led", 2048, NULL, 5, NULL);
}

