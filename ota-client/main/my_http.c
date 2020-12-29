/**
 * This code is public domain.
 */
#include <lwip/apps/sntp.h>

#include <esp_log.h>

#include "my_http.h"
#include "common.h"

#include "system/ota.h"
#include "system/wifi.h"
#include "system/my_settings.h"


static void reboot_task(void* arg);


static esp_err_t status_handler(httpd_req_t* req);
static esp_err_t ota_handler(httpd_req_t* req);
static esp_err_t set_handler(httpd_req_t* req);


static const char* RET_OK = "OK";
static const char* RET_ERR = "ERR";


static const char* MY_TAG = PROJECT_TAG("http");


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
	},
	{
		.uri	= "/set",
		.method	= HTTP_GET,
		.handler= set_handler,
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

	char* download_url = NULL;
	ota_download_url(&download_url);

	int buflen = snprintf(buf, bufsize,
							"ident %s\n"
							" sha256 %s\n"
							//
							" free-ram %u\n"
							" wifi %s\n"
							" wifi-reconnects %u\n"
							" ota %d\n"
							" boots %d\n"
							" uptime %lld\n"
							//
							" download-url %s\n",
							ota_project_name(),
							ota_sha256(),
							//
							esp_get_free_heap_size(),
							wifi_ssid(),
							wifi_reconnects(),
							ota_has_update_partition(),
							settings_boot_counter(),
							uptime,
							//
							download_url);
	httpd_resp_send(req, buf, buflen);

	free(buf);
	free(download_url);
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


esp_err_t set_handler(httpd_req_t* req)
{
	const char* ret = RET_ERR;

	size_t buf_len = httpd_req_get_url_query_len(req) + 1;
	if (buf_len > 1)
	{
		char* buf = malloc(buf_len);
		if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK)
		{
			ESP_LOGI(MY_TAG, "New download URL: %s", buf);
			ota_set_download_url(buf);
			ret = RET_OK;
		}
		free(buf);
	}

	httpd_resp_send(req, ret, strlen(ret));
	return ESP_OK;
}
