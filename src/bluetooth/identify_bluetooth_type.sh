#!/bin/bash

pkg-config --exists capi-network-bluetooth && echo "tizen_capi" && exit 0

bluez_ver=$(pkg-config --modversion bluez)
[[ $bluez_ver =~ ^5.([[:digit:]]*)$ ]] && echo "bluez5" && exit 0
[[ $bluez_ver =~ ^4.([[:digit:]]*)$ ]] && echo "bluez4" && exit 0

echo "Unknown"
exit 1
