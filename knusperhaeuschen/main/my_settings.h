/**
 * This code is public domain.
 */
#ifndef PROJECT_SETTING_H
#define PROJECT_SETTING_H

#include <freertos/FreeRTOS.h>
#include <esp_err.h>

esp_err_t settings_set(const char* key, int32_t val);
esp_err_t settings_get(const char* key, int32_t* val);


#endif
