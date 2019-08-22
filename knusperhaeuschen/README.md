# Project: knusperhauschen

* dfplayer
* buttons
* transistor switch
* *TODO* ina219
  * will monitor system voltage and send warning notifications to awShutters
    and turn off wifi and more
* HTTP support

*TODO* Setting like bell track and volume should be saved.

*TODO* speaker make noise on startup

*TODO* blink if has wifi

*TODO* light is not a push button but a switch button.

*TODO* save settings


## HTTP

* /status

* /play?NUM

* /bell

* /bell?NUM

* /volume

* /volume?NUM


## Save Power

* *TODO* Reduce speed from 240 to 80 MHz
* Sleep at night
* *TODO* Use PIR
* *TODO* switch off ADC
* *TODO* switch off dfplayer


### Sleep at Night

In order to enter the night mode we need to know the time. We use the SNTP
module to get it.

During the night, the module goes to sleep but wakes up every hour. It tries
to update/get the time and to go sleep again.

Wake up and wait about one minute before
going to sleep again.
