/**
 * This code is public domain. Have fun.
 */

#include <esp_log.h>

#include <lwip/apps/sntp.h>

static const char* MY_TAG = "knusperhaeuschen/sntp";


void project_sntp_init()
{
	sntp_setoperatingmode(SNTP_OPMODE_POLL);
	sntp_setservername(0, "pool.ntp.org");
	sntp_init();
}


void project_sntp_update()
{
	time_t now = 0;
	struct tm timeinfo = { 0 };

	int retry = 0;
	const int retry_count = 10;

	while (timeinfo.tm_year < (2016-1900) && ++retry < retry_count)
	{
		ESP_LOGI(MY_TAG, "Waiting... (%d/%d)", retry, retry_count);
		vTaskDelay(2000 / portTICK_PERIOD_MS);
		time(&now);
		localtime_r(&now, &timeinfo);
	}

	timeinfo.tm_year += 1900;
	ESP_LOGI(MY_TAG, "%04d-%02d-%02d %02d:%02d:%02d",
			timeinfo.tm_year, timeinfo.tm_mon, timeinfo.tm_mday,
			timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);

	char buf[64];
	strftime(buf, sizeof(buf), "%c", &timeinfo);
	ESP_LOGI(MY_TAG, "%s", buf);
}
