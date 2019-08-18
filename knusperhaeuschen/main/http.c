#include "http.h"
#include "parser.h"
#include "light.h"


static esp_err_t status_handler(httpd_req_t* req);
static esp_err_t light_handler(httpd_req_t* req);
static esp_err_t light_on_handler(httpd_req_t* req);
static esp_err_t light_off_handler(httpd_req_t* req);


httpd_uri_t status_desc = {
	.uri	= "/status",
	.method	= HTTP_GET,
	.handler	= status_handler,
};
httpd_uri_t light_desc = {
	.uri	= "/light",
	.method	= HTTP_GET,
	.handler	= light_handler,
};
httpd_uri_t light_on_desc = {
	.uri	= "/light/on",
	.method	= HTTP_GET,
	.handler	= light_on_handler,
};
httpd_uri_t light_off_desc = {
	.uri	= "/light/off",
	.method	= HTTP_GET,
	.handler	= light_off_handler,
};


httpd_handle_t http_start()
{
	httpd_handle_t server = NULL;
	httpd_config_t config = HTTPD_DEFAULT_CONFIG();

	if (httpd_start(&server, &config) == ESP_OK)
	{
		httpd_register_uri_handler(server, &status_desc);
		httpd_register_uri_handler(server, &light_desc);
		httpd_register_uri_handler(server, &light_on_desc);
		httpd_register_uri_handler(server, &light_off_desc);
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
	char* p = parse_input("light", 6);
	httpd_resp_send(req, p, strlen(p));
	return ESP_OK;
}


esp_err_t light_on_handler(httpd_req_t* req)
{
	char* p = parse_input("light 1", 8);
	httpd_resp_send(req, p, strlen(p));
	return ESP_OK;
}


esp_err_t light_off_handler(httpd_req_t* req)
{
	char* p = parse_input("light 0", 8);
	httpd_resp_send(req, p, strlen(p));
	return ESP_OK;
}
