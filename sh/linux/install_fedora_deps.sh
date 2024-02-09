#!/bin/sh

retry() {
  local result=0
  local count=1
  while [ $count -le 3 ]; do
    [ $result -ne 0 ] && {
      echo -e "\n${ANSI_RED}The command \"$@\" failed. Retrying, $count of 3.${ANSI_RESET}\n" >&2
    }
    "$@"
    result=$?
    [ $result -eq 0 ] && break
    count=$(($count + 1))
    sleep 1
  done

  [ $count -gt 3 ] && {
    echo -e "\n${ANSI_RED}The command \"$@\" failed 3 times.${ANSI_RESET}\n" >&2
  }

  return $result
}

# Build dependencies
retry yum install -y -q bison clang doxygen flex gcc gcc-c++ git kernel-devel libffi-devel libtool make mcpp ncurses-devel python sqlite sqlite-devel sudo swig zlib-devel

# Set up the package builder
retry yum install -y -q rpm-build ruby-devel
retry gem install --no-ri --no-rdoc rake
retry gem install --no-ri --no-rdoc fpm

fpm --version
