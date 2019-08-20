/**
 * http_request example.
 */
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>
#include <esp_log.h>

#include <lwip/err.h>

#include <esp_http_client.h>

#include "wifi.h"


static const char* MY_TAG = "knusperhaeuschen/shutters";


static const char* base_url = "http://192.168.1.86:8080/knusperhaeuschen/touch";


esp_err_t _http_event_handle(esp_http_client_event_t *evt)
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
	for (;;)
	{
		xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED, false, true,
				portMAX_DELAY);
		ESP_LOGI(MY_TAG, "Run.");

		esp_http_client_config_t config = {
			.url = base_url,
			.event_handler = _http_event_handle,
		};
		esp_http_client_handle_t client = esp_http_client_init(&config);
		esp_err_t err = esp_http_client_perform(client);

		if (err == ESP_OK)
		{
			ESP_LOGI(MY_TAG, "Status = %d, content_length = %d",
					esp_http_client_get_status_code(client),
					esp_http_client_get_content_length(client));
		}
		esp_http_client_cleanup(client);

		// -- wait 60 secs
		vTaskDelay(60 * 1000 / portTICK_PERIOD_MS);
	}
}

