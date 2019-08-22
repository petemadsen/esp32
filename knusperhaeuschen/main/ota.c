/**
 * This code is public domain.
 */
#include "ota.h"

#include <esp_system.h>
#include <esp_log.h>
#include <esp_partition.h>
#include <esp_ota_ops.h>

#include <nvs.h>


#define CFG_OTA_STORAGE	"ota"
#define CFG_OTA_FILENAME "filename"
#define CFG_OTA_FILENAME_LENGTH 30


static const char* ERR_NOFACTORY = "NOFACTORY";
static const char* ERR_NOBOOT = "NOBOOT";

#define PROJECT_NAME "knusperhaeuschen.bin"

static const char* MY_TAG = "knusperhaeuschen/ota";


esp_err_t ota_init()
{
	esp_err_t err;

	nvs_handle my_handle;
    err = nvs_open(CFG_OTA_STORAGE, NVS_READWRITE, &my_handle);
	if (err != ESP_OK)
	{
		ESP_LOGE(MY_TAG, "Could not open storage: %s", CFG_OTA_STORAGE);
		return err;
	}

	err = nvs_set_str(my_handle, CFG_OTA_FILENAME, PROJECT_NAME);
	if (err != ESP_OK)
	{
		ESP_LOGE(MY_TAG, "Could not set filename: %s", CFG_OTA_FILENAME);
		return err;
	}

	return nvs_commit(my_handle);
}


const char* ota_reboot()
{
	const esp_partition_t* factory = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_ANY, "factory");
	if (!factory)
		return ERR_NOFACTORY;

	if (esp_ota_set_boot_partition(factory) != ESP_OK)
		return ERR_NOBOOT;

//	esp_restart();
	return NULL;
}
