/**
 * This code is public domain.
 */
#ifndef MY_SLEEP_H
#define MY_SLEEP_H


#include <freertos/FreeRTOS.h>


void my_sleep_task(void* arg);


void my_sleep_enable_nightmode(bool);
void my_sleep_enable_watch_wifi(bool);


bool my_sleep_nightmode();
bool my_sleep_watch_wifi();


#endif
