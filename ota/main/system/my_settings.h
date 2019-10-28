/**
 * This code is public domain.
 */
#ifndef PROJECT_SETTING_H
#define PROJECT_SETTING_H

#include <freertos/FreeRTOS.h>
#include <esp_err.h>


#define STORAGE_APP "app"
#define STORAGE_OTA "ota"


esp_err_t settings_init(void);
esp_err_t settings_erase(void);

esp_err_t settings_set_int32(const char* storage, const char* key, int32_t val, bool must_exist);
esp_err_t settings_get_int32(const char* storage, const char* key, int32_t* val, bool save_if_missing);

// uses malloc()/free() on buffer
esp_err_t settings_get_str(const char* storage, const char* key, char** buffer, bool save_if_missing);
esp_err_t settings_set_str(const char* storage, const char* key, const char* val, bool must_exist);

int32_t settings_boot_counter(void);


#endif
