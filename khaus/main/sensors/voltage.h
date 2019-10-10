/**
 * This code is public domain.
 */
#ifndef PROJECT_VOLTAGE_H
#define PROJECT_VOLTAGE_H


#include <esp_adc_cal.h>


void voltage_init(adc1_channel_t);


double voltage_get(void);


#endif
