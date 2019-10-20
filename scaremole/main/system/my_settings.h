/**
 * This code is public domain.
 */
#ifndef PROJECT_SETTING_H
#define PROJECT_SETTING_H

#include <freertos/FreeRTOS.h>
#include <esp_err.h>

esp_err_t settings_init(void);
esp_err_t settings_erase(void);

esp_err_t settings_set_int32(const char* key, int32_t val, bool must_exist);
esp_err_t settings_get_int32(const char* key, int32_t* val, bool save_if_missing);

// uses malloc()/free() on buffer
esp_err_t settings_get_str(const char* key, const char** buffer, bool save_if_missing);
esp_err_t settings_set_str(const char* key, const char* val, bool must_exist);

int32_t settings_boot_counter(void);


#endif
