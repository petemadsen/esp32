/**
 * This code is public domain.
 */
#include <lwip/apps/sntp.h>

#include <esp_spiffs.h>
#include <esp_log.h>

#include "common.h"
#include "my_http.h"


static const char* MY_TAG = PROJECT_TAG("http");



static esp_err_t status_handler(httpd_req_t* req);
static esp_err_t settings_get_handler(httpd_req_t* req);
static esp_err_t settings_set_handler(httpd_req_t* req);
static esp_err_t log_handler(httpd_req_t* req);


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
		.uri	= "/sget",
		.method	= HTTP_GET,
		.handler= settings_get_handler,
	},
	{
		.uri	= "/sset",
		.method	= HTTP_GET,
		.handler= settings_set_handler,
	},
	{
		.uri	= "/log",
		.method	= HTTP_GET,
		.handler= log_handler,
	}
};


static bool save_to_file(const char* filename, const char* buf, size_t buf_len);


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
						  " ident %s\n",
						  PROJECT_VERSION,
						  PROJECT_NAME);
	httpd_resp_send(req, buf, buflen);

	free(buf);
	return ESP_OK;
}


esp_err_t settings_get_handler(httpd_req_t* req)
{
	esp_err_t ret = ESP_FAIL;
	char* reply = NULL;
	int reply_len = 0;

#if 0
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
#endif

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

#if 0
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
#endif

	if (ret != ESP_OK)
		httpd_resp_send_404(req);
	else
		httpd_resp_send(req, RET_OK, strlen(RET_OK));

	return ret;
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


esp_err_t log_handler(httpd_req_t* req)
{
#if 0
	const char* line = malloc(LOG_MAX_LINELEN);
	for (uint8_t i = 0; i < LOG_MAX_ENTRIES; ++i)
	{
		if (mylog_get(i, &line))
			httpd_resp_send_chunk(req, line, strlen(line));
	}
	free(line);
#else
	httpd_resp_send_chunk(req, RET_OK, strlen(RET_OK));
#endif

	return ESP_OK;
}
