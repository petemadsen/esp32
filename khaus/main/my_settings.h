/**
 * This code is public domain.
 */
#ifndef PROJECT_SETTING_H
#define PROJECT_SETTING_H

#include <freertos/FreeRTOS.h>
#include <esp_err.h>

esp_err_t settings_init();

esp_err_t settings_set_int32(const char* key, int32_t val);
esp_err_t settings_get_int32(const char* key, int32_t* val, bool save_if_missing);

// uses malloc()/free() on buffer
esp_err_t settings_get_str(const char* key, char** buffer, const char* default_val, bool save_if_missing);
esp_err_t settings_set_str(const char* key, const char* val);


#endif
