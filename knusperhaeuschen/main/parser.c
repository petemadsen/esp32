#include "sdkconfig.h"

#include <stdio.h>
#include <string.h>

#include <stdbool.h>

#include <esp_system.h>
#include <esp_log.h>
#include <esp_ota_ops.h>
#include <esp_spi_flash.h>
#include <esp_partition.h>

// ota-only
#include "esp_image_format.h"

#include "config.h"
#include "light.h"

//static const char* M_TAG = "DFPLAYER/parser";

static const char* HELP = "knusperhaeuschen: ident status light ota exit quit reboot\n";
static const char* IDENT = "knusperhaeuschen\n";
static const char* RET_ERR = "ERR\n";
static const char* RET_OK = "OK\n";
static const char* NOFACTORY = "NOFACTORY\n";
static const char* NOBOOT = "NOBOOT\n";
static const char* VERSION = "0.0.1";


static const char* run_ota();


static char buffer[80];
const char* parse_input(char* data, int data_len)
{
	char* line = strtok(data, "\n");

	if (strstr(line, "exit") == line || strstr(line, "quit") == line)
		return NULL;

	if (strstr(line, "reboot") == line)
		esp_restart();

	if (strstr(line, "light ") == line)
	{
		int onoff;
		if (sscanf(line, "light %d", &onoff)==1 && onoff>=0 && onoff<=1)
		{
			if (onoff == 0)
				light_off();
			else
				light_on();
			return RET_OK;
		}
		else
			return RET_ERR;
	}
	else if (strstr(line, "status") == line)
	{
		sprintf(buffer, "version: %s light: %d free-ram: %d\n",
				VERSION,
				light_status(),
				esp_get_free_heap_size());
		return buffer;
	}
	else if (strstr(line, "ota") == line)

	{
		return run_ota();
	}
	else if (strstr(line, "ident") == line)
	{
		return IDENT;
	}

	return HELP;
}


void parser_init()
{
}


static const char* run_ota()
{
	const esp_partition_t* factory = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_ANY, "factory");
	if (!factory)
		return NOFACTORY;

	if (esp_ota_set_boot_partition(factory) != ESP_OK)
		return NOBOOT;

	esp_restart();
}

