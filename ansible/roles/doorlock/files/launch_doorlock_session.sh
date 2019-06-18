#! /bin/bash

xset -dpms
xset -s off
xset -b off
unclutter &
chromium --kiosk --fullscreen --app=https://localhost/display
