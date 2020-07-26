/**
 * This code is public domain.
 *
 * https://www.avrfreaks.net/forum/ssd1306-lcd-initialization-commands
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

#define SSD1306_COLUMN_ADDR						0x21
#define SSD1306_PAGE_ADDR						0x22

#define LCD_WIDTH	128
#define LCD_HEIGHT	64

static uint8_t buffer[1024 + 1];


static void display_init(uint8_t addr);
static void display_flush(uint8_t addr);


#include "glyphs.h"


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
	uint8_t control = 0x00;
	uint8_t data[2];

	data[0] = control;
	data[1] = cmd;

	esp_err_t ret = i2c_master_write_slave(addr, data, 2);
	if (ret == ESP_OK)
		ESP_LOGE(MY_TAG, "%x good", cmd);
	else
		ESP_LOGE(MY_TAG, "%x bad", cmd);
}


void display_init(uint8_t addr)
{
	oledSendCommand(addr, SSD1306_DISPLAY_OFF);

	oledSendCommand(addr, SSD1306_SET_DISPLAY_CLOCK_DIV_RATIO);
	oledSendCommand(addr, 0x80);
//	oledSendCommand(addr, 0x3f);

	oledSendCommand(addr, SSD1306_SET_MULTIPLEX);
	oledSendCommand(addr, LCD_HEIGHT - 1);

	oledSendCommand(addr, SSD1306_SET_DISPLAY_OFFSET);
	oledSendCommand(addr, 0x0);

	oledSendCommand(addr, SSD1306_SET_START_LINE | 0x0);

	oledSendCommand(addr, SSD1306_CHARGE_PUMP);
	oledSendCommand(addr, 0x14);	// internal VCC

	oledSendCommand(addr, SSD1306_MEMORY_ADDR_MODE);
	oledSendCommand(addr, 0x00);	// horizontal addressing

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
	ESP_LOGI(MY_TAG, "--display");

	// set col address
#if 0
	{
		uint8_t control = 0x00;
		uint8_t data[4];

		data[0] = control;
		data[1] = SSD1306_COLUMN_ADDR;
		data[2] = 0;
		data[3] = LCD_WIDTH - 1;

		esp_err_t ret = i2c_master_write_slave(addr, data, 4);
		if (ret == ESP_OK)
			ESP_LOGE(MY_TAG, "set-col good");
		else
			ESP_LOGE(MY_TAG, "sel-col bad");
	}
#endif
	oledSendCommand(addr, SSD1306_COLUMN_ADDR);
	oledSendCommand(addr, 0);
	oledSendCommand(addr, LCD_WIDTH - 1);
	// set page addr
	oledSendCommand(addr, SSD1306_PAGE_ADDR);
	oledSendCommand(addr, 0);
	oledSendCommand(addr, LCD_HEIGHT / 8 - 1);

	// flush
	for (int i=0; i<1025; ++i)
		buffer[i] = 0x55;// i;
	buffer[0] = SSD1306_SET_START_LINE;

	size_t pos = 1;
	for (int k=0; k<9; ++k)
	{
		for (int i=0; i<8; ++i)
			buffer[pos+i] = 0;
		pos += 8;
		for (int i=0; i<8; ++i)
			buffer[pos+i] = 0xff;
		pos += 8;
	}

	for (int g=0; g<sizeof(glyphs); ++g)
	{
		for (int i=0; i<8; ++i)
			buffer[pos+g] = glyphs[g];
	}

	esp_err_t ret = i2c_master_write_slave(addr, buffer, 1025);
	if (ret == ESP_OK)
		ESP_LOGE(MY_TAG, "flush good");
	else
		ESP_LOGE(MY_TAG, "flush bad");
}
