#!/bin/bash

# Install dependency based on the runner OS.

# Enable debugging and logging of shell operations
# that are executed.
set -e
set -x

if [[ $1 == *"macos"* ]]; then
  $(dirname $0)/install_macos_deps.sh
elif [[ $1 == *"ubuntu"* ]]; then
  sudo $(dirname $0)/install_ubuntu_deps.sh
fi
