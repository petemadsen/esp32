/**
 * This code is public domain.
 */
#include "my_settings.h"

#include <esp_log.h>

#include <nvs.h>


#define STORAGE	"app"


esp_err_t settings_set(const char* key, int val);
esp_err_t settings_get(const char* key, int* val);


static const char* MY_TAG = "knusperhaeuschen/settings";


esp_err_t settings_set(const char* key, int val)
{
	esp_err_t err;

	nvs_handle my_handle;
	err = nvs_open(STORAGE, NVS_READWRITE, &my_handle);
	if (err != ESP_OK)
	{
		ESP_LOGE(MY_TAG, "Could not open storage: %s", STORAGE);
		return err;
	}

	err = nvs_set_i32(my_handle, key, val);
	if (err != ESP_OK)
	{
		nvs_close(my_handle);
		ESP_LOGE(MY_TAG, "Could not set filename: %s", key);
		return err;
	}

	err = nvs_commit(my_handle);
	nvs_close(my_handle);
	return err;
}


esp_err_t settings_get(const char* key, int32_t* val)
{
	esp_err_t err;

	nvs_handle my_handle;
	err = nvs_open(STORAGE, NVS_READWRITE, &my_handle);
	if (err != ESP_OK)
	{
		ESP_LOGE(MY_TAG, "Could not open storage: %s", STORAGE);
		return err;
	}

	err = nvs_get_i32(my_handle, key, val);
	if (err != ESP_OK)
	{
		nvs_close(my_handle);
		ESP_LOGE(MY_TAG, "Could not set filename: %s", key);
		return err;
	}

	err = nvs_commit(my_handle);
	nvs_close(my_handle);
	return err;
}
