/**
 * dfplayer.c
 */
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "dfplayer.h"

#include <stdio.h>
#include <string.h>
#include "esp_err.h"
#include "esp_log.h"
#include "driver/uart.h"
#include "soc/rmt_reg.h"

#include "sdkconfig.h"

static const char* MY_TAG = "DFPLAYER";


// NORFLASH => 7e ff 06 09 00 00 04 ff dd ef
// data length 6 bytes (ff 06 09 00 00 04)
// 7e start byte $S
// ff version
// 06 length of bytes after length byte
// 09 cmd
// 00 no feedback
// ...
// ef end bit

static void dfplayer_task(void* arg)
{
	const int uart_num = UART_NUM_2;
	const int uart_rx = GPIO_NUM_19;// GPIO_NUM_4;
	const int uart_tx = GPIO_NUM_21; // GPIO_NUM_5;
	const int BUF_SIZE = 1024;

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
	data[0] = 0x19;
	data[1] = 0x80;
	data[2] = 'A';

	for(;;)
	{
        vTaskDelay(1000 / portTICK_PERIOD_MS);

		uart_write_bytes(uart_num, (const char*)data, 3);
		printf("--snd %c\n", data[2]);

		int len = uart_read_bytes(uart_num, data, BUF_SIZE, 20 / portTICK_PERIOD_MS);
		if (len)
			printf("--rcv: %d bytes [%c]\n", len, data[2]);
//		uart_write_bytes(uart_num, (const char*)data, len);

		data[2] = data[2] + 1;
		if (data[2] == 'Z')
			data[2] = 'A';
	}
}


void dfplayer_init()
{
	xTaskCreate(dfplayer_task, "dfplayer_task", 2048, NULL, 10, NULL);
}
