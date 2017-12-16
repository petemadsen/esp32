#include <esp_log.h>

#include <lwip/sys.h>
#include <lwip/netdb.h>
#include <lwip/api.h>

#include <string.h>

#include "config.h"
#include "sdkconfig.h"


static const char* MY_TAG = "[telnet]";


static bool parse_input(struct netconn* conn, char* data, u16_t len)
{
	char* line = strtok(data, "\n");
	if (strstr(line, "exit")==line || strstr(line, "quit")==line)
		return false;

	if (strstr(line, "status")==line)
		netconn_write(conn, "OK\n", 3, NETCONN_NOCOPY);
	else if (strstr(line, "help")==line)
		netconn_write(conn, "V1\n", 3, NETCONN_NOCOPY);
	else
		netconn_write(conn, "ERR\n", 4, NETCONN_NOCOPY);

	return true;
}


static void telnet_serve(struct netconn* conn)
{
	err_t err;
	struct netbuf* buf;
	bool do_run = true;

	ESP_LOGI(MY_TAG, "new connection");
	while (do_run)
	{
		err = netconn_recv(conn, &buf);
		if (err != ERR_OK)
			break;

		char *data;
		u16_t len;
		netbuf_data(buf, (void**)&data, &len);
		ESP_LOGI(MY_TAG, "received %d bytes", len);

		do_run = parse_input(conn, data, len);

		netbuf_delete(buf);
	}
	ESP_LOGI(MY_TAG, "closing connection");

	netconn_close(conn);
}


void telnet_server(void* pvParameters)
{
	struct netconn* conn;
	struct netconn* newconn;
	err_t err;

	conn = netconn_new(NETCONN_TCP);
	netconn_bind(conn, NULL, 80);
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

