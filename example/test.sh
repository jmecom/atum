#!/usr/bin/env sh

cd ../build/example
./target &
sudo ./injector $! libpayload.dylib
sleep 1
pkill target
