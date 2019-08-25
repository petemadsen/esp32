/**
 * This code is public domain.
 */
#include "http.h"
#include "light.h"
#include "tone.h"
#include "ota.h"

#include <lwip/apps/sntp.h>

#define VERSION "0.0.8"

extern uint32_t g_boot_count;


static void reboot_task(void* arg);


static esp_err_t status_handler(httpd_req_t* req);
static esp_err_t light_handler(httpd_req_t* req);
static esp_err_t bell_handler(httpd_req_t* req);
static esp_err_t play_handler(httpd_req_t* req);
static esp_err_t volume_handler(httpd_req_t* req);
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
		.uri	= "/light",
		.method	= HTTP_GET,
		.handler= light_handler,
	},
	{
		.uri	= "/bell",
		.method	= HTTP_GET,
		.handler= bell_handler,
	},
	{
		.uri	= "/play",
		.method	= HTTP_GET,
		.handler= play_handler,
	},
	{
		.uri	= "/volume",
		.method	= HTTP_GET,
		.handler= volume_handler,
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

	int64_t uptime = xTaskGetTickCount() / 1000 / 1000;

	const size_t bufsize = 320;
	char* buf = malloc(bufsize);
	int buflen = snprintf(buf, bufsize,
						  "version %s light %d free-ram %u boots %u uptime %lld time %02d:%02d",
						  VERSION,
						  light_status(),
						  esp_get_free_heap_size(),
						  g_boot_count,
						  uptime,
						  timeinfo.tm_hour, timeinfo.tm_min);
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


esp_err_t light_handler(httpd_req_t* req)
{
	const char* ret = RET_ERR;

	int onoff;
	if (get_int(req, &onoff) && (onoff==0 || onoff==1))
	{
		if (onoff == 1)
			light_on();
		else
			light_off();
		ret = RET_OK;
	}
	else
	{
		char* buf = malloc(20);
		int buf_len = sprintf(buf, "%d", light_status());
		httpd_resp_send(req, buf, buf_len);
		free(buf);
		return ESP_OK;
	}

	httpd_resp_send(req, ret, strlen(ret));
	return ESP_OK;
}


esp_err_t bell_handler(httpd_req_t* req)
{
	const char* ret = RET_ERR;

#if 0
	int track;
	if (get_int(req, &track) && track >= 1)
	{
		dfplayer_set_track(track);
		ret = RET_OK;
	}
	else
	{
		char* buf = malloc(20);
		int buf_len = sprintf(buf, "%d", dfplayer_get_track());
		httpd_resp_send(req, buf, buf_len);
		free(buf);
		return ESP_OK;
	}
#endif

	httpd_resp_send(req, ret, strlen(ret));
	return ESP_OK;
}


esp_err_t play_handler(httpd_req_t* req)
{
	const char* ret = RET_ERR;

	tone_bell();
	ret = RET_OK;

#if 0
	int track;
	if (get_int(req, &track) && track >= 1 && track <= 9)
	{
		dfplayer_play(track);
		ret = RET_OK;
	}
#endif

	httpd_resp_send(req, ret, strlen(ret));
	return ESP_OK;
}


esp_err_t volume_handler(httpd_req_t* req)
{
	const char* ret = RET_ERR;

#if 0
	int volume;
	if (get_int(req, &volume) && volume >= 0 && volume <= 100)
	{
		dfplayer_set_volume_p(volume);
		ret = RET_OK;
	}
	else
	{
		char* buf = malloc(20);
		int buf_len = sprintf(buf, "%d", dfplayer_get_volume_p());
		httpd_resp_send(req, buf, buf_len);
		free(buf);
		return ESP_OK;
	}
#endif

	httpd_resp_send(req, ret, strlen(ret));
	return ESP_OK;
}


static void reboot_task(void* arg)
{
	vTaskDelay(3000 / portTICK_PERIOD_MS);
	esp_restart();
}
