/**
 * This code is public domain.
 */
#include "my_settings.h"

#include <esp_log.h>

#include <nvs.h>
#include <nvs_flash.h>

#include <string.h>


#define STORAGE	"app"


static const char* MY_TAG = "khaus/settings";


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


esp_err_t settings_set_int32(const char* key, int32_t val)
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


esp_err_t settings_get_int32(const char* key, int32_t* val, bool save_if_missing)
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
		ESP_LOGE(MY_TAG, "Could not get: %s", key);
		if (save_if_missing)
		{
			ESP_LOGI(MY_TAG, "Saving value: %s => %d", key, *val);
			err = nvs_set_i32(my_handle, key, *val);
		}
	}

	nvs_close(my_handle);
	return err;
}


esp_err_t settings_get_str(const char* key, char** buffer, const char* default_val, bool save_if_missing)
{
	esp_err_t err = ESP_OK;
	char* newval = NULL;

	nvs_handle my_handle;
	err = nvs_open(STORAGE, NVS_READWRITE, &my_handle);
	if (err == ESP_OK)
	{
		size_t len = 0;
		err = nvs_get_str(my_handle, key, NULL, &len);
		if (err == ESP_OK)
		{
			newval = malloc(len);
			err = nvs_get_str(my_handle, key, newval, &len);
			if (err != ESP_OK)
			{
				free(newval);
				newval = NULL;
			}
		}

		nvs_close(my_handle);
	}

	if (!newval)
	{
		printf("------------------- could not read value.\n");
		newval = default_val;
		if (save_if_missing)
		{
			err = settings_set_str(key, newval);
			printf("ERR: %s\n", esp_err_to_name(err));
		}
	}

	printf("-------%s\n", newval);
	printf("--+----%s\n", *buffer);
	if (strcmp(newval, *buffer) != 0)
	{
		printf("----------------- changed from: %s -> %s\n", *buffer, newval);
		free(*buffer);
		*buffer = newval;
	}

	return err;
}


esp_err_t settings_set_str(const char* key, const char* val)
{
	esp_err_t err;

	nvs_handle my_handle;
	err = nvs_open(STORAGE, NVS_READWRITE, &my_handle);
	if (err != ESP_OK)
	{
		ESP_LOGE(MY_TAG, "Could not open storage: %s", STORAGE);
		return err;
	}

	err = nvs_set_str(my_handle, key, val);
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
