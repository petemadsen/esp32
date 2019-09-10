/**
 * This code is public domain.
 */
#ifndef PROJECT_OTA_H
#define PROJECT_OTA_H

#include <esp_err.h>


esp_err_t ota_init();

/**
 * @brief Marks the factory parition as *boot*.
 * @return NULL on sucess. Error string otherwise.
 */
const char* ota_reboot();


#endif
