# Project: knusperhaeuschen

* dfplayer
* buttons
* load transistor switch
* **TODO** INA219 (i2c)
  * will monitor system voltage and send warning notifications to awShutters
    and turn off wifi and more
* **TODO** DHT12 (i2c)
* BMP280 (i2c)
* HTTP support
* Save settings
* Show blue led if has wifi
* Switch off the lights after x minutes
* Support ws2812b


## Pinout

* 17 - ws2812b pin
* 18 - bell button (external pull-up)
* 19 - light button (external pull-up)
* 23 - light transistor
* 21 - i2c sda
* 22 - i2c scl
* 25 - PAM8304 audio amplifier
* 32 - audio amplifier enable pin
* 34 - voltage sense pin


## HTTP Server

### GET /status

Returns various system parameters

### GET /light

Returns current light state.

### GET /light?ONOFF

Use 0 to switch the light off and 1 to turn the light on.

### GET /play

Plays currently selected bell sound.

### POST /bell?NUM

Command to upload binary files:

```curl --data-binary @uploadfile "IP/bell?NUM"```

Example:

```curl --data-binary @440Hz8bitUnsigned.wav 192.168.1.56/bell?3```

Return value:

0 on success.

Any other value to be found in ```read_wav.c```.

### GET /bell[?NUM]

Sets/gets the bell to play.

### GET /volume

Returns volume.

### GET /volume?NUM

Sets volume to NUM. NUM range is 0 to 100.

### GET /ota

Reboots the device into factory partition which should include an OTA updater.

### GET /sget?NAME

Gets setting NAME.

Returns 404 when NAME could not be read.

### GET /sset?NAME=VALUE

Sets setting NAME to VALUE

`/sset?ERASE` will erase the NVS partition.


## HTTP Client

Client tries to connect to awShutters:
* `GET /knusperhaeuschen/touch`
* `POST /knusperhaeuschen/save` - sends a couple of key-value pairs
  * `board_temp=VALUE`
  * `board_voltage=VALUE`
  * `out_temp=VALUE`
  * `light=VALUE`


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
