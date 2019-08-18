#include "http.h"
#include "parser.h"
#include "light.h"
#include "dfplayer.h"


static esp_err_t status_handler(httpd_req_t* req);
static esp_err_t light_handler(httpd_req_t* req);
static esp_err_t bell_handler(httpd_req_t* req);
static esp_err_t volume_handler(httpd_req_t* req);


static const char* RET_OK = "OK";
static const char* RET_ERR = "ERR";


static httpd_uri_t basic_handlers[] = {
	{
		.uri	= "/status",
		.method	= HTTP_GET,
		.handler	= status_handler,
	},
	{
		.uri	= "/light",
		.method	= HTTP_GET,
		.handler	= light_handler,
	},
	{
		.uri	= "/bell",
		.method	= HTTP_GET,
		.handler	= bell_handler,
	},
	{
		.uri	= "/volume",
		.method	= HTTP_GET,
		.handler	= volume_handler,
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


esp_err_t status_handler(httpd_req_t* req)
{
	char* p = parse_input("status", 6);
//	httpd_resp_send(req, "OK", 2);
	httpd_resp_send(req, p, strlen(p));
	return ESP_OK;
}


esp_err_t light_handler(httpd_req_t* req)
{
	const char* ret = RET_ERR;

    size_t buf_len = httpd_req_get_url_query_len(req) + 1;
    if (buf_len > 1)
	{
        char* buf = malloc(buf_len);
        if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK)
		{
			int onoff;
			if (sscanf(buf, "%d", &onoff)==1 && (onoff==0 || onoff==1))
			{
				if (onoff == 1)
					light_on();
				else
					light_off();
				ret = RET_OK;
			}
		}
		free(buf);
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

    size_t buf_len = httpd_req_get_url_query_len(req) + 1;
    if (buf_len > 1)
	{
        char* buf = malloc(buf_len);
        if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK)
		{
			int track;
			if (sscanf(buf, "%d", &track)==1 && track >= 1)
			{
				dfplayer_set_track(track);
				ret = RET_OK;
			}
		}
		free(buf);
	}
	else
	{
		char* buf = malloc(20);
		int buf_len = sprintf(buf, "%d", dfplayer_get_track());
		httpd_resp_send(req, buf, buf_len);
		free(buf);
		return ESP_OK;
	}

	httpd_resp_send(req, ret, strlen(ret));
	return ESP_OK;
}


esp_err_t volume_handler(httpd_req_t* req)
{
	const char* ret = RET_ERR;

    size_t buf_len = httpd_req_get_url_query_len(req) + 1;
    if (buf_len > 1)
	{
        char* buf = malloc(buf_len);
        if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK)
		{
			int volume;
			if (sscanf(buf, "%d", &volume)==1 && volume >= 0 && volume <= 100)
			{
				dfplayer_set_volume_p(volume);
				ret = RET_OK;
			}
		}
		free(buf);
	}
	else
	{
		char* buf = malloc(20);
		int buf_len = sprintf(buf, "%d", dfplayer_get_volume_p());
		httpd_resp_send(req, buf, buf_len);
		free(buf);
		return ESP_OK;
	}

	httpd_resp_send(req, ret, strlen(ret));
	return ESP_OK;
}

