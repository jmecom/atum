#!/usr/bin/env sh

./target &
sudo ./inject $! payload.dylib
sleep 1
pkill target
