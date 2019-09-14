# Project: knusperhaeuschen

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
* **TODO** switch off the lights after x minutes


## Pinout

* 18 - bell button
* 19 - light button
* 23 - light transistor
* 21 - i2c sda
* 22 - i2c scl
* 25 - PAM8304 audio amplifier
* 34 - voltage sense pin
* 32 - audio amplifier enable pin


## HTTP Server

* `/status` returns various system parameters

* `/play?NUM`

* `/bell`

* `/bell?NUM`

* `/volume`

* `/volume?NUM`

* `/ota` initiates the OTA process


## HTTP Client

Client tries to connect to awShutters:
* `GET /knusperhaeuschen/touch`
* `POST /knusperhaeuschen/save` - sends a couple of key-value pairs
  * `board_temp`
  * `voltage`
  * `out_temp`


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
