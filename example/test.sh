#!/usr/bin/env sh

cd ../build/example
./target &
sudo ./injector $! payload.dylib
sleep 1
pkill target
