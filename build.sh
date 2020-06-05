#!/usr/bin/env sh

set -euox pipefail

mkdir -p build
cd build
cmake ..
make
