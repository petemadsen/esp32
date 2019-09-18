#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "sdkconfig.h"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>

#include <esp_wifi.h>
#include <esp_system.h>
#include <esp_event.h>
#include <esp_event_loop.h>
#include <esp_log.h>
#include <driver/gpio.h>

#include <lwip/netdb.h>
#include <lwip/sockets.h>

#include <tcpip_adapter.h>

#include <nvs_flash.h>

#include <mdns.h>

#include "config.h"
#include "telnet.h"
#include "parser.h"
#include "colors.h"
#include "msgeq7.h"


const char* M_TAG = "[awXmasTree]";

static const char* M_HOSTNAME = "xmastree";
static const char* M_MDNS_INST = "awXmasTree";


/****************************************************************************
 *
 */
static void start_mdns_service()
{
	//initialize mDNS service
	esp_err_t err = mdns_init();
	if (err) {
		ESP_LOGE(M_TAG, "mDNS failed: %d", err);
		return;
	}

	//set hostname
	mdns_hostname_set(M_HOSTNAME);
	//set default instance
	mdns_instance_name_set(M_MDNS_INST);
}


/****************************************************************************
 *
 */
/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;
/* The event group allows multiple bits for each event, but we only care about one event
 * - are we connected to the AP with an IP? */
static const int WIFI_IP4_CONNECTED_BIT = BIT0;
static const int WIFI_IP6_CONNECTED_BIT = BIT1;
unsigned int g_wifi_reconnects = 0;


esp_err_t event_handler(void *ctx, system_event_t *event)
{
	switch(event->event_id) {
	case SYSTEM_EVENT_STA_START:
		ESP_LOGI(M_TAG, "Connecting...");
		esp_wifi_connect();
		break;
	case SYSTEM_EVENT_STA_CONNECTED:
		/* enable ipv6 */
		tcpip_adapter_create_ip6_linklocal(TCPIP_ADAPTER_IF_STA);
		break;
	case SYSTEM_EVENT_STA_GOT_IP:
		ESP_LOGI(M_TAG, "Got ip %s", ip4addr_ntoa(&event->event_info.got_ip.ip_info.ip));
		xEventGroupSetBits(s_wifi_event_group, WIFI_IP4_CONNECTED_BIT);
//		start_mdns_service();
		break;
	case SYSTEM_EVENT_AP_STA_GOT_IP6:
		ESP_LOGI(M_TAG, "Got ip6");
		xEventGroupSetBits(s_wifi_event_group, WIFI_IP6_CONNECTED_BIT);
	break;
	case SYSTEM_EVENT_STA_DISCONNECTED:
		g_wifi_reconnects++;
		esp_wifi_connect();
		xEventGroupClearBits(s_wifi_event_group, WIFI_IP4_CONNECTED_BIT | WIFI_IP6_CONNECTED_BIT);
		ESP_LOGI(M_TAG,"Reconnecting to the AP (try: %u)", g_wifi_reconnects);
		break;
	default:
		break;
	}

//	mdns_handle_system_event(ctx, event);

    return ESP_OK;
}


static void wifi_init()
{
	s_wifi_event_group = xEventGroupCreate();

    tcpip_adapter_init();

	tcpip_adapter_dhcpc_stop(TCPIP_ADAPTER_IF_STA); // no DHCP

	tcpip_adapter_ip_info_t ipInfo;
	inet_pton(AF_INET, CONFIG_ADDRESS, &ipInfo.ip);
	inet_pton(AF_INET, CONFIG_GATEWAY, &ipInfo.gw);
	inet_pton(AF_INET, CONFIG_NETMASK, &ipInfo.netmask);
	tcpip_adapter_set_ip_info(TCPIP_ADAPTER_IF_STA, &ipInfo);

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    wifi_config_t sta_config = {
        .sta = {
            .ssid = CONFIG_SSID,
            .password = CONFIG_PASSWORD,
            .bssid_set = false
        }
    };
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &sta_config));
    ESP_ERROR_CHECK(esp_wifi_start());

	ESP_LOGI(M_TAG, "Wifi ready.");
}


/****************************************************************************
 *
 */
SemaphoreHandle_t xButtonSem = NULL;
#define CONFIG_BUTTON_PIN 0 /* FIXME: can we have this in sdkconfig? */

void IRAM_ATTR button_isr_handler(void* arg)
{
	xSemaphoreGiveFromISR(xButtonSem, NULL);
}
static void button_task(void* arg)
{
	xButtonSem = xSemaphoreCreateBinary();

	gpio_pad_select_gpio(CONFIG_BUTTON_PIN);
	gpio_set_direction(CONFIG_BUTTON_PIN, GPIO_MODE_INPUT);
	gpio_set_intr_type(CONFIG_BUTTON_PIN, GPIO_INTR_NEGEDGE);
	gpio_install_isr_service(0); //ESP_INTR_FLAG_DEFAULT
	gpio_isr_handler_add(CONFIG_BUTTON_PIN, button_isr_handler, NULL);

	ESP_LOGE(M_TAG, "button-task.");
	for (;;)
	{
		if (xSemaphoreTake(xButtonSem, portMAX_DELAY) == true)
		{
			ESP_LOGI(M_TAG, "button pressed");
			colors_next_mode();
		}
	}
}



void app_main(void)
{
	ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));

	colors_init();

	parser_init();

	wifi_init();
	xTaskCreate(&telnet_task, "telnet_task", 4096, &parse_input, 5, NULL);

	msgeq7_init();

//    xTaskCreate(button_task, "button_task", 1024, NULL, 10, NULL);
}

