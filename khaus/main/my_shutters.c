/**
 * http_request example.
 */
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>
#include <esp_log.h>

#include <lwip/err.h>

#include <esp_http_client.h>

#include <string.h>

#include "wifi.h"
#include "my_sensors.h"
#include "light.h"
#include "common.h"
#include "ota.h"
#include "my_settings.h"


static const char* MY_TAG = "khaus/shutters";


#define DEFAULT_TOUCH_URL	PROJECT_SHUTTERS_ADDRESS "/khaus/touch"
#define DEFAULT_SAVE_URL	PROJECT_SHUTTERS_ADDRESS "/khaus/save"


#define SETTING_TOUCH_URL	"shutters.touch"
#define SETTING_SAVE_URL	"shutters.save"


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
	char* touch_url = strdup(DEFAULT_TOUCH_URL);
	settings_get_str(SETTING_TOUCH_URL, &touch_url, true);

	char* save_url = strdup(DEFAULT_SAVE_URL);
	settings_get_str(SETTING_SAVE_URL, &save_url, true);

	esp_err_t err;
	for (;;)
	{
		xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED, false, true,
				portMAX_DELAY);
		ESP_LOGI(MY_TAG, "Run.");

		settings_get_str(SETTING_TOUCH_URL, &touch_url, true);
		settings_get_str(SETTING_SAVE_URL, &save_url, true);

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

		// -- touch
		esp_http_client_config_t config = {
			.url = touch_url,
			.event_handler = _http_event_handle,
		};
		esp_http_client_handle_t client = esp_http_client_init(&config);
		err = esp_http_client_perform(client);
		if (err == ESP_OK)
		{
			ESP_LOGI(MY_TAG, "Touch/Status = %d, content_length = %d",
					esp_http_client_get_status_code(client),
					esp_http_client_get_content_length(client));
		}

		// -- save
		const size_t POST_MAXLEN = 200;
		char* save_data = malloc(POST_MAXLEN);
		int save_data_len = snprintf(save_data, POST_MAXLEN,
									 "board_temp=%.2f"
									 "&board_voltage=%.2f"
									 "&out_temp=%.2f"
									 "&light=%d",
									 my_sensors_board_temp(),
									 my_sensors_board_voltage(),
									 -1.0,
									 light_status());

		esp_http_client_set_url(client, save_url);
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

		// -- cleanup
		esp_http_client_cleanup(client);

		// -- wait 3600 secs = 1h
		vTaskDelay(3600 * 1000 / portTICK_PERIOD_MS);
	}
}

