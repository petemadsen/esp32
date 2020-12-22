#include "sdkconfig.h"

#include <stdio.h>
#include <string.h>

#include <stdbool.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <esp_system.h>
#include <esp_log.h>
#include <esp_ota_ops.h>
#include <esp_spi_flash.h>
#include <esp_partition.h>

// ota-only
#include "esp_image_format.h"

#include "config.h"
#include "config.h"
#include "common.h"


static const char* HELP = "HELP: status ota exit quit reboot mode NUM modedesc NUM ident\n";
static const char* RET_ERR = "ERR\n";
static const char* RET_OK = "OK\n";
static const char* NOFACTORY = "NOFACTORY\n";
static const char* NOBOOT = "NOBOOT\n";


static const char* run_ota();
static void reboot_me(void* pvParameters);


static const char* MY_TAG = "xmastree/parser";


static char buffer[160];
const char* parse_input(char* data, int data_len)
{
	char* line = strtok(data, "\n");

	if (strstr(line, "exit") == line || strstr(line, "quit") == line)
		return NULL;

	if (strstr(line, "reboot") == line)
	{
		xTaskCreate(&reboot_me, "reboot_me", 1024, NULL, 5, NULL);
		return NULL;
	}

	if (strstr(line, "status") == line)
	{
		sprintf(buffer, "version: %s free-ram: %d wifi-reconnects: %u",
				PROJECT_VERSION,
				esp_get_free_heap_size(),
				g_wifi_reconnects);
	}
	else if (strstr(line, "ota") == line)
	{
		return run_ota();
	}
	else if (strstr(line, "ident") == line)
	{
		return MY_TAG;
	}
	else
	{
		return HELP;
	}

	return buffer;
}


void parser_init()
{
}


static void reboot_me(void* pvParameters)
{
	vTaskDelay(1000 / portTICK_PERIOD_MS);
	esp_restart();
}


static const char* run_ota()
{
	const esp_partition_t* factory = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_ANY, "factory");
	if (!factory)
		return NOFACTORY;

	if (esp_ota_set_boot_partition(factory) != ESP_OK)
		return NOBOOT;

//	esp_restart();
	xTaskCreate(&reboot_me, "reboot_me", 1024, NULL, 5, NULL);
	return NULL;
}

