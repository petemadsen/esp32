/**
 * This code is public domain.
 */
#include "my_http.h"
#include "light.h"
#include "tone.h"
#include "ota.h"
#include "bmp280.h"
#include "voltage.h"
#include "common.h"
#include "my_settings.h"

#include <lwip/apps/sntp.h>

#include "esp_spiffs.h"
#include "esp_log.h"


extern uint32_t g_boot_count;

static const char* MY_TAG = "khaus/http";




static esp_err_t status_handler(httpd_req_t* req);
static esp_err_t light_handler(httpd_req_t* req);
static esp_err_t bell_handler(httpd_req_t* req);
static esp_err_t bell_upload_handler(httpd_req_t* req);
static esp_err_t play_handler(httpd_req_t* req);
static esp_err_t volume_handler(httpd_req_t* req);
static esp_err_t ota_handler(httpd_req_t* req);
static esp_err_t settings_get_handler(httpd_req_t* req);
static esp_err_t settings_set_handler(httpd_req_t* req);


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
		.uri	= "/bell",
		.method	= HTTP_POST,
		.handler= bell_upload_handler,
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
	},
	{
		.uri	= "/sget",
		.method	= HTTP_GET,
		.handler= settings_get_handler,
	},
	{
		.uri	= "/sset",
		.method	= HTTP_GET,
		.handler= settings_set_handler,
	}
};


httpd_handle_t http_start()
{
	httpd_handle_t server = NULL;
	httpd_config_t config = HTTPD_DEFAULT_CONFIG();
	config.max_uri_handlers = 10;

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
						  " light %d\n"
						  " light_on_secs %lld\n"
						  " free-ram %u\n"
						  " boots %u\n"
						  " uptime %lld\n"
						  " time %02d:%02d\n"
						  " board_temp %.2f\n"
						  " board_voltage %.2f",
						  PROJECT_VERSION,
						  PROJECT_NAME,
						  light_status(),
						  light_on_secs(),
						  esp_get_free_heap_size(),
						  g_boot_count,
						  uptime,
						  timeinfo.tm_hour, timeinfo.tm_min,
						  bmp280_get_temp(),
						  voltage_get());
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
		return ESP_OK;
	}

	httpd_resp_send(req, err, strlen(err));
	return ESP_OK;
}


esp_err_t settings_get_handler(httpd_req_t* req)
{
	esp_err_t ret = ESP_FAIL;
	char* reply = NULL;
	int reply_len = 0;

	size_t buf_len = httpd_req_get_url_query_len(req) + 1;
	if (buf_len > 1)
	{
		char* key = malloc(buf_len);
		if (httpd_req_get_url_query_str(req, key, buf_len) == ESP_OK)
		{
			int32_t val;
			ret = settings_get_int32(key, &val, false);
			if (ret == ESP_OK)
			{
				reply_len = 20;
				reply = malloc(reply_len);
				reply_len = snprintf(reply, reply_len, "%d", val);
				ret = ESP_OK;
			}

			if (!reply)
			{
				reply = strdup(RET_ERR);
				ret = settings_get_str(key, &reply, false);
				reply_len = strlen(reply);
			}
		}
		free(key);
	}

	if (ret != ESP_OK)
		httpd_resp_send_404(req);
	else
		httpd_resp_send(req, reply, reply_len);

	if (reply)
		free(reply);

	return ESP_OK;
}


esp_err_t settings_set_handler(httpd_req_t* req)
{
	esp_err_t ret = ESP_FAIL;

	size_t buf_len = httpd_req_get_url_query_len(req) + 1;
	if (buf_len > 1)
	{
		char* buf = malloc(buf_len);
		if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK)
		{
			int32_t val;
			char* key = malloc(buf_len);
			char* value = malloc(buf_len);

			if (sscanf(buf, "%s=%d", key, &val) == 2)
			{
				ret = settings_set_int32(key, val, true);
			}
			else if (sscanf(buf, "%[^=]=%s", key, value) == 2)
			{
				ret = settings_set_str(key, value, true);
			}
			else if (strcmp(buf, "ERASE") == 0)
			{
				ret = settings_erase();
			}

			free(key);
			free(value);
		}
		free(buf);
	}

	if (ret != ESP_OK)
		httpd_resp_send_404(req);
	else
		httpd_resp_send(req, RET_OK, strlen(RET_OK));

	return ret;
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

	int num;
	if (get_int(req, &num) && num >= 0)
	{
		if (tone_set(num))
			ret = RET_OK;
	}
	else
	{
		char* buf = malloc(20);
		int buf_len = sprintf(buf, "%d", tone_get());
		httpd_resp_send(req, buf, buf_len);
		free(buf);
		return ESP_OK;
	}

	httpd_resp_send(req, ret, strlen(ret));
	return ESP_OK;
}


esp_err_t bell_upload_handler(httpd_req_t* req)
{
	size_t len = req->content_len;
	char buf[100];

	int name = -1;
	if (!get_int(req, &name))
	{
		httpd_resp_send(req, RET_ERR, strlen(RET_ERR));
		return ESP_OK;
	}
	ESP_LOGE(MY_TAG, "Name: %d", name);

	ESP_LOGE(MY_TAG, "bell-upload-handler: %d", (int)len);
	// -- open
	esp_vfs_spiffs_conf_t conf = {
			.base_path = "/spiffs",
			.partition_label = NULL,
			.max_files = 5,
			.format_if_mount_failed = false
	};
	esp_err_t ret = esp_vfs_spiffs_register(&conf);
	if (ret != ESP_OK)
	{
		if (ret == ESP_FAIL)
			ESP_LOGE(MY_TAG, "Failed to mount or format filesystem");
		else if (ret == ESP_ERR_NOT_FOUND)
			ESP_LOGE(MY_TAG, "Failed to find SPIFFS partition");
		else
			ESP_LOGE(MY_TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));

		httpd_resp_send(req, RET_ERR, strlen(RET_ERR));
		return ESP_OK;
	}

	// -- info
	size_t total = 0, used = 0;
	ret = esp_spiffs_info(NULL, &total, &used);
	if (ret != ESP_OK)
		ESP_LOGE(MY_TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
	else
		ESP_LOGI(MY_TAG, "Partition size: total: %d, used: %d", total, used);

	// -- open/receive
	char filename[40];
	sprintf(filename, "/spiffs/bell%d.wav", name);
	FILE* file = fopen(filename, "w");
	while (len > 0)
	{
		int ret = httpd_req_recv(req, buf, MIN(len, sizeof(buf)));
		if (ret <= 0)
		{
			if (ret == HTTPD_SOCK_ERR_TIMEOUT)
				continue;

			esp_vfs_spiffs_unregister(NULL);
			httpd_resp_send(req, RET_ERR, strlen(RET_ERR));
			return ESP_FAIL;
		}

		fwrite(buf, sizeof(char), ret, file);
		len -= ret;
	}
	fclose(file);

	esp_vfs_spiffs_unregister(NULL);

	httpd_resp_send(req, RET_OK, strlen(RET_OK));
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

