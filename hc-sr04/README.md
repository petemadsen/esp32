# Project: HC-SR04

* Using ESP32 RMT
  * https://github.com/espressif/esp-idf/tree/master/examples/peripherals/rmt_nec_tx_rx

* Protocol (https://www.acmesystems.it/HC-SR04):
  * Send > 10us pulse on TRIGGER pin
  * Measure pulse width on ECHO pin
    * Width of 150us-25ms divided by 58 gives cm (148 to get inches)
	* Width of 38ms when nothing detected

