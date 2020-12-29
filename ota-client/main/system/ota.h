/**
 * This code is public domain.
 */
#ifndef PROJECT_OTA_H
#define PROJECT_OTA_H

#include <freertos/FreeRTOS.h>


esp_err_t ota_init(void);


esp_err_t ota_set_download_url(const char* url);
// buffer will be created with malloc() and must be freed with free().
esp_err_t ota_download_url(const char** buffer);


/**
 * @brief Marks the factory parition as *boot*.
 * @return NULL on sucess. Error string otherwise.
 */
const char* ota_reboot(void);


bool ota_need_update(void);


void ota_reboot_task(void* arg);


bool ota_has_update_partition();


const char* ota_sha256();


const char* ota_project_name();


#endif
