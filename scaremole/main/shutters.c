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
#include "my_sleep.h"
#include "ota.h"


static const char* MY_TAG = "scaremole/shutters";


#define IP_ADDRESS "192.168.1.86:8080"
#define IP_ADDRESS "192.168.1.51:8080"


static const char* UPDATE_URL = "http://" IP_ADDRESS "/scaremole/touch";
static const char* SAVE_URL = "http://" IP_ADDRESS "/scaremole/save";


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
	esp_err_t err;
	for (;;)
	{
		xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED, false, true,
				portMAX_DELAY);
		ESP_LOGI(MY_TAG, "Run.");

		// -- check for update
		esp_http_client_config_t config = {
			.url = UPDATE_URL,
			.event_handler = _http_event_handle,
		};
		esp_http_client_handle_t client = esp_http_client_init(&config);
		err = esp_http_client_perform(client);
		if (err == ESP_OK)
		{
			int code = esp_http_client_get_status_code(client);
			ESP_LOGI(MY_TAG, "Touch/Status = %d, content_length = %d",
					 code,
					 esp_http_client_get_content_length(client));

			// need update
			if (code == 200)
			{
				ota_reboot();
				vTaskDelay(1000 / portTICK_PERIOD_MS);
			}

			my_sleep_now("could touch");
		}

#if 0
		// -- save
		const size_t POST_MAXLEN = 200;
		char* save_data = malloc(POST_MAXLEN);
		int save_data_len = snprintf(save_data, POST_MAXLEN,
									 "board_temp=%.2f",
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
#endif

		// -- cleanup
		esp_http_client_cleanup(client);

		// -- wait 3600 secs = 1h
		vTaskDelay(3600 * 1000 / portTICK_PERIOD_MS);
	}
}

