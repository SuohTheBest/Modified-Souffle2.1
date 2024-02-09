#!/bin/bash

set -e
set -x

export MSYS_NO_PATHCONV=1
export MSYS2_ARG_CONV_EXCL="*"

cat > run-msvc-test.sh <<EOF
set -e
set -x

export SOUFFLE_CATEGORY="Interface"
export SOUFFLE_CONFS="-j8"
export SOUFFLE_TESTS_MSVC_VARS="C:\VS\VC\Auxiliary\Build\vcvars64.bat"
export GETOPT_LIB="$(cygpath -w $(pwd)/vcpkg/installed/x64-windows/lib/getopt.lib)"
export GETOPT_INCLUDE="$(cygpath -w $(pwd)/vcpkg/installed/x64-windows/include)"

./bootstrap
./configure
make -j$(nproc)
make check || true
cd tests
set +e
./testsuite -v -d -x
TESTS_RESULT=\$?
set -e
find testsuite.dir/ \( -name "*.err" -or -name "*.out" \) -printf "===== %p =====\n" -exec cat \{\} \;
exit \$TESTS_RESULT
EOF

chmod +x run-msvc-test.sh
PATH="$PATH:$(pwd)/vcpkg/installed/x64-windows/bin" wsl ./run-msvc-test.sh
