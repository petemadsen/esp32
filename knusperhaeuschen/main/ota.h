#ifndef PROJECT_OTA_H
#define PROJECT_OTA_H


#define CFG_OTA_STORAGE	"ota"
#define CFG_OTA_FILENAME "filename"
#define CFG_OTA_FILENAME_LENGTH 30

#define PROJECT_NAME "knusperhaeuschen.bin"

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

	return ESP_OK;
}

#endif
