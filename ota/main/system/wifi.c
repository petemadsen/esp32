/**
 * This code is public domain. Have fun.
 */
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <esp_system.h>
#include <esp_wifi.h>
#include <esp_event.h>
#include <esp_log.h>

#include <driver/gpio.h>

#include <lwip/netdb.h>
#include <lwip/sockets.h>
#include <lwip/dns.h>

#include <esp_image_format.h>

#include "my_http.h"
#include "wifi.h"

#include "config.h"
#include "common.h"


static const char* MY_TAG = PROJECT_TAG("wifi");



EventGroupHandle_t wifi_event_group;
const EventBits_t WIFI_CONNECTED = BIT0;
const EventBits_t WIFI_DISCONNECTED = BIT1;

static bool reconnect = true;
static bool run_httpd = true;

static httpd_handle_t server = NULL;



static void event_handler(void* arg, esp_event_base_t event_base,
								int32_t event_id, void* event_data)
{
	httpd_handle_t* http = (httpd_handle_t*)arg;

	if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
	{
		ESP_LOGI(MY_TAG, "Connecting...");
		gpio_set_level(PROJECT_LED_PIN, PROJECT_LED_PIN_OFF);

		esp_wifi_connect();
	}
	else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
	{
		ESP_LOGW(MY_TAG, "Disconnected");
		gpio_set_level(PROJECT_LED_PIN, PROJECT_LED_PIN_OFF);
		xEventGroupClearBits(wifi_event_group, WIFI_CONNECTED);

		if (reconnect)
		{
			esp_wifi_connect();
			ESP_LOGI(MY_TAG, "Retry to connect to the AP");
		}

		if (*http)
		{
			http_stop(http);
			http = NULL;
		}
	}
	else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
	{
		ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
		ESP_LOGI(MY_TAG, "Got ip: " IPSTR, IP2STR(&event->ip_info.ip));
		gpio_set_level(PROJECT_LED_PIN, PROJECT_LED_PIN_ON);
		xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED);

		if (run_httpd && *http == NULL)
		{
			*http = http_start();
		}

		// set DNS
#if 0
		{
			ip_addr_t d;
			d.type = IPADDR_TYPE_V4;
			d.u_addr.ip4.addr = 0x0101a8c0; // 1 1 168 192 dns
			dns_setserver(0, &d);

//			ip_addr_t dns_addr;
//			ip_addr_set_ip4_u32(&dns_addr, htonl(dhcp_get_option_value(dhcp, DHCP_OPTION_IDX_DNS_SERVER + n)));
//			dns_setserver(n, &dns_addr);
		}
#endif
	}
}


void wifi_init(bool fixed_ip)
{
	// -- status led
	gpio_pad_select_gpio(PROJECT_LED_PIN);
	gpio_set_direction(PROJECT_LED_PIN, GPIO_MODE_OUTPUT);

	// -- wifi
	ESP_ERROR_CHECK(esp_netif_init());
	ESP_ERROR_CHECK(esp_event_loop_create_default());

	if (fixed_ip)
	{
		esp_netif_t* my_sta = esp_netif_create_default_wifi_sta();
		esp_netif_dhcpc_stop(my_sta);
//		tcpip_adapter_dhcpc_stop(TCPIP_ADAPTER_IF_STA); // no DHCP

		esp_netif_ip_info_t ipInfo;
		inet_pton(AF_INET, CONFIG_ADDRESS, &ipInfo.ip);
		inet_pton(AF_INET, CONFIG_GATEWAY, &ipInfo.gw);
		inet_pton(AF_INET, CONFIG_NETMASK, &ipInfo.netmask);
		esp_netif_set_ip_info(my_sta, &ipInfo);
	}

	wifi_event_group = xEventGroupCreate();
	ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, &server));
	ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, &server));

	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&cfg));
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

	// save power
//	esp_wifi_set_ps(WIFI_PS_MIN_MODEM);
	esp_wifi_set_ps(WIFI_PS_MAX_MODEM);
}


void wifi_stop()
{
	reconnect = false;
	ESP_ERROR_CHECK(esp_wifi_stop());
}
