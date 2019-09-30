/**
 * This code is public domain.
 */
#ifndef PROJECT_OTA_H
#define PROJECT_OTA_H

#include <freertos/FreeRTOS.h>


esp_err_t ota_init();

/**
 * @brief Marks the factory parition as *boot*.
 * @return NULL on sucess. Error string otherwise.
 */
const char* ota_reboot();


bool ota_need_update();


#endif
