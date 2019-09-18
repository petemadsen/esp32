/*
 * msgeq7.h
 */
#ifndef MAIN_MSGEQ7_H
#define MAIN_MSGEQ7_H


#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include "freertos/task.h"
#include "freertos/queue.h"


extern void msgeq7_init();


#define MSGEQ_NUM_BANDS 7
extern void msgeq7_get(uint32_t levels[MSGEQ_NUM_BANDS]);


#endif /* MAIN_MSGEQ7_H */
