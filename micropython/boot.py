# This file is executed on every boot (including wake-boot from deepsleep)

import esp
esp.osdebug(None)

import webrepl
webrepl.start()

import time


#############################################################################

import json
import network

wifi = None
try:
    with open("wifi.json") as file:
        wifi = json.load(file)
except:
    print("Could not read wifi.json")

if wifi is not None:
    station = network.WLAN(network.STA_IF)
    station.active(True)
    try:
        sta = station.scan()
        print(sta)
        for name, *params in sta:
            print("STA %s" % name)
            if name.startswith("fuji-"):
                print("..connecting...")
                station.connect(name, wifi["password"])
                station.isconnected()
                station.ifconfig()
                time.sleep(1.0)
                break
    except Exception as ex:
        print("Something bad happend.")
        print(ex)

    if not station.isconnected():
        station.connect("fuji-fritz", wifi["password"])
        station.isconnected()
        station.ifconfig()
