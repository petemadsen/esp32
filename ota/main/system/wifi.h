/**
 * This code is public domain.
 */
#ifndef PROJECT_WIFI_H
#define PROJECT_WIFI_H


#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>


extern EventGroupHandle_t wifi_event_group;
extern const EventBits_t WIFI_CONNECTED;


void wifi_init(bool fixed_ip);


void wifi_stop();


unsigned int wifi_reconnects();


const char* wifi_ssid();


#endif
