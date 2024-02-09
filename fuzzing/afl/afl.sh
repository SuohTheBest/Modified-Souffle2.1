#!/usr/bin/env bash

set -eu
shopt -s globstar

souffle=/work
persist=${souffle}/fuzzing/afl/data
temp_dir=/temp/afl
mkdir -p "${persist}"
mkdir -p "${temp_dir}"

ro_tests=/tmp/tests
if [[ -d ${souffle}/fuzzing/afl/corpus-min ]]; then
  ro_tests=${souffle}/fuzzing/afl/corpus-min
elif [[ -d ${souffle}/fuzzing/afl/corpus ]]; then
  ro_tests=${souffle}/fuzzing/afl/corpus
else
  mkdir -p "${ro_tests}"
  cp --no-clobber ./tests/**/*.dl "${ro_tests}"
fi

# These test cases cause issues, either by causing crashes or by being slow.
rm -f "${ro_tests}/circuit_sat.dl"
rm -f "${ro_tests}/factorial.dl"
rm -f "${ro_tests}/functors.dl"

# Can't easily disable core dumps in Docker
export AFL_I_DONT_CARE_ABOUT_MISSING_CRASHES=1
# The bitmap gets pretty saturated pretty early
export AFL_INST_RATIO=10
# Don't print afl-cc banners
export AFL_QUIET=1
# Stops if there are no new paths for a while
export AFL_EXIT_WHEN_DONE=1

cp --no-clobber ./tests/**/*.csv ./tests/**/*.facts "${temp_dir}"
cd "${temp_dir}"
pids=""
for i in $(seq 1 $(nproc)); do

  # AFL docs:
  # If you don’t want to do deterministic fuzzing at all, it’s OK to run all
  # instances with -S. With very slow or complex targets, or when running heavily
  # parallelized jobs, this is usually a good plan.
  afl-fuzz \
    -t 30000+ \
    -m 100M+ \
    -x "${souffle}/fuzzing/afl/dict.txt" \
    -i "${ro_tests}" \
    -o "${persist}" \
    -S "fuzzer${i}" \
    "${souffle}/src/souffle" @@ &
  pids="${pids} ${!}"
done
for pid in ${pids}; do
  wait "${pid}"
done
