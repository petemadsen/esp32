/**
 * This code is public domain. Have fun.
 */
#include <esp_wifi.h>
#include <esp_log.h>

#include <lwip/apps/sntp.h>

#include <rom/uart.h>

#include "wifi.h"


static const char* MY_TAG = "knusperhaeuschen/sleep";


static struct tm timeinfo = { 0 };


static void update_time();
static void night_mode();


void my_sleep_task(void* arg)
{
//	xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED, false, true,
//			portMAX_DELAY);

	// -- init
	sntp_setoperatingmode(SNTP_OPMODE_POLL);
	sntp_setservername(0, "pool.ntp.org");
	sntp_init();

	for (;;)
	{
//		xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED, false, true,
//				portMAX_DELAY);
		ESP_LOGI(MY_TAG, "Run.");

		update_time();

		if (timeinfo.tm_min == 24)
			night_mode();

		// -- wait 60 secs
		vTaskDelay(60 * 1000 / portTICK_PERIOD_MS);
	}
}


void update_time()
{
	ESP_LOGI(MY_TAG, "Updating time");

	// -- update time
	time_t now = 0;

	timeinfo.tm_year = 0;
	for (int i=0; i<1; ++i)
	{
		time(&now);
		localtime_r(&now, &timeinfo);
		printf("-->%d\n", timeinfo.tm_year);
		if (timeinfo.tm_year != 0)
		{
			timeinfo.tm_year += 1900;
			break;
		}

		ESP_LOGI(MY_TAG, "Waiting...");
		vTaskDelay(2000 / portTICK_PERIOD_MS);
	}
	if (timeinfo.tm_year == 0)
		return;

	ESP_LOGI(MY_TAG, "%04d-%02d-%02d %02d:%02d:%02d",
			timeinfo.tm_year, timeinfo.tm_mon, timeinfo.tm_mday,
			timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);

	char buf[64];
	strftime(buf, sizeof(buf), "%c", &timeinfo);
	ESP_LOGI(MY_TAG, "%s", buf);
}


void night_mode()
{
	ESP_LOGI(MY_TAG, "Entering NIGHT MODE");
	wifi_stop();

	const int max_num_runs = 3;
	for (int run=0; run<max_num_runs; ++run)
	{
		ESP_LOGI(MY_TAG, "--night %d/%d", (run+1), max_num_runs);
		uart_tx_wait_idle(CONFIG_CONSOLE_UART_NUM); // wait for line output

		esp_sleep_enable_timer_wakeup(60 * 1000 * 1000);
		esp_light_sleep_start();
#if 0
		update_time();

		if (timeinfo.tm_min == 42)
			night_mode();
#endif
	}

	wifi_start();
	ESP_LOGI(MY_TAG, "Leaving NIGHT MODE");
}

