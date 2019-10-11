/**
 * This code is public domain.
 */
#include <lwip/apps/sntp.h>

#include <esp_spiffs.h>
#include <esp_log.h>

#include "common.h"
#include "my_http.h"
#include "tone.h"
#include "my_sensors.h"
#include "my_lights.h"
#include "sound/read_wav.h"
#include "system/my_settings.h"
#include "system/ota.h"


static const char* MY_TAG = PROJECT_TAG("http");



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
static const char* RET_ERR_SIZE = "ERR_SIZE";


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


static bool save_to_file(const char* filename, const char* buf, size_t buf_len);


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
						  " boots %d\n"
						  " uptime %lld\n"
						  " time %02d:%02d\n"
						  " board_temp %.2f\n"
						  " board_voltage %.2f",
						  PROJECT_VERSION,
						  PROJECT_NAME,
						  lamp_status(),
						  lamp_on_secs(),
						  esp_get_free_heap_size(),
						  settings_boot_counter(),
						  uptime,
						  timeinfo.tm_hour, timeinfo.tm_min,
						  my_sensors_board_temp(),
						  my_sensors_board_voltage());
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
			lamp_on();
		else
			lamp_off();
		ret = RET_OK;
	}
	else
	{
		char* buf = malloc(20);
		int buf_len = sprintf(buf, "%d", lamp_status());
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
	int ret = 0;
	size_t len = req->content_len;
	ESP_LOGE(MY_TAG, "bell-upload-handler: %d", (int)len);

	int name = -1;
	if (!get_int(req, &name))
	{
		httpd_resp_send(req, RET_ERR, strlen(RET_ERR));
		return ESP_OK;
	}
	ESP_LOGE(MY_TAG, "Name: %d", name);

	// -- receive file into a string
	char* buf = malloc(len);
	if (!buf)
	{
		httpd_resp_send(req, RET_ERR_SIZE, strlen(RET_ERR_SIZE));
		return ESP_OK;
	}

	char* p = buf;
	while (len > 0)
	{
		int rcv = httpd_req_recv(req, p, len);
		if (rcv <= 0)
		{
			if (rcv == HTTPD_SOCK_ERR_TIMEOUT)
				continue;

			free(buf);
			httpd_resp_send(req, RET_ERR, strlen(RET_ERR));
			return ESP_FAIL;
		}

//		fwrite(buf, sizeof(char), ret, file);
		len -= rcv;
		p += rcv;
	}

	// -- check file
	ret = read_wav_check(buf, len);
	if (ret != 0)
	{
		char filename[40];
		sprintf(filename, "/spiffs/bell%d.wav", name);
		if (!save_to_file(filename, buf, len))
			ret = 100;
	}

	// -- reply
	ESP_LOGE(MY_TAG, "WAV file: %d", ret);

	int buf_len = sprintf(buf, "%d", ret);
	httpd_resp_send(req, buf, buf_len);

	free(buf);
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

	int volume;
	if (get_int(req, &volume) && volume >= 0 && volume <= 100)
	{
		tone_set_volume_p(volume);
		ret = RET_OK;
	}
	else
	{
		char* buf = malloc(20);
		int buf_len = sprintf(buf, "%d", tone_get_volume_p());
		httpd_resp_send(req, buf, buf_len);
		free(buf);
		return ESP_OK;
	}

	httpd_resp_send(req, ret, strlen(ret));
	return ESP_OK;
}


bool save_to_file(const char* filename, const char* buf, size_t buf_len)
{
	bool ret = false;

	// -- write file to disc
	esp_vfs_spiffs_conf_t conf = {
			.base_path = "/spiffs",
			.partition_label = NULL,
			.max_files = 2,
			.format_if_mount_failed = false
	};
	esp_err_t err = esp_vfs_spiffs_register(&conf);
	if (err != ESP_OK)
	{
		if (err == ESP_FAIL)
			ESP_LOGE(MY_TAG, "Failed to mount or format filesystem");
		else if (err == ESP_ERR_NOT_FOUND)
			ESP_LOGE(MY_TAG, "Failed to find SPIFFS partition");
		else
			ESP_LOGE(MY_TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));

		return false;
	}

	// -- info
	size_t total = 0, used = 0;
	ret = esp_spiffs_info(NULL, &total, &used);
	if (ret != ESP_OK)
		ESP_LOGE(MY_TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
	else
		ESP_LOGI(MY_TAG, "Partition size: total: %d, used: %d", total, used);

	// -- write
	FILE* file = fopen(filename, "w");
	if (file)
	{
		unsigned long len = fwrite(buf, sizeof(char), buf_len, file);
		if (len == buf_len)
			ret = true;
		fclose(file);
	}
	else
	{
		ESP_LOGE(MY_TAG, "Could not open file for writing.");
		ret = false;
	}

	esp_vfs_spiffs_unregister(NULL);
	return ret;
}
