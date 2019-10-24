/**
 * Based on OTA example.

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include "sdkconfig.h"

#include <string.h>
#include <sys/socket.h>
#include <netdb.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>

#include <esp_system.h>
#include <esp_wifi.h>
#include <esp_event_loop.h>
#include <esp_log.h>
#include <esp_ota_ops.h>
#include <esp_http_client.h>
#include <esp_flash_partitions.h>
#include <esp_partition.h>
#include <esp_sleep.h>

#include <driver/gpio.h>

#include <esp_image_format.h>

#include <nvs.h>
#include <nvs_flash.h>

#include <rom/uart.h>

#include "config.h"
#include "common.h"
#include "system/wifi.h"


#define BUFFSIZE			1024
#define TEXT_BUFFSIZE		1024
#define DEFAULT_FILENAME	"peterpan.bin"
#define DEFAULT_SERVER_IP	"192.168.1.51"
#define DEFAULT_SERVER_PORT	"8081"

#define DOWNLOAD_URL_MAXLEN	80
static char download_url[DOWNLOAD_URL_MAXLEN];


#define CFG_OTA_STORAGE			"ota"
#define CFG_OTA_FILENAME		"filename"
#define CFG_OTA_FILENAME_LENGTH	30
static char ota_filename[CFG_OTA_FILENAME_LENGTH];


static const char* MY_TAG = "ota";
/*an ota data write buffer ready to write to the flash*/
static char ota_write_data[BUFFSIZE + 1] = { 0 };


#if 0
// Event group to signal when we are connected & ready to make a request */
static EventGroupHandle_t wifi_event_group;
static const EventBits_t CONNECTED_BIT = BIT0;


static esp_err_t event_handler(void *ctx, system_event_t *event)
{
    switch (event->event_id)
	{
    case SYSTEM_EVENT_STA_START:
        esp_wifi_connect();
        break;
    case SYSTEM_EVENT_STA_GOT_IP:
        xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
        /* This is a workaround as ESP32 WiFi libs don't currently
           auto-reassociate. */
        esp_wifi_connect();
        xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
        break;
    default:
        break;
    }
    return ESP_OK;
}


static void initialise_wifi(void)
{
    tcpip_adapter_init();
    wifi_event_group = xEventGroupCreate();
	ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&cfg) );
	ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    wifi_config_t wifi_config = {
        .sta = {
			.ssid = CONFIG_SSID,
			.password = CONFIG_PASS,
        },
    };
	ESP_LOGI(MY_TAG, "Setting WiFi configuration SSID %s...", wifi_config.sta.ssid);
	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
	ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
	ESP_ERROR_CHECK(esp_wifi_start());
}
#endif


static void http_cleanup(esp_http_client_handle_t client)
{
    esp_http_client_close(client);
    esp_http_client_cleanup(client);
}


static void __attribute__((noreturn)) ota_fatal_error()
{
//	ESP_LOGE(MY_TAG, "Exiting task due to fatal error...");
//    (void)vTaskDelete(NULL);

	while (1)
	{
		vTaskDelay(60 * 1000 / portTICK_PERIOD_MS);
		esp_restart();
    }
}


static bool download_and_install()
{
	esp_err_t err;
	/* update handle : set by esp_ota_begin(), must be freed via esp_ota_end() */
	esp_ota_handle_t update_handle = 0 ;
	const esp_partition_t* update_partition = NULL;

	esp_http_client_config_t config = {
		.url = download_url,
//        .cert_pem = (char *)server_cert_pem_start,
	};
	esp_http_client_handle_t client = esp_http_client_init(&config);
	if (client == NULL)
	{
		ESP_LOGE(MY_TAG, "Failed to initialise HTTP connection");
		return false;
//		ota_fatal_error();
	}
	err = esp_http_client_open(client, 0);
	if (err != ESP_OK)
	{
		ESP_LOGE(MY_TAG, "Failed to open HTTP connection: %s", esp_err_to_name(err));
		esp_http_client_cleanup(client);
		return false;
//		ota_fatal_error();
	}

	int datalen = esp_http_client_fetch_headers(client);
	if (datalen <= 0)
	{
		ESP_LOGE(MY_TAG, "Failed to read headers: %d", datalen);
		esp_http_client_cleanup(client);
		return false;
	}

	int statuscode = esp_http_client_get_status_code(client);
	if (statuscode != 200)
	{
		ESP_LOGE(MY_TAG, "Image not found: %d", statuscode);
		esp_http_client_cleanup(client);
		return false;
	}

	update_partition = esp_ota_get_next_update_partition(NULL);
	ESP_LOGI(MY_TAG, "Writing to partition subtype %d at offset 0x%x",
			 update_partition->subtype, update_partition->address);
	assert(update_partition != NULL);

	err = esp_ota_begin(update_partition, OTA_SIZE_UNKNOWN, &update_handle);
	if (err != ESP_OK)
	{
		ESP_LOGE(MY_TAG, "esp_ota_begin failed (%s)", esp_err_to_name(err));
		http_cleanup(client);
		ota_fatal_error();
	}
	ESP_LOGI(MY_TAG, "esp_ota_begin succeeded");

	int binary_file_length = 0;
	/* deal with all receive packet */
	while (1)
	{
		int data_read = esp_http_client_read(client, ota_write_data, BUFFSIZE);
		if (data_read < 0)
		{
			ESP_LOGE(MY_TAG, "Error: SSL data read error");
			http_cleanup(client);
			ota_fatal_error();
		}
		else if (data_read > 0)
		{
			err = esp_ota_write( update_handle, (const void *)ota_write_data, data_read);
			if (err != ESP_OK)
			{
				http_cleanup(client);
				ota_fatal_error();
			}
			binary_file_length += data_read;
			ESP_LOGD(MY_TAG, "Written image length %d", binary_file_length);
		}
		else if (data_read == 0)
		{
			ESP_LOGI(MY_TAG, "Connection closed,all data received");
			break;
		}
	}
	ESP_LOGI(MY_TAG, "Total Write binary data length : %d", binary_file_length);

	if (esp_ota_end(update_handle) != ESP_OK)
	{
		ESP_LOGE(MY_TAG, "esp_ota_end failed!");
		http_cleanup(client);
		ota_fatal_error();
	}

#if 0
	if (esp_partition_check_identity(esp_ota_get_running_partition(), update_partition) == true)
	{
		ESP_LOGI(MY_TAG, "The current running firmware is same as the firmware just downloaded");
		int i = 0;
		ESP_LOGI(MY_TAG, "When a new firmware is available on the server, press the reset button to download it");
		while(1)
		{
			ESP_LOGI(MY_TAG, "Waiting for a new firmware ... %d", ++i);
			vTaskDelay(2000 / portTICK_PERIOD_MS);
		}
	}
#endif

	err = esp_ota_set_boot_partition(update_partition);
	if (err != ESP_OK)
	{
		ESP_LOGE(MY_TAG, "esp_ota_set_boot_partition failed (%s)!", esp_err_to_name(err));
		http_cleanup(client);
		ota_fatal_error();
	}

	return true;
}


static bool find_and_activate()
{
	// no update found, try to boot a valid ota partition
	esp_partition_iterator_t ota = esp_partition_find(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_ANY, NULL);
	while (ota)
	{
		const esp_partition_t* part = esp_partition_get(ota);
		ESP_LOGI(MY_TAG, "Check partition: %s", part->label);

		if (part->subtype != ESP_PARTITION_SUBTYPE_APP_FACTORY)
		{
			esp_image_metadata_t data;
			const esp_partition_pos_t part_pos = {
					.offset = part->address,
					.size = part->size,
			};
			if (esp_image_verify(ESP_IMAGE_VERIFY, &part_pos, &data) == ESP_OK)
			{
				ESP_LOGE(MY_TAG, "Found one: %s", part->label);
				esp_ota_set_boot_partition(part);
				return true;
			}
		}

		ota = esp_partition_next(ota);
	}

	return false;
}


void ota_task(void *pvParameter)
{
	ESP_LOGI(MY_TAG, "Starting OTA...");

	const esp_partition_t* configured = esp_ota_get_boot_partition();
	const esp_partition_t* running = esp_ota_get_running_partition();

	if (configured != running)
	{
		ESP_LOGW(MY_TAG, "Configured OTA boot partition at offset 0x%08x, but running from offset 0x%08x",
                 configured->address, running->address);
		ESP_LOGW(MY_TAG, "(This can happen if either the OTA boot data or preferred boot image become corrupted somehow.)");
    }
	ESP_LOGI(MY_TAG, "Running partition type %d subtype %d (offset 0x%08x)",
             running->type, running->subtype, running->address);

	// -- wait for wifi
	xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED,
                        false, true, portMAX_DELAY);
	ESP_LOGI(MY_TAG, "Connected to Wifi! Start to connect to server....");
    
	// -- start
	bool found = false;
	for (int i=0; i<5; ++i)
	{
		ESP_LOGI(MY_TAG, "Attempt #%d", (i+1));
		gpio_set_level(PROJECT_LED_PIN, PROJECT_LED_PIN_ON);

		found = download_and_install();
		if (found)
			break;

		gpio_set_level(PROJECT_LED_PIN, PROJECT_LED_PIN_OFF);
		vTaskDelay(5000 / portTICK_PERIOD_MS);
	}

	// -- reboot
	if (!found)
	{
		// no update found. try to boot into the existing partition again.
		found = find_and_activate();
	}

	// Oops, will reboot into OTA but after a deep sleep.
	if (!found)
	{
		ESP_LOGE(MY_TAG, "Update failed. Rebooting into OTA. Deep sleep.");
		vTaskDelay(10000 / portTICK_PERIOD_MS);

		esp_wifi_stop();
		uart_tx_wait_idle(CONFIG_CONSOLE_UART_NUM);
		esp_deep_sleep(300LL * 1000000LL);
	}

	ESP_LOGI(MY_TAG, "Ready to restart system!");
	vTaskDelay(1000 / portTICK_PERIOD_MS);
	esp_restart();
}


esp_err_t read_ota()
{
	esp_err_t err;

	nvs_handle my_handle;
    err = nvs_open(CFG_OTA_STORAGE, NVS_READWRITE, &my_handle);
	if (err != ESP_OK)
	{
		ESP_LOGE(MY_TAG, "Could not open storage: %s", CFG_OTA_STORAGE);
		return err;
	}

	size_t keylen;
	err = nvs_get_str(my_handle, CFG_OTA_FILENAME, NULL, &keylen);
	if (err != ESP_OK)
	{
		ESP_LOGE(MY_TAG, "Key not found: %s", CFG_OTA_FILENAME);
		return err;
	}
	if (keylen > CFG_OTA_FILENAME_LENGTH)
	{
		ESP_LOGE(MY_TAG, "Filename too long: %u", keylen);
		return ESP_FAIL;
	}

	err = nvs_get_str(my_handle, CFG_OTA_FILENAME, ota_filename, &keylen);
	ESP_LOGI(MY_TAG, "Filename to load: %s", ota_filename);

	return ESP_OK;
}


#if 0
static void nowifi_watch_task(void *pvParameter)
{
	const int64_t max_seconds = 2 * 60;
	int64_t last_ok = esp_timer_get_time() / 1000 / 1000;
	for (;;)
	{
		EventBits_t bits = xEventGroupGetBits(wifi_event_group);
		if (bits & CONNECTED_BIT)
		{
			last_ok = esp_timer_get_time() / 1000 / 1000;
			vTaskDelay(1000 / portTICK_PERIOD_MS);
			continue;
		}

		gpio_set_level(PROJECT_LED_PIN, PROJECT_LED_PIN_ON);
		vTaskDelay(1000 / portTICK_PERIOD_MS);
		gpio_set_level(PROJECT_LED_PIN, PROJECT_LED_PIN_OFF);
		vTaskDelay(1000 / portTICK_PERIOD_MS);

		int64_t now = esp_timer_get_time() / 1000 / 1000;
		int64_t diff = now - last_ok;
		if ((diff % 10) == 0)
			ESP_LOGE(MY_TAG, "NoWiFi watch - NO WIFI - %lld", diff);

		if ( diff > max_seconds)
		{
			ESP_LOGI(MY_TAG, "NoWiFi deep sleep.");

			esp_wifi_stop();
			uart_tx_wait_idle(CONFIG_CONSOLE_UART_NUM);
			esp_deep_sleep(300LL * 1000000LL);
		}
	}
}
#endif



void ota_init()
{
	// create download url
	if (read_ota() != ESP_OK)
	{
		strcpy(ota_filename, DEFAULT_FILENAME);
		ESP_LOGE(MY_TAG, "Using default filename: %s", DEFAULT_FILENAME);
	}
	ESP_LOGI(MY_TAG, "Using filename: %s", ota_filename);
	int len = snprintf(download_url, DOWNLOAD_URL_MAXLEN, "http://%s:%s/%s",
			DEFAULT_SERVER_IP, DEFAULT_SERVER_PORT, ota_filename);
	if (len >= DOWNLOAD_URL_MAXLEN)
	{
		ESP_LOGE(MY_TAG, "Download URL too long!");
		ota_fatal_error();
	}
	ESP_LOGI(MY_TAG, "Using url: %s", download_url);
}
