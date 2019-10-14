/**
 * http_request example.
 */
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>
#include <esp_log.h>

#include <lwip/err.h>

#include <esp_http_client.h>

#include "common.h"

#include "wifi.h"
#include "my_sleep.h"
#include "ota.h"
#include "voltage.h"


static const char* MY_TAG = "scaremole/shutters";


static const char* SAVE_URL = PROJECT_SHUTTERS_ADDRESS "/scaremole/save";


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
            ESP_LOGD(MY_TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
            if (!esp_http_client_is_chunked_response(evt->client))
			{
                printf("%.*s", evt->data_len, (char*)evt->data);
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


void shutters_task(void* pvParameters)
{
	esp_err_t err;
	for (;;)
	{
		xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED, false, true,
				portMAX_DELAY);
		ESP_LOGI(MY_TAG, "Run.");

		// -- save
		ESP_LOGI(MY_TAG, "Sending data: %s", SAVE_URL);
		esp_http_client_config_t config = {
			.url = SAVE_URL,
			.event_handler = _http_event_handle,
		};
		esp_http_client_handle_t client = esp_http_client_init(&config);

		const size_t POST_MAXLEN = 200;
		char* save_data = malloc(POST_MAXLEN);
		int save_data_len = snprintf(save_data, POST_MAXLEN,
									 "version=%s"
									 " board_voltage=%.2f"
									 " board_temp=%.2f",
									 PROJECT_VERSION,
									 voltage_get(),
									 -1.0);

		esp_http_client_set_url(client, SAVE_URL);
		esp_http_client_set_method(client, HTTP_METHOD_POST);
		esp_http_client_set_post_field(client, save_data, save_data_len);
		err = esp_http_client_perform(client);
		if (err == ESP_OK)
		{
			ESP_LOGI(MY_TAG, "Save/Status = %d, content_length = %d",
					esp_http_client_get_status_code(client),
					esp_http_client_get_content_length(client));
		}
		free(save_data);
		esp_http_client_cleanup(client);

		// -- check for ota update
		ESP_LOGI(MY_TAG, "Checking for OTA updates.");
		if (ota_need_update())
		{
			ESP_LOGW(MY_TAG, "OTA updated required. Trying to reboot.");
			const char* err_msg = ota_reboot();
			if (!err_msg)
			{
				for (;;)
					vTaskDelay(1000 * 1000 / portTICK_PERIOD_MS);
			}
			ESP_LOGE(MY_TAG, "OTA reboot failed: %s", err_msg);
		}
		else
			ESP_LOGI(MY_TAG, "No OTA update needed.");


		my_sleep_now("after shutters connection");

		// -- wait 3600 secs = 1h
		vTaskDelay(3600 * 1000 / portTICK_PERIOD_MS);
	}
}

