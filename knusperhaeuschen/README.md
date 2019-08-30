# Project: knusperhauschen

* dfplayer
* buttons
* load transistor switch
* **TODO** INA219 (i2c)
  * will monitor system voltage and send warning notifications to awShutters
    and turn off wifi and more
* **TODO** DHT12 (i2c)
* **TODO** BMP280 (i2c)
* HTTP support
* Save settings
* Show blue led if has wifi


## Pinout

* 18 - bell button
* 19 - light button
* 23 - light transistor
* 21 - i2c sda (defined by hardware)
* 22 - i2c scl (defined by hardware)
* 25 - speaker transistor


## HTTP

* `/status` returns various system parameters

* `/play?NUM`

* `/bell`

* `/bell?NUM`

* `/volume`

* `/volume?NUM`

* `/ota` initiates the OTA process


## Save Power

* Reducing speed from 160 to 80 MHz provides around 6mA less power consumption.
* Sleep at night


### Sleep at Night

In order to enter the night mode we need to know the time. We use the SNTP
module to get it.

During the night, the module goes to sleep but wakes up every hour. It tries
to update/get the time and to go sleep again.

Wake up and wait about one minute before
going to sleep again.
