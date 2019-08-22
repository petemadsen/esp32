/**
 * This code is public domain.
 */
#include "my_settings.h"

#include <esp_log.h>

#include <nvs.h>
#include <nvs_flash.h>


#define STORAGE	"app"


esp_err_t settings_set(const char* key, int val);
esp_err_t settings_get(const char* key, int* val);


static const char* MY_TAG = "knusperhaeuschen/settings";


esp_err_t settings_init()
{
	// -- initialize nvs.
	esp_err_t err = nvs_flash_init();
	if (err == ESP_ERR_NVS_NO_FREE_PAGES)
	{
		ESP_LOGE(MY_TAG, "NVC no free pages.");
		// OTA app partition ();table has a smaller NVS partition size than the
		// non-OTA partition table. This size mismatch may cause NVS
		// initialization to fail. If this happens, we erase NVS partition
		// and initialize NVS again.
		ESP_ERROR_CHECK(nvs_flash_erase());
		err = nvs_flash_init();
	}

	// Example of nvs_get_stats() to get the number of used entries and free entries:
	nvs_stats_t nvs_stats;
	nvs_get_stats(STORAGE, &nvs_stats);
	ESP_LOGI(MY_TAG, "UsedEntries %zu FreeEntries %zu AllEntries %zu",
		   nvs_stats.used_entries, nvs_stats.free_entries, nvs_stats.total_entries);

	return err;
}


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
		ESP_LOGE(MY_TAG, "Could not set: %s", key);
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
		ESP_LOGE(MY_TAG, "Could not get: %s", key);
		return err;
	}

	err = nvs_commit(my_handle);
	nvs_close(my_handle);
	return err;
}
