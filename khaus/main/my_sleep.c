/**
 * This code is public domain.
 */
#include <esp_wifi.h>
#include <esp_log.h>

#include <lwip/apps/sntp.h>

#include <rom/uart.h>

#include "wifi.h"
#include "my_settings.h"
#include "light.h"


static const char* MY_TAG = "khaus/sleep";


static struct tm timeinfo = { 0 };


static void update_time(void);


#define SETTING_HOUR_FROM	"sleep.night_from"
#define SETTING_HOUR_TO		"sleep.night_to"
#define SETTING_LIGHTS_OFF	"sleep.lights_on_mins"


void my_sleep_task(void* arg)
{
	// -- init time
	sntp_setoperatingmode(SNTP_OPMODE_POLL);
	sntp_setservername(0, "pool.ntp.org");
	setenv("TZ", "CET-1CEST-2,M3.5.0/02:00:00,M10.5.0/03:00:00", 1);
	sntp_init();

	int hour_from = 21;
	int hour_to = 7;
	int lights_off_mins = 10;

	uint32_t mins = 0;
	uint32_t nowifi_mins = 0;

	for (;;)
	{
		ESP_LOGI(MY_TAG, "Run.");

		// read settings
		settings_get(SETTING_HOUR_FROM, &hour_from);
		settings_get(SETTING_HOUR_TO, &hour_to);
		settings_get(SETTING_LIGHTS_OFF, &lights_off_mins);

		//
		update_time();

		// enter night mode?
		if (timeinfo.tm_year > 1980)
		{
			bool is_night = timeinfo.tm_hour >= hour_from || timeinfo.tm_hour < hour_to;
//			is_night = true;

			if (is_night && mins != 0)
			{
				ESP_LOGW(MY_TAG, "Entering NIGHT MODE");
				wifi_stop();
				uart_tx_wait_idle(CONFIG_CONSOLE_UART_NUM); // wait for line output
				esp_deep_sleep(3600LL * 1000000LL);
			}
		}

		// reboot/sleep if no wifi
		EventBits_t bits = xEventGroupGetBits(wifi_event_group);
		if ((bits & WIFI_CONNECTED) == 0)
		{
			ESP_LOGW(MY_TAG, "NoWiFi %d", nowifi_mins);
			if (++nowifi_mins > 10)
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

		// turn off the lights
		if (light_on_secs() > lights_off_mins * 60)
		{
			light_off();
		}

		// -- wait 60 secs
		vTaskDelay(60 * 1000 / portTICK_PERIOD_MS);
		++mins;
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
