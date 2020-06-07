#!/usr/bin/env sh

set -euox pipefail

mkdir -p build
cd build
CPPUTEST_HOME=/usr/local/Cellar/cpputest/4.0 cmake ..
make
