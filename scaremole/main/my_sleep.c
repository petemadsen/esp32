/**
 * This code is public domain.
 */
#include <esp_wifi.h>
#include <esp_log.h>

#include <lwip/apps/sntp.h>

#include <rom/uart.h>

#include "wifi.h"
#include "common.h"
#include "my_sleep.h"


static const char* MY_TAG = "scaremole/sleep";


#define MIN_SLEEP_SECS 10LL
#define MIN_SLEEP_SECS 600LL


void my_sleep_task(void* arg)
{
	uint32_t mins = 0;
	uint32_t nowifi_mins = 0;

	for (;;)
	{
		ESP_LOGI(MY_TAG, "Run.%u", mins);

		// go to sleep after maximum minutes
		if (mins > 0)
		{
			my_sleep_now("time out");
		}

#if 0
		// reboot/sleep if no wifi
		EventBits_t bits = xEventGroupGetBits(wifi_event_group);
		if ((bits & WIFI_CONNECTED) == 0)
		{
			ESP_LOGW(MY_TAG, "NoWiFi %d", nowifi_mins);
			if (++nowifi_mins > 2)
			{
				ESP_LOGW(MY_TAG, "Entering SECURITY MODE");
				wifi_stop();
				uart_tx_wait_idle(CONFIG_CONSOLE_UART_NUM); // wait for line output
//				esp_deep_sleep(600LL * 1000000LL);
				esp_restart();
			}
			esp_wifi_connect();
		}
		else
		{
			nowifi_mins = 0;
		}
#endif

		// -- wait 60 secs
		vTaskDelay(60 * 1000 / portTICK_PERIOD_MS);
		++mins;
	}
}


void my_sleep_now(const char* reason)
{
	uint64_t secs = MIN_SLEEP_SECS + esp_random() % 60;
	ESP_LOGW(MY_TAG, "going to sleep because '%s' for %llu secs",
			 reason, secs);
	vTaskDelay(100);

	esp_deep_sleep(secs * 1000000LL);
}
