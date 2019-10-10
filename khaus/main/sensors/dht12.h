/**
 * This code is public domain.
 */
#ifndef PROJECT_DHT12_H
#define PROJECT_DHT12_H

#include <freertos/FreeRTOS.h>


int dht12_get(uint8_t addr, double* temp, double* humidity);


#endif
