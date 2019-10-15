/**
 * This code is public domain.
 */
#ifndef PROJECT_SYSTEM_LOG_H
#define PROJECT_SYSTEM_LOG_H

#include <esp_err.h>

#define LOG_MAX_ENTRIES	10
#define LOG_MAX_LINELEN	78

esp_err_t mylog_add(const char* str);


bool mylog_get(uint8_t num, const char**);


#endif
