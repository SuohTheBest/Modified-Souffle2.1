# Souffle - A Datalog Compiler
# Copyright (c) 2021 The Souffle Developers. All rights reserved
# Licensed under the Universal Permissive License v 1.0 as shown at:
# - https://opensource.org/licenses/UPL
# - <souffle root>/licenses/SOUFFLE-UPL.txt

set -e

TEST_NAME="$1"
EXTRA_DATA="$2"

if [ -z ${EXTRA_DATA} ]; then
    EXTRA_DATA="nothing"
fi

if [ ${EXTRA_DATA} = "json" ]; then
    # The json tests require additional shenanigans
    sed 's/\(@<:@@:>@,@:>@\)$/\\1/' "${TEST_NAME}.out" | sort > "${TEST_NAME}.out.sorted"
else
    sort "${TEST_NAME}.out" > "${TEST_NAME}.out.sorted"
fi;

diff "${TEST_NAME}.out.sorted" "${TEST_NAME}.out.expected.sorted"

diff "${TEST_NAME}.err" "${TEST_NAME}.err.expected"

if [ ${EXTRA_DATA} = "python" ] || [ ${EXTRA_DATA} = "java" ]; then
    # We also want to check the output from the swig run
    sort "${TEST_NAME}-${EXTRA_DATA}.out" > "${TEST_NAME}-${EXTRA_DATA}.out.sorted"
    diff "${TEST_NAME}-${EXTRA_DATA}.out.sorted" "${TEST_NAME}-${EXTRA_DATA}.out.expected.sorted"
fi
