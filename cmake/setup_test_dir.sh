# Souffle - A Datalog Compiler
# Copyright (c) 2021 The Souffle Developers. All rights reserved
# Licensed under the Universal Permissive License v 1.0 as shown at:
# - https://opensource.org/licenses/UPL
# - <souffle root>/licenses/SOUFFLE-UPL.txt

set -e

INPUT_DIR="$1"
OUTPUT_DIR="$2"
TEST_NAME="$3"
EXTRA_DATA="$4"

mkdir -p "${OUTPUT_DIR}"
rm -rf "${OUTPUT_DIR}"/*

if [ -z ${EXTRA_DATA} ]; then
    EXTRA_DATA="nothing"
fi

if [ ${EXTRA_DATA} = "json" ]; then
    # The json tests require additional shenanigans
    sed 's/\(@<:@@:>@,@:>@\)$/\\1/' "${INPUT_DIR}/${TEST_NAME}.out" > "${OUTPUT_DIR}/${TEST_NAME}.out.expected"

    for file in $(cd "${INPUT_DIR}" && find . -maxdepth 1 -name "*.json"); do
        sed 's/\(@<:@@:>@,@:>@\)$/\\1/' "${INPUT_DIR}/${file}" > "${OUTPUT_DIR}/${file}.expected"
    done
else
    cp "${INPUT_DIR}/${TEST_NAME}.out" "${OUTPUT_DIR}/${TEST_NAME}.out.expected"

    if [ ${EXTRA_DATA} = "python" ] || [ ${EXTRA_DATA} = "java" ]; then
        cp "${INPUT_DIR}/${TEST_NAME}-${EXTRA_DATA}.out" "${OUTPUT_DIR}/${TEST_NAME}-${EXTRA_DATA}.out.expected"
    fi

    if [ ${EXTRA_DATA} = "provenance" ]; then
        cp "${INPUT_DIR}/${TEST_NAME}.in" "${OUTPUT_DIR}/${TEST_NAME}.in"
    fi
fi

cp "${INPUT_DIR}/${TEST_NAME}".err "${OUTPUT_DIR}/${TEST_NAME}".err.expected

for file in $(cd "${INPUT_DIR}" && find . -maxdepth 1 -name "*.csv"); do
    cp "${INPUT_DIR}/$file" "${OUTPUT_DIR}/${file}.expected"
done

find "${INPUT_DIR}" -maxdepth 1 -name "*.csv" | wc -l > "${OUTPUT_DIR}/num.expected"

for file in $(cd "${OUTPUT_DIR}" && find . -maxdepth 1 -name "*.expected"); do
    sort "${OUTPUT_DIR}/$file" > "${OUTPUT_DIR}/${file}.sorted"
done
