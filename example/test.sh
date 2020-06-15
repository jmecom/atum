#!/usr/bin/env sh

cd ../build/example
./target &
echo 'Attach debugger...'
sleep 3
sudo ./injector $! libpayload.dylib
# sleep 3
# pkill target
