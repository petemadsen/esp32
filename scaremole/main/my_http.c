/**
 * This code is public domain.
 */
#include <lwip/apps/sntp.h>

#include "common.h"

#include "system/my_settings.h"

#include "my_http.h"
#include "ota.h"
#include "voltage.h"


static void reboot_task(void* arg);


static esp_err_t status_handler(httpd_req_t* req);
static esp_err_t ota_handler(httpd_req_t* req);


static const char* RET_OK = "OK";
static const char* RET_ERR = "ERR";


static httpd_uri_t basic_handlers[] = {
	{
		.uri	= "/status",
		.method	= HTTP_GET,
		.handler= status_handler,
	},
	{
		.uri	= "/ota",
		.method	= HTTP_GET,
		.handler= ota_handler,
	}
};


httpd_handle_t http_start()
{
	httpd_handle_t server = NULL;
	httpd_config_t config = HTTPD_DEFAULT_CONFIG();

	if (httpd_start(&server, &config) == ESP_OK)
	{
		int num = sizeof(basic_handlers) / sizeof(httpd_uri_t);
		for (int i=0; i<num; ++i)
			httpd_register_uri_handler(server, &basic_handlers[i]);
		return server;
	}

	return NULL;
}


void http_stop(httpd_handle_t server)
{
	httpd_stop(server);
}


static bool get_int(httpd_req_t* req, int* val)
{
	bool ret = false;

    size_t buf_len = httpd_req_get_url_query_len(req) + 1;
    if (buf_len > 1)
	{
        char* buf = malloc(buf_len);
        if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK)
		{
			ret = (sscanf(buf, "%d", val)==1);
		}
		free(buf);
	}

	return ret;
}


esp_err_t status_handler(httpd_req_t* req)
{
	time_t now;
	struct tm timeinfo;
	time(&now);
	localtime_r(&now, &timeinfo);

	int64_t uptime = esp_timer_get_time() / 1000 / 1000;

	const size_t bufsize = 320;
	char* buf = malloc(bufsize);
	int buflen = snprintf(buf, bufsize,
						  "version %s\n"
						  " ident %s\n"
						  " free-ram %u\n"
						  " wifi-reconnects %u\n"
						  " voltage %.2f\n"
						  " uptime %lld\n",
						  PROJECT_VERSION,
						  PROJECT_NAME,
						  esp_get_free_heap_size(),
						  settings_boot_counter(),
						  voltage_get(),
						  uptime);
	httpd_resp_send(req, buf, buflen);

	free(buf);
	return ESP_OK;
}


esp_err_t ota_handler(httpd_req_t* req)
{
	const char* err = ota_reboot();
	if (!err)
	{
		httpd_resp_send(req, RET_OK, strlen(RET_OK));
		xTaskCreate(reboot_task, "reboot_task", 2048, NULL, 5, NULL);
		return ESP_OK;
	}

	httpd_resp_send(req, err, strlen(err));
	return ESP_OK;
}


static void reboot_task(void* arg)
{
	vTaskDelay(3000 / portTICK_PERIOD_MS);
	esp_restart();
}
