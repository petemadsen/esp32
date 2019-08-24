# Project: knusperhauschen

* dfplayer
* buttons
* load transistor switch
* **TODO** ina219
  * will monitor system voltage and send warning notifications to awShutters
    and turn off wifi and more
* HTTP support
* save settings
* show blue led if has wifi
* **TODO** speaker makes noises when not operating


## HTTP

* `/status` returns various system parameters

* `/play?NUM`

* `/bell`

* `/bell?NUM`

* `/volume`

* `/volume?NUM`

* `ota` initiates the OTA process


## Save Power

* Reducing speed from 160 to 80 MHz provides around 6mA less power consumption.
* Sleep at night
* **TODO** Use PIR to wake up
* **TODO** turn off dfplayer


### Sleep at Night

In order to enter the night mode we need to know the time. We use the SNTP
module to get it.

During the night, the module goes to sleep but wakes up every hour. It tries
to update/get the time and to go sleep again.

Wake up and wait about one minute before
going to sleep again.
