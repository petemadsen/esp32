# OTA

Based on https://github.com/espressif/esp-idf/tree/master/examples/system/ota

## Requirements

WiFi configuration in config.h
```
#define CONFIG_SSID ""
#define CONFIG_PASS ""
#define CONFIG_GATEWAY ""
#define CONFIG_NETMASK "255.255.255.0"
#define CONFIG_ADDRESS ""
```

## How does it work?

* Try to download EXAMPLE_FILENAME several times.
  * If successful, download, install and boot into it.
  * Otherwise, try to find a valid OTA image and boot into it.
* Finally, simply reboot.

## Todo

* sleep if cannot download
