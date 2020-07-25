/**
 * This code is public domain.
 */
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>

#include "oled.h"

#include "system/my_i2c.h"


static const char* MY_TAG = "scaremole/oled";


#define SSD1306_DISPLAY_OFF						0xae
#define SSD1306_DISPLAY_ON						0xaf
#define SSD1306_SET_START_LINE					0x40
#define SSD1306_SET_DISPLAY_OFFSET				0xd3
#define SSD1306_SET_DISPLAY_CLOCK_DIV_RATIO		0xd5
#define SSD1306_SET_MULTIPLEX					0xa8
#define SSD1306_CHARGE_PUMP						0x8d
#define SSD1306_MEMORY_ADDR_MODE				0x20
#define SSD1306_SET_PRECHARGE_PERIOD			0xd9
#define SSD1306_NORMAL_DISPLAY					0xa6
#define SSD1306_DISPLAY_ALL_ON_RESUME			0xa4
#define SSD1306_SET_VCOM_DESELECT				0xdb
#define SSD1306_SET_COM_PINS					0xda
#define SSD1306_SET_CONTRAST_CONTROL			0x81
#define SSD1306_COM_SCAN_DIR_DEC				0xc8
#define SSD1306_SET_SEGMENT_REMAP				0xa0

#define SSD1306_SET_CONTRAST					0x81


static void display_init(uint8_t addr);
static void display_flush(uint8_t addr);


void oled_task(void* pvParameters)
{
	i2c_master_init();

	for (;;)
	{
		int found_devices = 0;
		for (uint8_t addr=0; addr<128; ++addr)
		{
			esp_err_t ret = i2c_master_scan(addr);
			if (ret == ESP_OK)
			{
				++found_devices;
				ESP_LOGI(MY_TAG, "FOUND: %d", addr);
				display_init(addr);
				display_flush(addr);
			}
		}
		ESP_LOGE(MY_TAG, "I2C scanning finished: %d found", found_devices);

		vTaskDelay(60 * 1000 / portTICK_PERIOD_MS);
	}
}

static void oledSendCommand(uint8_t addr, uint8_t cmd)
{
	uint8_t data[1];

	data[0] = cmd;
	esp_err_t ret = i2c_master_write_slave(addr, data, 1);
	if (ret == ESP_OK)
		ESP_LOGE(MY_TAG, "%x good", cmd);
	else
		ESP_LOGE(MY_TAG, "%x bad", cmd);
}


void display_init(uint8_t addr)
{
	uint8_t height = 32;

	oledSendCommand(addr, SSD1306_DISPLAY_OFF);

	oledSendCommand(addr, SSD1306_SET_DISPLAY_CLOCK_DIV_RATIO);
	oledSendCommand(addr, 0x80);
//	oledSendCommand(addr, 0x3f);

	oledSendCommand(addr, SSD1306_SET_MULTIPLEX);
	oledSendCommand(addr, height - 1);

	oledSendCommand(addr, SSD1306_SET_DISPLAY_OFFSET);
	oledSendCommand(addr, 0x0);

	oledSendCommand(addr, SSD1306_SET_START_LINE | 0x0);

	oledSendCommand(addr, SSD1306_CHARGE_PUMP);
	oledSendCommand(addr, 0x14);

	oledSendCommand(addr, SSD1306_MEMORY_ADDR_MODE);
	oledSendCommand(addr, 0x00);

	oledSendCommand(addr, SSD1306_SET_SEGMENT_REMAP | 0x1);

	oledSendCommand(addr, SSD1306_COM_SCAN_DIR_DEC);

	oledSendCommand(addr, SSD1306_SET_COM_PINS);
	oledSendCommand(addr, 0x12);

	oledSendCommand(addr, SSD1306_SET_CONTRAST_CONTROL);
	oledSendCommand(addr, 0xCF);

	oledSendCommand(addr, SSD1306_SET_PRECHARGE_PERIOD);
	oledSendCommand(addr, 0xF1);

	oledSendCommand(addr, SSD1306_SET_VCOM_DESELECT);
	oledSendCommand(addr, 0x40);

	oledSendCommand(addr, SSD1306_DISPLAY_ALL_ON_RESUME);

	oledSendCommand(addr, SSD1306_NORMAL_DISPLAY);

	oledSendCommand(addr, SSD1306_DISPLAY_ON);
}


void display_flush(uint8_t addr)
{
	// begin transaction

	// flush
//	oledSendCommand(addr,

	// end transaction
}
