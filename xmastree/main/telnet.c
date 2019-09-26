#include "sdkconfig.h"

#include <esp_log.h>

#include <lwip/sys.h>
#include <lwip/netdb.h>
#include <lwip/api.h>

#include <string.h>
#include <stdint.h>

#include "config.h"


static char* (*parse_input)(char* data, int data_len);

#define RECV_TIMEOUT_SECS 60


static const char* MY_TAG = "xmastree/telnet";


static void telnet_serve(struct netconn* conn)
{
	err_t err;
	struct netbuf* buf;

	char* data;
	u16_t len;

	char* output = (char*)MY_TAG; // assignment required for first loop run

	// in msec
	if (RECV_TIMEOUT_SECS > 0)
		netconn_set_recvtimeout(conn, RECV_TIMEOUT_SECS * 1000);

	ESP_LOGI(MY_TAG, "New connection");
	while (output)
	{
		err = netconn_recv(conn, &buf);
		if (err != ERR_OK)
			break;

		netbuf_data(buf, (void**)&data, &len);
		ESP_LOGD(MY_TAG, "received %d bytes", len);

		ESP_LOGI(MY_TAG, "telnet[%d]: %s",  len, data);
		output = parse_input(data, len);
		if (output)
		{
			ESP_LOGI(MY_TAG, "telnet: %s", output);
			netconn_write(conn, output, strlen(output), NETCONN_NOCOPY);
		}

		netbuf_delete(buf);
	}
	ESP_LOGI(MY_TAG, "Closing connection");

	netconn_close(conn);
}


void telnet_task(void* pvParameters)
{
	struct netconn* conn;
	struct netconn* newconn;
	err_t err;

	parse_input = pvParameters;
	if (!parse_input)
	{
		ESP_LOGE(MY_TAG, "No parse_input function provided.");
		vTaskDelete(NULL);
	}

	conn = netconn_new(NETCONN_TCP);
	netconn_bind(conn, NULL, 23);
	netconn_listen(conn);

	do
	{
		err = netconn_accept(conn, &newconn);
		if (err == ERR_OK)
		{
			telnet_serve(newconn);
			netconn_delete(newconn);
		}
	}
    while (err == ERR_OK);

	netconn_close(conn);
	netconn_delete(conn);
}

