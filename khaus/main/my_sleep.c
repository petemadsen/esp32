/**
 * This code is public domain.
 */
#include <esp_wifi.h>
#include <esp_log.h>
#include <esp_sleep.h>

#include <lwip/apps/sntp.h>

#include <rom/uart.h>

#include "common.h"

#include "my_lights.h"
#include "system/wifi.h"
#include "system/my_settings.h"
#include "system/my_log.h"


static const char* MY_TAG = PROJECT_TAG("sleep");


static void update_time(void);


#define SETTING_HOUR_FROM	"sleep.from"
#define SETTING_HOUR_TO		"sleep.to"
#define SETTING_LIGHTS_OFF	"sleep.lights"


RTC_DATA_ATTR int m_deep_sleep_boots = 0;


void my_sleep_task(void* arg)
{
#if 0
	// -- init time
	sntp_setoperatingmode(SNTP_OPMODE_POLL);
	sntp_setservername(0, "pool.ntp.org");
	setenv("TZ", "CET-1CEST-2,M3.5.0/02:00:00,M10.5.0/03:00:00", 1);
	sntp_init();
#else
	setenv("TZ", "CET-1CEST-2,M3.5.0/02:00:00,M10.5.0/03:00:00", 1);
#endif

	int32_t hour_from = 21;
	int32_t hour_to = 7;
	int32_t lights_off_mins = 10;

	uint32_t mins = 0;
	uint32_t nowifi_mins = 0;

	bool ok_to_go_to_sleep = true;

	// count deep sleep wakeups
	esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
	if (wakeup_reason == ESP_SLEEP_WAKEUP_TIMER)
	{
		++m_deep_sleep_boots;
		int32_t hours_to_sleep = hour_to + 24 - hour_from;
		if (hour_to > hour_from)
			hours_to_sleep = hour_to - hour_from;

		// Oops, we slept too long. What happened? What to do?
		if (m_deep_sleep_boots > hours_to_sleep)
		{
			ok_to_go_to_sleep = false;
			mylog_add("deep slept too long.");
			ESP_LOGE(MY_TAG, "Deep slept too long.");
		}
	}

	for (;;)
	{
		ESP_LOGI(MY_TAG, "Run [%d]", mins);

		// read settings
		esp_err_t err = settings_get_int32(SETTING_HOUR_FROM, &hour_from, true);
		ESP_LOGW(MY_TAG, "RET=%s\n", esp_err_to_name(err));
		err = settings_get_int32(SETTING_HOUR_TO, &hour_to, true);
		ESP_LOGW(MY_TAG, "RET=%s\n", esp_err_to_name(err));
		err = settings_get_int32(SETTING_LIGHTS_OFF, &lights_off_mins, true);
		ESP_LOGW(MY_TAG, "RET=%s\n", esp_err_to_name(err));

		// update time
		time_t now;
		struct tm timeinfo;
		time(&now);
		localtime_r(&now, &timeinfo);
		timeinfo.tm_year += 1900;
		ESP_LOGW(MY_TAG, "TIME: %02d:%02d / %04d",
				 timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_year);

		// enter night mode?
		if (ok_to_go_to_sleep && timeinfo.tm_year > 1980)
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
#if 1
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
#endif

		// turn off the lights
		if (lights_off_mins != -1 && lamp_on_secs() > lights_off_mins * 60)
		{
			lamp_off();
		}

		// FIXME
		if (!ok_to_go_to_sleep && mins > 60 * 12)
		{
			mylog_add("Long time no sleep. Reboot!");
			esp_restart();
		}

		// -- wait 60 secs
		vTaskDelay(60 * 1000 / portTICK_PERIOD_MS);
		++mins;
	}
}
