# Souffle - A Datalog Compiler
# Copyright (c) 2021 The Souffle Developers. All rights reserved
# Licensed under the Universal Permissive License v 1.0 as shown at:
# - https://opensource.org/licenses/UPL
# - <souffle root>/licenses/SOUFFLE-UPL.txt

set -e

INPUT_DIR="$1"
EXTRA_DATA="$2"
BINARY="$3" #BINARY will be set to gzip or sqlite3

find . -maxdepth 1 -name "*.csv.expected.sorted" | wc -l > num.generated

diff num.generated num.expected

for file in $(find . -maxdepth 1 -name "*.csv"); do
    sort "${file}" > "${file}.generated.sorted"
    diff "${file}.generated.sorted" "${file}.expected.sorted"
done


if [ -z ${EXTRA_DATA} ]; then
    # We're done
    exit 0
elif [ ${EXTRA_DATA} = "gzip" ]; then
    EXTRA_FILE_PAT="*.output"
elif [ ${EXTRA_DATA} = "sqlite3" ]; then
    EXTRA_FILE_PAT="*.output"
elif [ ${EXTRA_DATA} = "json" ]; then
    EXTRA_FILE_PAT="*.json"
else
    echo "Unknown processing type '${EXTRA_DATA}'"
    exit 1
fi

EXTRA_FILES=$(find . -maxdepth 1 -name "${EXTRA_FILE_PAT}")

for file in ${EXTRA_FILES}; do
    if [ ${EXTRA_DATA} = "gzip" ]; then
        # Strip the .gz.output suffix.  There should be a corresponding .csv file
        # in the input directory
        EXTRA_FILE=${file%%.gz.output}
        "${BINARY}" -d -c "$file" > "${EXTRA_FILE}.generated.unsorted"
    elif [ ${EXTRA_DATA} = "sqlite3" ]; then
        # Strip the .sqlite.output suffix.  There should be a corresponding .csv file
        # in the input directory
        EXTRA_FILE=${file%%.sqlite.output}.csv
        "${BINARY}" -batch "$file" -init "${INPUT_DIR}/$file.script" "" > "${EXTRA_FILE}.generated.unsorted"
    elif [ ${EXTRA_DATA} = "json" ]; then
        EXTRA_FILE="${file}"
        sed 's/\(@<:@@:>@,@:>@\)$/\\1/' "${file}" > "${file}.generated.unsorted"
    fi

    sort "${EXTRA_FILE}.generated.unsorted" > "${EXTRA_FILE}.generated.sorted"
    diff "${EXTRA_FILE}.generated.sorted" "${EXTRA_FILE}.expected.sorted"
done
