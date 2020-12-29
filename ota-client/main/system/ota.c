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
#include "system/my_settings.h"


#define CFG_OTA_URL		"url"


static const char* ERR_NOOTA = "NOOTA";
static const char* ERR_NOFACTORY = "NOFACTORY";
static const char* ERR_NOBOOT = "NOBOOT";

//static const char* OTA_URL = PROJECT_SHUTTERS_ADDRESS "/ota/" PROJECT_NAME "?" PROJECT_VERSION;
//static const char* OTA_FILE = PROJECT_SHUTTERS_ADDRESS "/ota/file/" PROJECT_NAME;

#define RCV_BUFLEN 32
static char m_rcv_buffer[RCV_BUFLEN];


static char m_sha256[65];
static char m_project_name[33];


static const char* MY_TAG = PROJECT_TAG("ota");


esp_err_t ota_init()
{
	const esp_app_desc_t* app = esp_ota_get_app_description();

	char* p = m_sha256;
	for (int i=0; i<32; ++i, p += 2)
		sprintf(p, "%02x", app->app_elf_sha256[i]);
	m_sha256[64] = 0;

	strncpy(m_project_name, app->project_name, 32);
	m_project_name[32] = 0;

	ESP_LOGE(MY_TAG, "VERSION..: %s", app->version);
	ESP_LOGE(MY_TAG, "PROJECT..: %s", m_project_name);
	ESP_LOGE(MY_TAG, "TIME.....: %s", app->time);
	ESP_LOGE(MY_TAG, "DATE.....: %s", app->date);
	ESP_LOGE(MY_TAG, "SHA256...: %s", m_sha256);

	char* ota_file = malloc(1025);
	snprintf(ota_file, 1024,
			 PROJECT_SHUTTERS_ADDRESS "/ota/file/%s", ota_project_name());
	esp_err_t err = ota_set_download_url(ota_file);
	free(ota_file);
	return err;
}


esp_err_t ota_set_download_url(const char* url)
{
	return settings_set_str(STORAGE_OTA, CFG_OTA_URL, url, false);
}


esp_err_t ota_download_url(const char** buffer)
{
	return settings_get_str(STORAGE_OTA, CFG_OTA_URL, buffer, false);
}


void ota_reboot_task(void* arg)
{
	ESP_LOGI(MY_TAG, "Going OTA in 3 seconds.");
	vTaskDelay(3000 / portTICK_PERIOD_MS);
	esp_restart();
}


bool ota_has_update_partition()
{
	return esp_ota_get_next_update_partition(NULL);
}


const char* ota_reboot()
{
	if (!ota_has_update_partition())
		return ERR_NOOTA;

	const esp_partition_t* factory = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_ANY, "factory");
	if (!factory)
		return ERR_NOFACTORY;

	if (esp_ota_set_boot_partition(factory) != ESP_OK)
		return ERR_NOBOOT;

	// get ready to reboot
	xTaskCreate(ota_reboot_task, "ota_reboot_task", 2048, NULL, 5, NULL);
	return NULL;
}


static esp_err_t _http_event_handle(esp_http_client_event_t *evt)
{
	switch(evt->event_id)
	{
	case HTTP_EVENT_ERROR:
		ESP_LOGI(MY_TAG, "HTTP_EVENT_ERROR");
		break;
	case HTTP_EVENT_ON_CONNECTED:
		ESP_LOGI(MY_TAG, "HTTP_EVENT_ON_CONNECTED");
		break;
	case HTTP_EVENT_HEADER_SENT:
		ESP_LOGI(MY_TAG, "HTTP_EVENT_HEADER_SENT");
		break;
	case HTTP_EVENT_ON_HEADER:
		ESP_LOGI(MY_TAG, "HTTP_EVENT_ON_HEADER (%.*s)", evt->data_len, (char*)evt->data);
		break;
	case HTTP_EVENT_ON_DATA:
		ESP_LOGI(MY_TAG, "HTTP_EVENT_ON_DATA, len=%d is_chunked=%d",
				 evt->data_len,
				 esp_http_client_is_chunked_response(evt->client));
		if (evt->data_len < RCV_BUFLEN)
		{
			strncpy(m_rcv_buffer, (char*)evt->data, evt->data_len);
			ESP_LOGI(MY_TAG, "HTTP_EVENT_ON_DATA (%s)", m_rcv_buffer);
		}
		break;
	case HTTP_EVENT_ON_FINISH:
		ESP_LOGI(MY_TAG, "HTTP_EVENT_ON_FINISH");
		break;
	case HTTP_EVENT_DISCONNECTED:
		ESP_LOGI(MY_TAG, "HTTP_EVENT_DISCONNECTED");
		break;
	}

	return ESP_OK;
}


bool ota_need_update()
{
	size_t ota_url_len = 160;
	char* ota_url = malloc(ota_url_len + 1);
	snprintf(ota_url, ota_url_len,
			 PROJECT_SHUTTERS_ADDRESS "/ota/%s?%s",
			 ota_project_name(), m_sha256);

	bool need_update = false;
	m_rcv_buffer[0] = 0;

	ESP_LOGI(MY_TAG, "Checking for update: %s", ota_url);

	esp_http_client_config_t request = {
		.url = ota_url,
		.event_handler = _http_event_handle,
		.timeout_ms = 2000,
	};
	esp_http_client_handle_t client = esp_http_client_init(&request);
	esp_err_t err = esp_http_client_perform(client);
	if (err == ESP_OK)
	{
		int status = esp_http_client_get_status_code(client);
		int size = esp_http_client_get_content_length(client);
		ESP_LOGI(MY_TAG, "received[%d,%d]: %s", status, size, m_rcv_buffer);

		need_update = (status == 200);
	}
	else
		ESP_LOGI(MY_TAG, "Could not download info.");

	free(ota_url);
	esp_http_client_cleanup(client);
	return need_update;
}


const char* ota_sha256()
{
	return m_sha256;
}


const char* ota_project_name()
{
	return m_project_name;
}
