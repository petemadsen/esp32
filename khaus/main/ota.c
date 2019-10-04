/**
 * This code is public domain.
 */
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <string.h>

#include <esp_system.h>
#include <esp_log.h>
#include <esp_partition.h>
#include <esp_ota_ops.h>

#include <nvs.h>

#include <esp_http_client.h>

#include "ota.h"
#include "common.h"


#define CFG_OTA_STORAGE			"ota"
#define CFG_OTA_FILENAME		"filename"
#define CFG_OTA_FILENAME_LENGTH 30


static const char* ERR_NOFACTORY = "NOFACTORY";
static const char* ERR_NOBOOT = "NOBOOT";

#define BIN_NAME "khaus.bin"


static const char* OTA_URL = PROJECT_SHUTTERS_ADDRESS "/ota/" PROJECT_NAME;
#define RCV_BUFLEN 20
static char m_rcv_buffer[RCV_BUFLEN];


static const char* MY_TAG = "khaus/ota";


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

	err = nvs_set_str(my_handle, CFG_OTA_FILENAME, BIN_NAME);
	if (err != ESP_OK)
	{
		nvs_close(my_handle);
		ESP_LOGE(MY_TAG, "Could not set filename: %s", CFG_OTA_FILENAME);
		return err;
	}

	err = nvs_commit(my_handle);
	nvs_close(my_handle);
	return err;
}


static void reboot_task(void* arg)
{
	ESP_LOGI(MY_TAG, "Going OTA in 3 seconds.");
	vTaskDelay(3000 / portTICK_PERIOD_MS);
	esp_restart();
}


const char* ota_reboot()
{
	const esp_partition_t* factory = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_ANY, "factory");
	if (!factory)
		return ERR_NOFACTORY;

	if (esp_ota_set_boot_partition(factory) != ESP_OK)
		return ERR_NOBOOT;

	// get ready to reboot
	xTaskCreate(reboot_task, "reboot_task", 2048, NULL, 5, NULL);
	return NULL;
}


static esp_err_t _http_event_handle(esp_http_client_event_t *evt)
{
	switch(evt->event_id) {
		case HTTP_EVENT_ERROR:
			ESP_LOGD(MY_TAG, "HTTP_EVENT_ERROR");
			break;
		case HTTP_EVENT_ON_CONNECTED:
			ESP_LOGD(MY_TAG, "HTTP_EVENT_ON_CONNECTED");
			break;
		case HTTP_EVENT_HEADER_SENT:
			ESP_LOGD(MY_TAG, "HTTP_EVENT_HEADER_SENT");
			break;
		case HTTP_EVENT_ON_HEADER:
			ESP_LOGD(MY_TAG, "HTTP_EVENT_ON_HEADER");
			printf("%.*s", evt->data_len, (char*)evt->data);
			break;
		case HTTP_EVENT_ON_DATA:
			ESP_LOGI(MY_TAG, "HTTP_EVENT_ON_DATA, len=%d is_chunked=%d",
					 evt->data_len,
					 esp_http_client_is_chunked_response(evt->client));
			if (evt->data_len < RCV_BUFLEN)
			{
				strncpy(m_rcv_buffer, (char*)evt->data, evt->data_len);
				ESP_LOGI(MY_TAG, "ooook(%s)", m_rcv_buffer);
			}
			break;
		case HTTP_EVENT_ON_FINISH:
			ESP_LOGD(MY_TAG, "HTTP_EVENT_ON_FINISH");
			break;
		case HTTP_EVENT_DISCONNECTED:
			ESP_LOGD(MY_TAG, "HTTP_EVENT_DISCONNECTED");
			break;
	}
	return ESP_OK;
}


bool ota_need_update()
{
	bool need_update = false;
	m_rcv_buffer[0] = 0;

	esp_http_client_config_t request = {
		.url = OTA_URL,
		.event_handler = _http_event_handle,
	};
	esp_http_client_handle_t client = esp_http_client_init(&request);
	esp_err_t err = esp_http_client_perform(client);
	if (err == ESP_OK)
	{
		int status = esp_http_client_get_status_code(client);
		int size = esp_http_client_get_content_length(client);
		ESP_LOGI(MY_TAG, "received[%d,%d]: %s", status, size, m_rcv_buffer);

		need_update = status == 200 && strlen(m_rcv_buffer) != 0 && strcmp(m_rcv_buffer, PROJECT_VERSION);
		if (status == 200 && strlen(m_rcv_buffer) != 0)
		{
			need_update = strcmp(m_rcv_buffer, PROJECT_VERSION);
			ESP_LOGI(MY_TAG, "Version compare '%s' == '%s' ==> %d",
					 m_rcv_buffer, PROJECT_VERSION, need_update);
		}
	}
	else
		ESP_LOGI(MY_TAG, "Could not download info.");

	esp_http_client_cleanup(client);
	return need_update;
}
