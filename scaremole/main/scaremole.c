/**
 * This code is public domain.
 */
#include "scaremole.h"
#include "common.h"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <esp_log.h>

#include <driver/gpio.h>

static const char* MY_TAG = "scaremole/scaremole";



void scaremole_run()
{
	ESP_LOGI(MY_TAG, "run");

	gpio_pad_select_gpio(PIN_BUZZER);
	gpio_set_direction(PIN_BUZZER, GPIO_MODE_OUTPUT);
	gpio_hold_dis(PIN_BUZZER);

	gpio_pad_select_gpio(PIN_MOTOR);
	gpio_set_direction(PIN_MOTOR, GPIO_MODE_OUTPUT);
	gpio_hold_dis(PIN_MOTOR);

#if 0
	gpio_set_level(PIN_BUZZER, 1);
	vTaskDelay(1000 / portTICK_PERIOD_MS);
	gpio_set_level(PIN_BUZZER, 0);
#endif

	// buzzer
	for (int i=0; i<3; ++i)
	{
		gpio_set_level(PIN_BUZZER, 1);
		vTaskDelay(200 / portTICK_PERIOD_MS);
		gpio_set_level(PIN_BUZZER, 0);
		vTaskDelay(200 / portTICK_PERIOD_MS);
	}

	// motor
	TickType_t times[] = { 2000, 1000, 3000 };
	for (int i=0; i<3; ++i)
	{
		gpio_set_level(PIN_MOTOR, 1);
		vTaskDelay(times[i] / portTICK_PERIOD_MS);
		gpio_set_level(PIN_MOTOR, 0);
		vTaskDelay(500 / portTICK_PERIOD_MS);
	}

	// buzzer
	for (int i=0; i<3; ++i)
	{
		gpio_set_level(PIN_BUZZER, 1);
		vTaskDelay(500 / portTICK_PERIOD_MS);
		gpio_set_level(PIN_BUZZER, 0);
		vTaskDelay(500 / portTICK_PERIOD_MS);
	}

	// disable all output
	gpio_deep_sleep_hold_en();
	gpio_hold_en(PIN_BUZZER);
	gpio_hold_en(PIN_MOTOR);

	ESP_LOGI(MY_TAG, "done");
}
