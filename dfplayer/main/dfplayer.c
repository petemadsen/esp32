/**
 * This code is public domain.
 */
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>

#include "dfplayer.h"

#include <stdio.h>
#include <string.h>
#include <math.h>

#include "esp_err.h"
#include "esp_log.h"
#include "driver/uart.h"
#include "soc/rmt_reg.h"
#include "my_settings.h"

#include "sdkconfig.h"

static const char* MY_TAG = "knusperhaeuschen/dfplayer";


static const int uart_num = UART_NUM_2;

static int dfplayer_vol = 33;
static int dfplayer_bell_track = 1;
static int dfplayer_play_track = 1;

static EventGroupHandle_t x_events;
#define EVENT_BELL		(1 << 0)
#define EVENT_VOLUME	(1 << 1)
#define EVENT_PLAY		(1 << 2)


#ifndef max
#define max(a,b) ((a) > (b) ? (a) : (b))
#endif



#define POS_CMD 3
#define POS_ACK	4
#define POS_BYTE_HIGH	5
#define POS_BYTE_LOW	6
#define POS_CS1			7
#define POS_CS2			8

#define CMD_RESET		0xc
#define CMD_VOLUME		0x6
#define CMD_PLAY		0x3
#define CMD_PLAY_NEXT	0x1
//
#define REPLY_INIT		0x3f

#define CMD_ACK			0x1
#define CMD_NOACK		0x0


#define SETTING_VOL		"dfplayer.volume"
#define SETTING_BELL	"dfplayer.bell_track"


static const unsigned int CMD_LEN = 10;
static const char cmd_reset[] = {
	0x7E, 0xFF, 0x6,
	CMD_RESET, CMD_ACK, 0x0, 0x0,
	0xFE, 0xEE, 0xEF
};
static const char cmd_volume[] = { // volume 10
	0x7E, 0xFF, 0x6,
	CMD_VOLUME, CMD_ACK, 0x0, 0xA,
	0xFE, 0xEA, 0xEF
};
static const char cmd_play[] = { // play first track, actually
	0x7E, 0xFF, 0x6,
	CMD_PLAY, CMD_ACK, 0x0, 0x1,
	0xFE, 0xF6, 0xEF
};
static const char cmd_play_next[] = {
	0x7E, 0xFF, 0x6,
	CMD_PLAY_NEXT, CMD_ACK, 0x0, 0x0,
	0xFE, 0xF9, 0xEF
};


static unsigned int checksum(char msg[], int len)
{
	unsigned int sum = 0;

	for (int i=1; i<7; ++i)
		sum += msg[i];

	sum = -sum;

//	unsigned char cs1 = (sum >> 8) & 0xff;
//	unsigned char cs2 = sum & 0xff;
//	printf("%x -> %x %x\n", sum, cs1, cs2);
//	printf("     -> %x %x\n\n", msg[7], msg[8]);

	return sum;
}


static void parse_reply(const char* prefix, const uint8_t data[])
{
	printf("%s\n", prefix);
#if 0
//	printf("--rcv: %d bytes [%c]\n", len, data[2]);
	printf("%02x . ", data[0]);
	printf("%02x . ", data[1]);
	printf("%02x . ", data[2]);
	printf("%02x . ", data[3]);
	printf("%02x . ", data[4]);
	printf("%02x ", data[5]);
	printf("%02x . ", data[6]);
	printf("%02x ", data[7]);
	printf("%02x . ", data[8]);
	printf("%02x", data[9]);
	printf("\n");
#endif

	switch (data[POS_CMD])
	{
	case REPLY_INIT:
		if (data[POS_BYTE_LOW] == 2)
			printf("--> FLASH\n");
//			ESP_LOGE(MY_TAG, "flash
		break;
	}
}


static void prepare_cmd(const char* cmd, char dest[], int param)
{
	for (int i=0; i<CMD_LEN; ++i)
		dest[i] = cmd[i];

	dest[POS_BYTE_LOW] = param & 0xff;
	dest[POS_BYTE_HIGH] = (param >> 8) & 0xff;

	unsigned int sum = checksum(dest, CMD_LEN);
	unsigned char cs1 = (sum >> 8) & 0xff;
	unsigned char cs2 = sum & 0xff;

//	printf(". %02x %02x\n", dest[POS_CS1], dest[POS_CS2]);
//	printf(". %02x %02x\n", cs1, cs2);

	dest[POS_CS1] = cs1;
	dest[POS_CS2] = cs2;

#if 0
	for (int i=0; i<CMD_LEN; ++i)
		printf(".%02x.", dest[i]);
	printf("\n");
#endif
}


static bool has_flash(const uint8_t data[])
{
	return data[POS_CMD] == REPLY_INIT && data[POS_BYTE_LOW] == 2;
}


static void dfplayer_task(void* arg)
{
	const int uart_rx = GPIO_NUM_13; // works: GPIO_NUM_19;
	const int uart_tx = GPIO_NUM_12; // works: GPIO_NUM_21;
	const int BUF_SIZE = max(CMD_LEN * 2, 256);

	uart_config_t uart_config = {
		.baud_rate = 9600,
		.data_bits = UART_DATA_8_BITS,
		.parity = UART_PARITY_DISABLE,
		.stop_bits = UART_STOP_BITS_1,
		.flow_ctrl = UART_HW_FLOWCTRL_DISABLE
//		.flow_ctrl = UART_HW_FLOWCTRL_CTS_RTS,
//		.rx_flow_ctrl_thresh = 122,
	};
	ESP_ERROR_CHECK(uart_param_config(uart_num, &uart_config));

	// Set UART pins(TX: IO16 (UART2 default), RX: IO17 (UART2 default), RTS: IO18, CTS: IO19)
	ESP_ERROR_CHECK(uart_set_pin(uart_num,
				uart_tx, uart_rx, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE))

	// Install UART driver using an event queue here
	ESP_ERROR_CHECK(uart_driver_install(uart_num, BUF_SIZE * 2, 0, 0, NULL, 0));

	uint8_t* data = (uint8_t*)malloc(BUF_SIZE);
	char to_send[CMD_LEN];

	bool need_reset = true;
	int track_num = 1;

	for(;;)
	{
		if (need_reset)
		{
			printf("--sending RESET\n");
			prepare_cmd(cmd_reset, to_send, 0);
//			uart_write_bytes(uart_num, to_send, CMD_LEN);
			uart_write_bytes(uart_num, cmd_reset, CMD_LEN);

			// wait for reply
			for (;;)
			{
				printf("--waiting for reply\n");
				int len = uart_read_bytes(uart_num, data, CMD_LEN,
						4000 / portTICK_PERIOD_MS);
				printf("len %d\n", len);
				if (!len)
					break;

				parse_reply("RESET", data);
				if (has_flash(data))
				{
					need_reset = false;

					// set volume
//					dfplayer_set_volume_p(25);
					uart_write_bytes(uart_num, cmd_volume, CMD_LEN);
				}
			}

			vTaskDelay(1000 / portTICK_PERIOD_MS);
			continue;
		}


		EventBits_t bits = xEventGroupGetBits(x_events);
		if (bits & EVENT_BELL)
		{
			xEventGroupClearBits(x_events, EVENT_BELL);

			ESP_LOGI(MY_TAG, "play bell (%d)", dfplayer_bell_track);

			track_num = dfplayer_bell_track;
			prepare_cmd(cmd_play, to_send, track_num);
			if (++track_num > 2)
				track_num = 1;
			uart_write_bytes(uart_num, to_send, CMD_LEN);

			int len = uart_read_bytes(uart_num, data, CMD_LEN,
					100 / portTICK_PERIOD_MS);
			if (len)
			{
				parse_reply("PLAY1ST", data);
			}
		}
		if (bits & EVENT_VOLUME)
		{
			xEventGroupClearBits(x_events, EVENT_VOLUME);

			int raw_vol = (float)dfplayer_vol / (float)100 * 30;
			ESP_LOGI(MY_TAG, "volume: %d (raw: %d)", dfplayer_vol, raw_vol);

			prepare_cmd(cmd_volume, to_send, raw_vol);
			uart_write_bytes(uart_num, to_send, CMD_LEN);
		}
		if (bits & EVENT_PLAY)
		{
			xEventGroupClearBits(x_events, EVENT_PLAY);

			ESP_LOGI(MY_TAG, "play track (%d)", dfplayer_play_track);

			track_num = dfplayer_play_track;
			prepare_cmd(cmd_play, to_send, track_num);
			if (++track_num > 2)
				track_num = 1;
			uart_write_bytes(uart_num, to_send, CMD_LEN);

			int len = uart_read_bytes(uart_num, data, CMD_LEN,
					100 / portTICK_PERIOD_MS);
			if (len)
			{
				parse_reply("PLAY1ST", data);
			}
		}

        vTaskDelay(1000 / portTICK_PERIOD_MS);
#if 0
		// reply
		int len = uart_read_bytes(uart_num, data, BUF_SIZE,
				20 / portTICK_PERIOD_MS);
		if (len)
		{
			printf("--rcv: %d bytes [%c]\n", len, data[2]);
			for (int i=0; i<len; ++i)
				printf(" %02x", data[i]);
			printf("\n");
		}
#endif
	}
}


void dfplayer_bell()
{
	xEventGroupSetBits(x_events, EVENT_BELL);
}


void dfplayer_init()
{
	x_events = xEventGroupCreate();
	if (x_events == NULL)
		ESP_LOGE(MY_TAG, "Could create event group.");

	settings_get(SETTING_VOL, &dfplayer_vol);
	settings_get(SETTING_BELL, &dfplayer_bell_track);

	ESP_LOGI(MY_TAG, "volume %d", dfplayer_vol);
	ESP_LOGI(MY_TAG, "bell track %d", dfplayer_bell_track);

	xTaskCreate(dfplayer_task, "dfplayer_task", 4096, NULL, 5, NULL);
}


void dfplayer_set_volume_p(int vol)
{
	if (vol == dfplayer_vol)
		return;

	dfplayer_vol = vol;
	settings_set(SETTING_VOL, dfplayer_vol);

	xEventGroupSetBits(x_events, EVENT_VOLUME);
}


int dfplayer_get_volume_p()
{
	return dfplayer_vol;
}


int dfplayer_get_track()
{
	return dfplayer_bell_track;
}


void dfplayer_set_track(int track)
{
	if (track == dfplayer_bell_track)
		return;

	dfplayer_bell_track = track;
	settings_set(SETTING_BELL, dfplayer_bell_track);
}


void dfplayer_play(int track)
{
	dfplayer_play_track = track;
	xEventGroupSetBits(x_events, EVENT_PLAY);
}
