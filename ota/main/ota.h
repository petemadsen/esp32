#ifndef PROJECT_OTA_H
#define PROJECT_OTA_H


#include <freertos/FreeRTOS.h>


esp_err_t ota_init(void);


void ota_fatal_error(void);


void ota_task(void* args);


#endif
