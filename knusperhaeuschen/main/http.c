#include "http.h"
#include "parser.h"


static esp_err_t http_status_handler(httpd_req_t* req);


httpd_uri_t status_handler = {
	.uri	= "/status",
	.method	= HTTP_GET,
	.handler	= http_status_handler,
};


httpd_handle_t http_start()
{
	httpd_handle_t server = NULL;
	httpd_config_t config = HTTPD_DEFAULT_CONFIG();

	if (httpd_start(&server, &config) == ESP_OK)
	{
		httpd_register_uri_handler(server, &status_handler);
		return server;
	}

	return NULL;
}


void http_stop(httpd_handle_t server)
{
	httpd_stop(server);
}


esp_err_t http_status_handler(httpd_req_t* req)
{
	char* p = parse_input("status", 6);
//	httpd_resp_send(req, "OK", 2);
	httpd_resp_send(req, p, strlen(p));
	return ESP_OK;
}
