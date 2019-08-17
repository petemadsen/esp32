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

static const char* M_TAG = "DFPLAYER/parser";

static const char* HELP = "HELP: read status ota exit quit reboot\n";
static const char* IDENT = "awPower\n";
static const char* ERR = "ERR\n";
static const char* NOCONNECTION = "NOCONNECTION\n";
static const char* NODEVICES = "NODEVICES\n";
static const char* NOFACTORY = "NOFACTORY\n";
static const char* NOBOOT = "NOBOOT\n";


static const char* run_ota();


static char buffer[80];
const char* parse_input(char* data, int data_len)
{
	char* line = strtok(data, "\n");

	if (strstr(line, "exit") == line || strstr(line, "quit") == line)
		return NULL;
#if 0
	if (strstr(line, "reboot") == line)
		esp_restart();

	if (strstr(line, "read ") == line)
	{
		int channel;
		if (sscanf(line, "read %d", &channel)==1 && channel>=0 && channel<NUM_INA_MODULES && m_modules[channel].calibrated)
			return read_ina219(channel);
		else
			return ERR;
	}
	else if (strstr(line, "status") == line)
	{
		sprintf(buffer, "version: 0.0.2 free-ram: %d\n", esp_get_free_heap_size());
	}
	else if (strstr(line, "ota") == line)
	{
		return run_ota();
	}
	else if (strstr(line, "scan") == line)
	{
		return run_scan();
	}
	else if (strstr(line, "ident") == line)
	{
		return IDENT;
	}
	else
	{
		return HELP;
	}
#endif
	return HELP;
}


void parser_init()
{
#if 0
	ESP_ERROR_CHECK(i2c_master_init(GPIO_NUM_19, GPIO_NUM_21));

	for (int i=0; i<NUM_INA_MODULES; ++i)
	{
		m_modules[i].i2c_addr = 0x40 + i;
		m_modules[i].r_shunt = 0.1;
		m_modules[i].calibrated = (i2c_master_scan(0x40 + i) == ESP_OK);
#ifdef MY_DEBUG
		m_modules[i].calibrated = (i==0 || i==2);
#endif

		if (m_modules[i].calibrated)
			ina219_init(&m_modules[i]);
	}
#endif
}


#if 0
static const char* read_ina219(int channel)
{
	ina219_record_t record;
#ifdef MY_DEBUG
	record.bus_voltage = (float)channel + 3.3 + (float)(rand() % 5) / 10.0f;	// some noise
	record.shunt_current = (float)channel + 0.036 + (float)(rand() % 10) / 100.f;	// some noise
#else
	if (!ina219_exec(&m_modules[channel], &record))
		return NOCONNECTION;
#endif

	sprintf(buffer, "V %.3f A %.3f\n", record.bus_voltage, record.shunt_current);
	return buffer;
}
#endif


static const char* run_ota()
{
	const esp_partition_t* factory = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_ANY, "factory");
	if (!factory)
		return NOFACTORY;

	if (esp_ota_set_boot_partition(factory) != ESP_OK)
		return NOBOOT;

	esp_restart();
}

