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

#include "my_sensors.h"
#include "my_lights.h"
#include "common.h"
#include "system/ota.h"
#include "system/my_settings.h"
#include "system/wifi.h"


static const char* MY_TAG = PROJECT_TAG("shutters");


#define DEFAULT_SAVE_URL	PROJECT_SHUTTERS_ADDRESS "/khaus/save"


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
	char* save_url = strdup(DEFAULT_SAVE_URL);
	settings_get_str(SETTING_SAVE_URL, &save_url, true);

	esp_err_t err;
	for (;;)
	{
		xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED, false, true,
				portMAX_DELAY);
		ESP_LOGI(MY_TAG, "Run.");

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
			ESP_LOGI(MY_TAG, "No OTA update available.");

		// -- save
		ESP_LOGI(MY_TAG, "Save: %s", save_url);
		const size_t POST_MAXLEN = 300;
		char* save_data = malloc(POST_MAXLEN);
		int save_data_len = snprintf(save_data, POST_MAXLEN,
									 "board_temp=%.2f"
									 "&board_voltage=%.2f"
									 "&out_temp=%.2f"
									 "&out_humidity=%.2f"
									 "&light=%d",
									 my_sensors_board_temp(),
									 my_sensors_board_voltage(),
									 my_sensors_out_temp(),
									 my_sensors_out_humidity(),
									 lamp_status());

		esp_http_client_config_t config = {
			.url = save_url,
			.event_handler = _http_event_handle,
		};
		esp_http_client_handle_t client = esp_http_client_init(&config);
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

#if 0
	time_t now;
	struct tm timeinfo;

	for (int i=0; i<3; ++i)
	{
		time_t ret = time(&now);
		printf("--T %ld (%d)\n", now, ret == ((time_t)-1));
		localtime_r(&now, &timeinfo);
		printf("--Y %d\n", timeinfo.tm_year);
		printf("--x %d\n", timeinfo.tm_isdst);
		if (timeinfo.tm_year != 0 && timeinfo.tm_year < 1900)
		{
			timeinfo.tm_year += 1900;
			break;
		}

		ESP_LOGW(MY_TAG, "--time");
		vTaskDelay(2000 / portTICK_PERIOD_MS);
	}

	ESP_LOGW(MY_TAG, "TIME: %02d:%02d / %04d",
			 timeinfo.tm_hour, timeinfo.tm_min,
			 timeinfo.tm_year);

	ESP_LOGW(MY_TAG, "Turning off WiFi.");
	if (lamp_status() == 0)
	{
		wifi_stop(); // to save energy

		// wait 1h
		vTaskDelay(60 * 1000 / portTICK_PERIOD_MS);
//		vTaskDelay(3600 * 1000 / portTICK_PERIOD_MS);

		// wait for the lamp to turn off
		while (lamp_status() != 0)
			vTaskDelay(60 * 1000 / portTICK_PERIOD_MS);

		if (lamp_status() == 0)
			esp_restart();
	}
#endif

		// -- wait 3600 secs = 1h
		vTaskDelay(3600 * 1000 / portTICK_PERIOD_MS);
	}
}

