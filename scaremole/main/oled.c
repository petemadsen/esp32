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
static size_t buffer_pointer = 0;


static void display_init(uint8_t addr);


#if 1
#include "glyphs.h"
#else
const uint8_t glyphs[] = {
	0x00, 0x03, 0x0e, 0x38, 0x60, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x7f, 0x41, 0x41, 0x7f, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x7f, 0x00, 0x00,
	0x00, 0x00, 0x4f, 0x49, 0x49, 0x79, 0x00, 0x00,	// 50
	0x00, 0x00, 0x49, 0x49, 0x49, 0x7f, 0x00, 0x00,
	0x00, 0x00, 0x78, 0x08, 0x08, 0x7f, 0x00, 0x00,
	0x00, 0x00, 0x79, 0x49, 0x49, 0x4f, 0x00, 0x00,
	0x00, 0x00, 0x7f, 0x49, 0x49, 0x4f, 0x00, 0x00,
	0x00, 0x00, 0x40, 0x40, 0x40, 0x7f, 0x00, 0x00,
	0x00, 0x00, 0x7f, 0x49, 0x49, 0x7f, 0x00, 0x00,
	0x00, 0x00, 0x79, 0x49, 0x49, 0x7f, 0x00, 0x00,
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x7d, 0x01,
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x7d, 0x01,
};
#endif


uint8_t oled_init()
{
	esp_err_t ret;

	do {
		ret = i2c_master_init();
		if (ret != ESP_OK)
		{
			ESP_LOGE(MY_TAG, "I2C master init error: %s", esp_err_to_name(ret));
			vTaskDelay(60 * 1000 / portTICK_PERIOD_MS);
		}
	} while(ret != ESP_OK);

	ESP_LOGI(MY_TAG, "Ready to go.");

	for (uint8_t addr=0; addr<128; ++addr)
	{
		esp_err_t ret = i2c_master_scan(addr);
		if (ret == ESP_OK)
		{
			display_init(addr);
			return addr;
		}
	}

	return OLED_INVALID_DEVICE;
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


void oled_clear(uint8_t addr)
{
	ESP_LOGI(MY_TAG, "--display");

	// set col address
	oledSendCommand(addr, SSD1306_COLUMN_ADDR);
	oledSendCommand(addr, 0);
	oledSendCommand(addr, LCD_WIDTH - 1);
	// set page addr
	oledSendCommand(addr, SSD1306_PAGE_ADDR);
	oledSendCommand(addr, 0);
	oledSendCommand(addr, LCD_HEIGHT / 8 - 1);

	// flush
	for (int i=0; i<1025; ++i)
		buffer[i] = 0;
	buffer[0] = SSD1306_SET_START_LINE;

	buffer_pointer = 1;
}


void oled_flush(uint8_t addr)
{
	esp_err_t ret = i2c_master_write_slave(addr, buffer, sizeof(buffer));
	if (ret == ESP_OK)
		ESP_LOGE(MY_TAG, "flush good");
	else
		ESP_LOGE(MY_TAG, "flush ĵbad");
}


void oled_print(int x, int y, const char* text)
{
	char ch;

	if (x < 1 || x > LCD_WIDTH / 8)
		return;
	if (y < 1 || y > LCD_HEIGHT / 8)
		return;

	buffer_pointer = 1 + (y - 1) * LCD_WIDTH + (x - 1) * 8;

	while ((ch = *text++))
	{
		int g = (int)ch * 8;
		for (int i=0; i<8; ++i)
			buffer[buffer_pointer + i] = glyphs[g+i];
		buffer_pointer += 8;
		if (buffer_pointer >= sizeof(buffer))
			break;
	}
}
