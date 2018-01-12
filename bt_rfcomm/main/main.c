// Copyright 2015-2017 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "sdkconfig.h"

#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include "nvs.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"


#include "bt_rfcomm.h"


#define MAXBUF 40
static uint8_t buffer[MAXBUF];
static uint8_t* cmd_parser(uint8_t* data, uint16_t len, uint16_t* out_len)
{
	if (len == 0)
		return NULL;

	switch (data[0])
	{
	case 'h':
		buffer[0] = 'O';
		buffer[1] = 'K';
		buffer[2] = '\n';
		*out_len = 3;
		break;
	default:
		buffer[0] = 'E';
		buffer[1] = 'R';
		buffer[2] = 'R';
		buffer[3] = '\n';
		*out_len = 4;
		break;
	}

	return buffer;
}

void app_main()
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    bt_rfcomm_init(&cmd_parser);
}

