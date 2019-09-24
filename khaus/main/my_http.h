/**
 * This code is public domain.
 */
#ifndef PROJECT_HTTP_H
#define PROJECT_HTTP_H


#include <esp_http_server.h>


httpd_handle_t http_start();


void http_stop(httpd_handle_t);


#endif
