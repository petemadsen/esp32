/**
 * This code is public domain.
 */
#ifndef PROJECT_WIFI_H
#define PROJECT_WIFI_H


#include <freertos/event_groups.h>


extern EventGroupHandle_t wifi_event_group;
extern const int WIFI_CONNECTED;

void wifi_init(bool b);


void wifi_stop();


void wifi_start();


#endif
