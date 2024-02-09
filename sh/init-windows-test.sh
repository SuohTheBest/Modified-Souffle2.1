#!/bin/bash

choco install wsl-ubuntu-1804 --ignorechecksums
choco install visualstudio2019community -y --version 16.7.2.0 --params="--installPath 'C:\\VS' --add Microsoft.VisualStudio.Component.VC.Tools.x86.x64 --add Microsoft.VisualStudio.Component.VC.CoreBuildTools --add Microsoft.VisualStudio.Component.VC.Redist.14.Latest --add Microsoft.VisualStudio.Component.Windows10SDK --add Microsoft.VisualStudio.Component.Windows10SDK.18362 --includeRecommended --passive --wait"
./sh/install-get-opt-msvc.bat
wsl bash -c "apt-get update -y"
wsl bash -c "apt-get install -y bison build-essential dos2unix flex libffi-dev libncurses5-dev libtool lsb-release mcpp swig zlib1g-dev libsqlite3-dev"
