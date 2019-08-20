/**
 * This code is public domain. Have fun.
 */
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <esp_system.h>
#include <esp_wifi.h>
#include <esp_event_loop.h>
#include <esp_log.h>

#include <lwip/netdb.h>
#include <lwip/sockets.h>
#include <lwip/dns.h>

#include <esp_image_format.h>

#include "http.h"
#include "wifi.h"

#include "config.h"


static const char* MY_TAG = "knusperhaeuschen/wifi";

#define CONFIG_LED_PIN		GPIO_NUM_2


EventGroupHandle_t wifi_event_group;
const int WIFI_CONNECTED = BIT0;
static bool reconnect = true;


static esp_err_t event_handler(void *ctx, system_event_t *event)
{
	httpd_handle_t* http = (httpd_handle_t*)ctx;

    switch (event->event_id) {
    case SYSTEM_EVENT_STA_START:
		ESP_LOGI(MY_TAG, "SYSTEM_EVENT_STA_START");
		gpio_set_level(CONFIG_LED_PIN, 0);
        esp_wifi_connect();
        break;

    case SYSTEM_EVENT_STA_GOT_IP:
		ESP_LOGI(MY_TAG, "SYSTEM_EVENT_STA_GOT_IP");
		gpio_set_level(CONFIG_LED_PIN, 1);
        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED);
		if (*http == NULL)
		{
			*http = http_start();
		}

		// set DNS
		{
			ip_addr_t d;
			d.type = IPADDR_TYPE_V4;
			d.u_addr.ip4.addr = 0x0101a8c0; // 1 1 168 192 dns
			dns_setserver(0, &d);

//			ip_addr_t dns_addr;
//			ip_addr_set_ip4_u32(&dns_addr, htonl(dhcp_get_option_value(dhcp, DHCP_OPTION_IDX_DNS_SERVER + n)));
//			dns_setserver(n, &dns_addr);
		}

        break;

    case SYSTEM_EVENT_STA_DISCONNECTED:
		ESP_LOGI(MY_TAG, "SYSTEM_EVENT_STA_DISCONNECTED");
		gpio_set_level(CONFIG_LED_PIN, 0);

		if (reconnect)
			esp_wifi_connect();
        xEventGroupClearBits(wifi_event_group, WIFI_CONNECTED);

		if (*http)
		{
			http_stop(http);
			http = NULL;
		}
        break;

    default:
		ESP_LOGI(MY_TAG, "WTF: %d", event->event_id);
        break;
    }

    return ESP_OK;
}


void wifi_init(void* arg)
{
	// -- status led
	gpio_pad_select_gpio(CONFIG_LED_PIN);
	gpio_set_direction(CONFIG_LED_PIN, GPIO_MODE_OUTPUT);

	// -- wifi
    tcpip_adapter_init();

	tcpip_adapter_dhcpc_stop(TCPIP_ADAPTER_IF_STA); // no DHCP

	tcpip_adapter_ip_info_t ipInfo;
	inet_pton(AF_INET, CONFIG_ADDRESS, &ipInfo.ip);
	inet_pton(AF_INET, CONFIG_GATEWAY, &ipInfo.gw);
	inet_pton(AF_INET, CONFIG_NETMASK, &ipInfo.netmask);
	tcpip_adapter_set_ip_info(TCPIP_ADAPTER_IF_STA, &ipInfo);

    wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_event_loop_init(event_handler, arg));
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
}


void wifi_stop()
{
	reconnect = false;
	esp_wifi_stop();
}


void wifi_start()
{
	reconnect = true;
	esp_wifi_start();
	esp_wifi_connect();
}
