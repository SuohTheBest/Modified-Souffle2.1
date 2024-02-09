#!/bin/bash


# Enable debugging and logging of shell operations
# that are executed.
set -e
set -x

JOBS=$(nproc || sysctl -n hw.ncpu || echo 2)

# create configure files
./bootstrap
./configure $1

# build souffle
make -j$JOBS

set +e
set +x
