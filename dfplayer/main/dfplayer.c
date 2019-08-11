/**
 * dfplayer.c
 */
#include "dfplayer.h"

#include <stdio.h>
#include <string.h>
#include "esp_err.h"
#include "esp_log.h"
#include "driver/rmt.h"
#include "driver/periph_ctrl.h"
#include "soc/rmt_reg.h"

#include "sdkconfig.h"

static const char* MY_TAG = "DFPLAYER";


static void tx_task(void* arg)
{
	for(;;)
	{
        vTaskDelay(1000 / portTICK_PERIOD_MS);
	}
}


void dfplayer_init()
{
	xTaskCreate(tx_task, "tx_task", 2048, leds, 10, NULL);
}
