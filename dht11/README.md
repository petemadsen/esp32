# Project: DHT11

* Using ESP32 RMT and GPIO
  * https://github.com/espressif/esp-idf/tree/master/examples/peripherals/rmt_nec_tx_rx

* Protocol
  * Level HIGH initially
  * Pull it down for at least 18ms
  * Pull up again
  * DHT11 pulls the line down and sends a 80us pulse
  * Now 40 data bits coming in
    * 26-28us for "0"
	* 70us for "0"
  * byte 0: humidity
  * byte 1: humidity integral
  * byte 2: temperature
  * byte 3: temperature integral
  * byte 5: CRC (sum) of byte 0 and byte 2

