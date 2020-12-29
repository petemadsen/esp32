#!/bin/bash

IP=192.168.1.11

echo
curl $IP/status

echo
curl $IP/set?http://192.168.1.86:8085/ota/file/ota-client2

echo
curl $IP/status

echo
curl $IP/set?http://192.168.1.86:8085/ota/file/ota-client

echo
curl $IP/status

echo
curl $IP/set?http://192.168.1.86:8085/ota/file/khaus

echo
curl $IP/status
