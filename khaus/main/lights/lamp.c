/**
 * This code is public domain.
 */
#include <driver/gpio.h>

#include "sdkconfig.h"

#include "lamp.h"


static bool m_relay_on = false;
static int64_t m_relay_on_at = 0; // time in secs from boot when relay was turned on
static uint8_t m_relay_pin = 0;


void lamp_init(uint8_t relay_pin)
{
	m_relay_pin = relay_pin;

	gpio_pad_select_gpio(m_relay_pin);
	gpio_set_direction(m_relay_pin, GPIO_MODE_OUTPUT);

	lamp_set_relay(false);
}


void lamp_set_relay(bool on)
{
	m_relay_on = on;

	if (m_relay_on)
		m_relay_on_at = esp_timer_get_time() / 1000 / 1000;
	else
		m_relay_on_at = 0;

	gpio_set_level(m_relay_pin, m_relay_on);
}


int lamp_get_relay()
{
	return m_relay_on;
}


int64_t lamp_get_on_time()
{
	return m_relay_on_at;
}
