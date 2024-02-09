#!/bin/bash

# Enable debugging and logging of shell operations
# that are executed.
set -e
set -x

# Install requirements of MAC OS X
. sh/osx/install.sh

rm /usr/local/include/c++ || true

brew unlink gcc
brew install gcc@10
# Using 'g++' will call the xcode link to clang
export CC=gcc-10
export CXX=g++-10

$CXX --version
$CC --version

set +e
set +x
