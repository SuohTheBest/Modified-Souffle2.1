#!/usr/bin/env bash

set -eu
shopt -s globstar

souffle=/work
persist=${souffle}/fuzzing/afl/corpus
ro_tests=/tests
temp_dir=/temp/afl
mkdir -p "${persist}"
mkdir -p "${ro_tests}"
mkdir -p "${temp_dir}"
cp --no-clobber ./tests/**/*.csv ./tests/**/*.facts "${temp_dir}"
cp --no-clobber ./tests/**/*.dl "${ro_tests}"

# These test cases cause issues, either by causing crashes or by being slow.
rm "${ro_tests}/circuit_sat.dl"
rm "${ro_tests}/factorial.dl"
rm "${ro_tests}/functors.dl"
rm "${ro_tests}/inline_nqueens.dl"

cd "${temp_dir}"
# Remove redundant files
afl-cmin \
  -i "${ro_tests}" \
  -o "${persist}" \
  -t 30000+ \
  -m 100M+ \
  "${souffle}/src/souffle" @@

# Minimize individual files in parallel
#
# This takes a really long time (hours) and only slightly reduces corpus size;
# it's not clear if it's worth the wait.
mkdir -p "${persist}-min"
# We do 8 * number of cores because there are several outliers that take a
# really long time to minimize relative to the others (they reach the 5s
# timeout), so this results in better overall utilization by running more of
# those in parallel.
cores=$(($(nproc) * 8))
for k in $(seq 1 ${cores} $(ls ${persist} | wc -l)); do
  for i in $(seq 0 $((${cores} - 1))); do
    file=$(ls ${persist} | sed $(expr $i + $k)"q;d")
    afl-tmin \
      -i "${persist}/${file}" \
      -o "${persist}-min/min-$(basename ${file})" \
      -t 5000+ \
      -m 100M+ \
      "${souffle}/src/souffle" @@ &
  done
  wait
done
