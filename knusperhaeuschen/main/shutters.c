/**
 * http_request example.
 */
#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>
#include <esp_system.h>
#include <esp_wifi.h>
#include <esp_event_loop.h>
#include <esp_log.h>
#include <nvs_flash.h>

#include <lwip/err.h>
#include <lwip/sockets.h>
#include <lwip/sys.h>
#include <lwip/netdb.h>
#include <lwip/dns.h>


static const char* MY_TAG = "knusperhaeuschen/shutters";

//#define WEB_URL		"http://example.com"
//#define WEB_SERVER	"example.com"
//#define WEB_PORT	"80"
#define WEB_URL		"http://192.168.1.86"
#define WEB_SERVER	"192.168.1.86"
#define WEB_PORT	"8080"
static const char* REQUEST = "GET / HTTP/1.0\r\n"
	"Host: " WEB_SERVER "\r\n"
	"User-Agent: esp-idf/1.0 esp32\r\n"
	"\r\n";


void shutters_task(void* pvParameters)
{
    const struct addrinfo hints = {
        .ai_family = AF_INET,
        .ai_socktype = SOCK_STREAM,
    };
    struct addrinfo* res;
    struct in_addr *addr;
    int s, r;
    char recv_buf[64];

	for (;;)
	{
		ESP_LOGI(MY_TAG, "run");

#if 1
		// connect
		int err = getaddrinfo(WEB_SERVER, WEB_PORT, &hints, &res);
        if (err != 0 || res == NULL)
		{
            ESP_LOGE(MY_TAG, "DNS lookup failed err=%d res=%p", err, res);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            continue;
        }

        /* Code to print the resolved IP.

           Note: inet_ntoa is non-reentrant, look at ipaddr_ntoa_r for "real"
		   code
		*/
        addr = &((struct sockaddr_in *)res->ai_addr)->sin_addr;
        ESP_LOGI(MY_TAG, "DNS lookup succeeded. IP=%s", inet_ntoa(*addr));
#else
		res
#endif

        s = socket(res->ai_family, res->ai_socktype, 0);
        if (s < 0)
		{
            ESP_LOGE(MY_TAG, "... Failed to allocate socket.");
            freeaddrinfo(res);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            continue;
        }
        ESP_LOGI(MY_TAG, "... allocated socket");

        if(connect(s, res->ai_addr, res->ai_addrlen) != 0) {
            ESP_LOGE(MY_TAG, "... socket connect failed errno=%d", errno);
            close(s);
            freeaddrinfo(res);
            vTaskDelay(4000 / portTICK_PERIOD_MS);
            continue;
        }

        ESP_LOGI(MY_TAG, "... connected");
        freeaddrinfo(res);

        if (write(s, REQUEST, strlen(REQUEST)) < 0) {
            ESP_LOGE(MY_TAG, "... socket send failed");
            close(s);
            vTaskDelay(4000 / portTICK_PERIOD_MS);
            continue;
        }
        ESP_LOGI(MY_TAG, "... socket send success");

        struct timeval receiving_timeout;
        receiving_timeout.tv_sec = 5;
        receiving_timeout.tv_usec = 0;
        if (setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &receiving_timeout,
                sizeof(receiving_timeout)) < 0) {
            ESP_LOGE(MY_TAG, "... failed to set socket receiving timeout");
            close(s);
            vTaskDelay(4000 / portTICK_PERIOD_MS);
            continue;
        }
        ESP_LOGI(MY_TAG, "... set socket receiving timeout success");

        /* Read HTTP response */
        do {
            bzero(recv_buf, sizeof(recv_buf));
            r = read(s, recv_buf, sizeof(recv_buf)-1);
            for(int i = 0; i < r; i++) {
                putchar(recv_buf[i]);
            }
        } while(r > 0);

        ESP_LOGI(MY_TAG, "... done reading from socket. Last read return=%d errno=%d\r\n", r, errno);
        close(s);

		// sleep
		for (int i=0; i<10; ++i)
			vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
