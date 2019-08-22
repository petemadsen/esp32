/**
 * This code is public domain.
 */
#ifndef PROJECT_WIFI_H
#define PROJECT_WIFI_H


#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>


extern EventGroupHandle_t wifi_event_group;
extern const EventBits_t WIFI_CONNECTED;


void wifi_init(bool b);


void wifi_stop();

#endif
