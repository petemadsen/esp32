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
#include "wifi.h"


static const char* MY_TAG = "xmastree/main";

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
		ESP_LOGE(MY_TAG, "mDNS failed: %d", err);
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

	ESP_LOGE(MY_TAG, "button-task.");
	for (;;)
	{
		if (xSemaphoreTake(xButtonSem, portMAX_DELAY) == true)
		{
			ESP_LOGI(MY_TAG, "button pressed");
			colors_next_mode();
		}
	}
}



void app_main(void)
{
	ESP_ERROR_CHECK(nvs_flash_init());

	colors_init();

	parser_init();

	wifi_init(true);

	xTaskCreate(&telnet_task, "telnet_task", 4096, &parse_input, 5, NULL);

	msgeq7_init();

//    xTaskCreate(button_task, "button_task", 1024, NULL, 10, NULL);
}

