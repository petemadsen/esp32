/**
 * This code is public domain.
 */
// Based on OTA example.
#include <string.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>

#include <esp_system.h>
#include <esp_event.h>
#include <esp_log.h>
#include <esp_ota_ops.h>
#include <esp_http_client.h>
#include <esp_flash_partitions.h>
#include <esp_partition.h>
#include <esp_sleep.h>

#include <esp_image_format.h>

#include <nvs.h>
#include <nvs_flash.h>

#include <driver/gpio.h>

#include <esp32/rom/uart.h>

#include <errno.h>

#include "config.h"
#include "common.h"
#include "system/wifi.h"
#include "system/my_settings.h"


#define BUFFSIZE			2048
#define DEFAULT_FILENAME	"ota"

#define CFG_OTA_FILENAME		"filename"
#define CFG_OTA_URL				"url"
#define CFG_OTA_LOADER			"loader"

#define DOWNLOAD_URL_MAXLEN	128
static char* ota_url;


// FIXME: does not work with current config
#define CONFIG_CONSOLE_UART_NUM 0


static const char* MY_TAG = "ota";
// an ota data write buffer ready to write to the flash
static char ota_read_buf[BUFFSIZE + 1] = { 0 };


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


static int do_connect(esp_http_client_handle_t client)
{
	esp_err_t err;

	err = esp_http_client_open(client, 0);
	if (err != ESP_OK)
	{
		ESP_LOGE(MY_TAG, "Failed to open connection: %s/%d",
				 esp_err_to_name(err), err);
		return false;
	}

	int datalen = esp_http_client_fetch_headers(client);
	if (datalen < 0)
	{
		ESP_LOGE(MY_TAG, "Failed to read headers: %d", datalen);
		return false;
	}

	return esp_http_client_get_status_code(client);
}


static bool do_download(esp_http_client_handle_t client)
{
	int file_size = esp_http_client_get_content_length(client);
	ESP_LOGI(MY_TAG, "Firmware size: %d", file_size);


	esp_err_t err;

	// update handle:
	// set by esp_ota_begin(), must be freed via esp_ota_end()
	esp_ota_handle_t update_handle = 0 ;
	const esp_partition_t* update_partition = NULL;

	update_partition = esp_ota_get_next_update_partition(NULL);
	ESP_LOGI(MY_TAG, "Writing to partition '%s' subtype %d at offset 0x%x",
			 update_partition->label,
			 update_partition->subtype, update_partition->address);
	assert(update_partition != NULL);
	if (update_partition == NULL)
	{
		ESP_LOGE(MY_TAG, "Could not set update parition!");
		return false;
	}

	err = esp_ota_begin(update_partition, OTA_WITH_SEQUENTIAL_WRITES, &update_handle);
	if (err != ESP_OK)
	{
		ESP_LOGE(MY_TAG, "esp_ota_begin() failed (%s)", esp_err_to_name(err));
		return false;
	}
	ESP_LOGI(MY_TAG, "esp_ota_begin() succeeded");

	int led_status = PROJECT_LED_PIN_ON;
	int binary_file_length = 0;
	int zero_len_packets = 0;
	// deal with all receive packet
	while (binary_file_length != file_size)
	{
		gpio_set_level(PROJECT_LED_PIN, led_status);
		led_status = !led_status;

		int data_read = esp_http_client_read(client, ota_read_buf, BUFFSIZE);
		ESP_LOGI(MY_TAG, "[Received] %.1f%% (%d) [%d] %d bytes",
				 (float)binary_file_length / (float)file_size * 100.0,
				 errno, zero_len_packets, data_read);
		if (data_read > 0)
		{
			zero_len_packets = 0;
			err = esp_ota_write(update_handle, (const void*)ota_read_buf, data_read);
			if (err != ESP_OK)
			{
				ESP_LOGE(MY_TAG, "esp_ota_begin() failed (%s)", esp_err_to_name(err));
				esp_ota_end(update_handle);
				return false;
			}
			binary_file_length += data_read;
		}
		else
		{
//			ESP_LOGE(MY_TAG, "Error: SSL data read error");
//			http_cleanup(client);
//			return false;
			/*
			 * As esp_http_client_read never returns negative error code, we rely on
			 * `errno` to check for underlying transport connectivity closure if any
			 */
			if (errno == ECONNRESET || errno == ENOTCONN) {
				ESP_LOGE(MY_TAG, "Connection closed, errno = %d", errno);
				break;
			}
			if (esp_http_client_is_complete_data_received(client)) {
				ESP_LOGI(MY_TAG, "Connection closed");
				break;
			}
			if (++zero_len_packets == 10)
			{
				ESP_LOGE(MY_TAG, "Too many zero packets.");
				break;
			}
		}
	}
	gpio_set_level(PROJECT_LED_PIN, PROJECT_LED_PIN_ON);

	ESP_LOGI(MY_TAG, "Total received: %d / %d", binary_file_length, file_size);
	if (binary_file_length != file_size)
	{
		ESP_LOGE(MY_TAG, "Not all bytes received!");
		return false;
	}

	if (esp_ota_end(update_handle) != ESP_OK)
	{
		ESP_LOGE(MY_TAG, "esp_ota_end() failed!");
		return false;
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
		return false;
	}

	return true;
}


static int download_and_install()
{
	esp_http_client_config_t config = {
		.url = ota_url,
//        .cert_pem = (char *)server_cert_pem_start,
		.timeout_ms = 2000,
	};

	ESP_LOGI(MY_TAG, "Connecting to: %s", ota_url);

	esp_http_client_handle_t client = esp_http_client_init(&config);
	if (client == NULL)
	{
		ESP_LOGE(MY_TAG, "Failed to initialise HTTP connection");
		return 1;
	}

	int statuscode = do_connect(client);
	if (statuscode != 200)
	{
		ESP_LOGE(MY_TAG, "Could not connect: %d", statuscode);
		esp_http_client_cleanup(client);
		return statuscode;
	}

	if (!do_download(client))
	{
		esp_http_client_close(client);
		esp_http_client_cleanup(client);
		return 500;
	}

	return 200;
}


#if 0
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
#endif


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
	vTaskDelay(1000 / portTICK_PERIOD_MS);

	// -- start
	const int max_tries = 5;
	bool found = false;
	for (int i=0; i<max_tries; ++i)
	{
		ESP_LOGI(MY_TAG, "Attempt %d/%d", (i+1), max_tries);

		int retcode = download_and_install();
		found = (retcode == 200);
		if (found)
			break;

		if (retcode == 404)
		{
			ESP_LOGW(MY_TAG, "File not on server. Stop.");
			break;
		}

		vTaskDelay(5000 / portTICK_PERIOD_MS);
	}

	// -- reboot
#if 0
	if (!found)
	{
		// no update found.
		// trying to boot into the existing partition again.
		ESP_LOGI(MY_TAG, "Trying to boot into an existing partition.");
		found = find_and_activate();
	}
#endif

	// Oops, will reboot into OTA but after a deep sleep.
	if (!found)
	{
		ESP_LOGE(MY_TAG, "Update failed. Rebooting into OTA. Deep sleep for 5 minutes.");
		vTaskDelay(10000 / portTICK_PERIOD_MS);

//		wifi_stop();
		uart_tx_wait_idle(CONFIG_CONSOLE_UART_NUM);
		esp_deep_sleep(300LL * 1000000LL);
	}

	ESP_LOGI(MY_TAG, "Ready to reboot system!");
	vTaskDelay(1000 / portTICK_PERIOD_MS);
	esp_restart();
}


void ota_nowifi_task(void *pvParameter)
{
	const int64_t max_seconds = 2 * 60;
	int64_t last_ok = esp_timer_get_time() / 1000 / 1000;
	for (;;)
	{
		EventBits_t bits = xEventGroupGetBits(wifi_event_group);
		if (bits & WIFI_CONNECTED)
		{
			last_ok = esp_timer_get_time() / 1000 / 1000;
			vTaskDelay(1000 / portTICK_PERIOD_MS);
			continue;
		}

#if 0
		gpio_set_level(PROJECT_LED_PIN, PROJECT_LED_PIN_ON);
		vTaskDelay(1000 / portTICK_PERIOD_MS);
		gpio_set_level(PROJECT_LED_PIN, PROJECT_LED_PIN_OFF);
		vTaskDelay(1000 / portTICK_PERIOD_MS);
#else
		vTaskDelay(1000 / portTICK_PERIOD_MS);
#endif

		int64_t now = esp_timer_get_time() / 1000 / 1000;
		int64_t diff = now - last_ok;
		if ((diff % 10) == 0)
			ESP_LOGE(MY_TAG, "NoWiFi watch - NO WIFI - %lld", diff);

		if ( diff > max_seconds)
		{
			ESP_LOGI(MY_TAG, "NoWiFi deep sleep.");

			wifi_stop();
			uart_tx_wait_idle(CONFIG_CONSOLE_UART_NUM);
			esp_deep_sleep(300LL * 1000000LL);
		}
	}
}


const char* ota_get_url(void)
{
	return ota_url;
}


void ota_init()
{
	// -- save our version
	settings_set_str(STORAGE_OTA, CFG_OTA_LOADER, PROJECT_VERSION, false);

	// -- NEW: try to read the complete url first
	if (settings_get_str(STORAGE_OTA, CFG_OTA_URL, &ota_url, false) == ESP_OK)
		return;

	// -- OLD: get filename
	ESP_LOGI(MY_TAG, "Fallback to OLD");
	char* filename = strdup(DEFAULT_FILENAME);
	settings_get_str(STORAGE_OTA, CFG_OTA_FILENAME, &filename, false);

	int len;
	ota_url = malloc(DOWNLOAD_URL_MAXLEN + 1);
	len = snprintf(ota_url, DOWNLOAD_URL_MAXLEN,
				   "%s/ota/file/%s",
				   PROJECT_SHUTTERS_ADDRESS, filename);
	if (len >= DOWNLOAD_URL_MAXLEN)
	{
		ESP_LOGE(MY_TAG, "Download URL too long! This should never happen!");
		ota_fatal_error();
	}
}
