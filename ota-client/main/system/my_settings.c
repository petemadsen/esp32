/**
 * This code is public domain.
 */
#include "system/my_settings.h"
#include "common.h"
#include "system/ota.h"

#include <esp_log.h>

#include <nvs.h>
#include <nvs_flash.h>

#include <string.h>


#define APP_STORAGE	"app"


static const char* MY_TAG = PROJECT_TAG("settings");


#define SETTING_BOOT_COUNTER "boot_counter"


static int32_t m_boot_counter = 1;


static void print_all_keys(const char* storage);


esp_err_t settings_init()
{
	ESP_LOGI(MY_TAG, "%s / %s / %d", ota_project_name(), ota_sha256(), PROJECT_BOARD);

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
		ESP_ERROR_CHECK(nvs_flash_init());
	}

	// boot counter
	err = settings_get_int32(STORAGE_APP, SETTING_BOOT_COUNTER, &m_boot_counter, true);
	if (err == ESP_OK)
	{
		m_boot_counter += 1;
		settings_set_int32(STORAGE_APP, SETTING_BOOT_COUNTER, m_boot_counter, false);
	}
	else
	{
		m_boot_counter = -1;
	}
	ESP_LOGI(MY_TAG, "Boot counter: %d", m_boot_counter);

	// Example of nvs_get_stats() to get the number of used entries and free entries:
	nvs_stats_t nvs_stats;
	err = nvs_get_stats(NVS_DEFAULT_PART_NAME, &nvs_stats);
	ESP_LOGI(MY_TAG, "UsedEntries %zu FreeEntries %zu AllEntries %zu",
			 nvs_stats.used_entries, nvs_stats.free_entries, nvs_stats.total_entries);

	print_all_keys(STORAGE_APP);

	ESP_LOGI(MY_TAG, "Error: %s/%d", esp_err_to_name(err), err);
	return ESP_OK;
	return err;
}


esp_err_t settings_set_int32(const char* storage, const char* key, int32_t val, bool must_exist)
{
	esp_err_t err = ESP_OK;

	nvs_handle my_handle;
	err = nvs_open(storage, NVS_READWRITE, &my_handle);
	if (err != ESP_OK)
	{
		ESP_LOGE(MY_TAG, "Could not open storage: %s", storage);
		return err;
	}

	if (must_exist)
	{
		int32_t ignored;
		err = nvs_get_i32(my_handle, key, &ignored);
	}

	if (err == ESP_OK)
	{
		err = nvs_set_i32(my_handle, key, val);
		if (err != ESP_OK)
		{
			nvs_close(my_handle);
			ESP_LOGE(MY_TAG, "Could not set: %s", key);
			return err;
		}
		err = nvs_commit(my_handle);
	}

	nvs_close(my_handle);
	return err;
}


esp_err_t settings_get_int32(const char* storage, const char* key, int32_t* val, bool save_if_missing)
{
	esp_err_t err;

	nvs_handle my_handle;
	err = nvs_open(storage, NVS_READWRITE, &my_handle);
	if (err != ESP_OK)
	{
		ESP_LOGE(MY_TAG, "Could not open storage: %s", storage);
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
			if (err == ESP_OK)
				err = nvs_commit(my_handle);
		}
	}

	nvs_close(my_handle);
	return err;
}


esp_err_t settings_get_str(const char* storage, const char* key, char** buffer, bool save_if_missing)
{
	esp_err_t err = ESP_OK;
	char* newval = NULL;

	nvs_handle my_handle;
	err = nvs_open(storage, NVS_READWRITE, &my_handle);
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
		ESP_LOGW(MY_TAG, "Could not read '%s' (%d).", key, save_if_missing);
		if (save_if_missing)
		{
			err = settings_set_str(storage, key, *buffer, false);
		}
		return err;
	}

	if (*buffer == NULL)
	{
		*buffer = newval;
	}
	else if (strcmp(newval, *buffer) != 0)
	{
		free(*buffer);
		*buffer = newval;
	}

	return err;
}


esp_err_t settings_set_str(const char* storage, const char* key, const char* val, bool must_exist)
{
	esp_err_t err;

	nvs_handle my_handle;
	err = nvs_open(storage, NVS_READWRITE, &my_handle);
	if (err != ESP_OK)
	{
		ESP_LOGE(MY_TAG, "Could not open storage: %s", storage);
		return err;
	}

	if (must_exist)
	{
		size_t len = 0;
		err = nvs_get_str(my_handle, key, NULL, &len);
	}

	if (err == ESP_OK)
	{
		err = nvs_set_str(my_handle, key, val);
		if (err != ESP_OK)
		{
			nvs_close(my_handle);
			ESP_LOGE(MY_TAG, "Could not set: %s", key);
			return err;
		}
		err = nvs_commit(my_handle);
	}

	nvs_close(my_handle);
	return err;
}


esp_err_t settings_clear(const char* storage)
{
	esp_err_t err;

	nvs_handle handle;
	err = nvs_open(storage, NVS_READWRITE, &handle);
	if (err != ESP_OK)
	{
		ESP_LOGE(MY_TAG, "Could not open storage: %s", storage);
		return err;
	}

	if ((err = nvs_erase_all(handle)) == ESP_OK)
	{
		err = nvs_commit(handle);
	}

	nvs_close(handle);
	return err;
}


static void print_all_keys(const char* namespace)
{
	ESP_LOGE(MY_TAG, "PRINT_ALL_KEYS: %s", namespace);

	nvs_iterator_t it = nvs_entry_find(NVS_DEFAULT_PART_NAME, namespace, NVS_TYPE_ANY);
	while (it != NULL)
	{
		nvs_entry_info_t info;
		nvs_entry_info(it, &info);
		it = nvs_entry_next(it);
		switch (info.type)
		{
		case NVS_TYPE_STR:
			{
				char* buffer = NULL;
				settings_get_str(namespace, info.key, &buffer, false);
				ESP_LOGE(MY_TAG, "%s [%s]", info.key, buffer);
				if (buffer)
					free(buffer);
			}
			break;
		case NVS_TYPE_I32:
			{
				int32_t value;
				settings_get_int32(namespace, info.key, &value, false);
				ESP_LOGE(MY_TAG, "%s [%d]", info.key, value);
			}
			break;
		default:
			ESP_LOGE(MY_TAG, "%s, 0x%x", info.key, info.type);
			break;
		}
	}

	ESP_LOGE(MY_TAG, "DONE: %s", namespace);
}


size_t settings_get_all(const char* namespace, const char** buffer, size_t buffer_len)
{
	char* p = *buffer;

	nvs_iterator_t it = nvs_entry_find(NVS_DEFAULT_PART_NAME, namespace, NVS_TYPE_ANY);
	while (it != NULL)
	{
		nvs_entry_info_t info;
		nvs_entry_info(it, &info);
		it = nvs_entry_next(it);

		if (buffer_len < strlen(info.key) + 5)
		{
			ESP_LOGE(MY_TAG, "Too many entries.");
			// we will fill the trailing chars with "." to indicate there is more.
			while (--buffer_len)
			{
				*p = '.';
				p += 1;
			}
			break;
		}

		strcpy(p, info.key);
		p += strlen(info.key);
		*p = ' ';
		p += 1;
		buffer_len -= strlen(info.key) + 1;
	}

	buffer_len = p - *buffer;
	if (buffer_len)
		--buffer_len;

	return buffer_len;
}


esp_err_t settings_erase()
{
	return nvs_flash_erase();
}


int32_t settings_boot_counter()
{
	return m_boot_counter;
}
