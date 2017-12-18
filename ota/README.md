Origin: https://github.com/espressif/esp-idf/tree/master/examples/system/ota

# Changes

Configuration in config.h
```
#define EXAMPLE_WIFI_SSID ""
#define EXAMPLE_WIFI_PASS ""
#define EXAMPLE_SERVER_IP   ""
#define EXAMPLE_SERVER_PORT ""
#define EXAMPLE_FILENAME ""
```

Sequence:
* Try to download EXAMPLE_FILENAME several times. If successful, download, install and boot into it.
* Otherwise, try to find a valid OTA image and boot into it.
* Finally, simply reboot.

