/**
 * This code is public domain.
 */
#include <string.h>

#include <esp_log.h>
#include <lwip/apps/sntp.h>

#include "common.h"
#include "my_log.h"
#include "system/my_settings.h"


static const char* MY_TAG = PROJECT_TAG("log");

#define SETTING_NEXT_LOG_POS	"log.next"
#define SETTING_LOG_PREFIX		"log.entry%d"


esp_err_t mylog_add(const char* msg)
{
	esp_err_t ret = ESP_OK;

	// get next log entry position
	int32_t next_pos = 0;
	ret = settings_get_int32(STORAGE_APP, SETTING_NEXT_LOG_POS, &next_pos, true);
	if ( ret != ESP_OK)
		return ret;

	char* key = malloc(10 + strlen(SETTING_LOG_PREFIX));
	sprintf(key, SETTING_LOG_PREFIX, next_pos);

	// create msg to save
	time_t now;
	struct tm ti;
	time(&now);
	localtime_r(&now, &ti);

	// buffer to contain date + msg
	char* buffer = malloc(30 + strlen(msg));
	sprintf(buffer, "%04d-%02d-%02d_%02d-%02d-%02d %s",
			ti.tm_year, ti.tm_mon, ti.tm_mday,
			ti.tm_hour, ti.tm_min, ti.tm_sec,
			msg);

	// write msg
	ret = settings_set_str(STORAGE_APP, key, buffer, false);

	// cleanup
	free(key);
	free(buffer);

	// increase next position
	if (ret == ESP_OK)
	{
		next_pos = (next_pos + 1) % LOG_MAX_ENTRIES;
		settings_set_int32(STORAGE_APP, SETTING_NEXT_LOG_POS, next_pos, false);
	}

	return ret;
}


bool mylog_get(uint8_t num, char** buf)
{
	if (num >= LOG_MAX_ENTRIES)
		return false;

	char* key = malloc(10 + strlen(SETTING_LOG_PREFIX));
	sprintf(key, SETTING_LOG_PREFIX, num);

	esp_err_t err = settings_get_str(STORAGE_APP, key, buf, false);
	if (err != ESP_OK)
		return false;

	return true;
}
