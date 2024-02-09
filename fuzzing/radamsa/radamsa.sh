#!/usr/bin/env bash

set -u
shopt -s globstar

## Configuration
log_level=${LOG_LEVEL:-2}
valgrind=${VALGRIND:-}

log() {
  if [[ ${log_level} -ge ${1} ]]; then
    printf "[%s] %s\n" "${2}" "${3}"
  fi
}
debug() { log 3 DEBUG "${1}"; }
info() { log 2 INFO "${1}"; }
warn() { log 1 WARN "${1}"; }
error() { log 1 ERROR "${1}"; }
bail() {
  error "${1}"
  exit "${2:-1}"
}

souffle=/work
persist=${souffle}/fuzzing/radamsa/data
tmp=/temp/radamsa
eval=/tmp/eval
mkdir -p "${persist}"
mkdir -p "${tmp}"
mkdir -p "${eval}"
cp --no-clobber ./tests/**/*.csv ./tests/**/*.facts "${tmp}"
cp --no-clobber ./tests/**/*.csv ./tests/**/*.facts "${eval}"
cd "${tmp}"

instrument() {
    debug "Running $@"
    if [[ -n "${valgrind}" ]]; then
      timeout 2m valgrind --tool=memcheck --leak-check=no "$@"
    else
      timeout 1m "$@"
    fi
}

run() {
  debug "Starting thread ${1}"
  while true; do
    suffix=${1}-${SECONDS}
    input=test-${suffix}.dl
    log=test-${suffix}.log
    profile=test-${suffix}.json
    html_report=test-${suffix}.html
    radamsa "${souffle}"/tests/**/*.dl > "${input}"
    debug "Generated input ${input}"

    debug "Testing evaluation of ${input}"

    instrument \
        ${souffle}/src/souffle \
        --debug-report="${html_report}" \
        --profile=${profile} \
        "${input}" &> "${log}"

    # Only report exits due to signals, and ignore SIGTERM from timeout
    code=$?
    if [ ${code} -eq 124 ]; then
        debug "Evaluation timed out"
    fi
    if [ ${code} -gt 127 ]; then

      # Don't report uninteresting bugs
      if grep -c "cannot find user-defined operator" "${log}" > /dev/null; then
        debug "Skipping uninteresting bug: functor not found"
        continue
      elif grep -c "std::out_of_range" "${log}" > /dev/null; then
        debug "Skipping uninteresting bug: out of range"
        continue
      fi

      info "Crash found! Exit code: ${code}. Input: ${input}"
      info "Log: $(cat "${log}")"
      cp "${input}" "${persist}/CRASH-${input}"
      cp "${log}" "${persist}"
      continue
    fi

    debug "Testing synthesis of ${input}"

    synthesis_log="test-${suffix}-synth.log"
    instrument \
        ${souffle}/src/souffle \
        -o test-${suffix} \
        "${input}" &> "${synthesis_log}"
    code=$?
    if [ ${code} -eq 124 ]; then
        debug "Synthesis timed out"
    fi
    if [ ${code} -gt 127 ]; then
      info "Synthesis failed! Exit code: ${code}. Input: ${input}"
      info "Log: $(cat "${synthesis_log}")"
      cp "${input}" "${persist}"
      cp "${log}" "${persist}"
      cp "${synthesis_log}" "${persist}"
      continue
    fi

    # Test that the profile can be parsed and a report generated
    prof_log=test-${suffix}-prof.log
    if [[ -f ${profile} ]]; then

      debug "Testing profiling of ${input}"

      instrument ${souffle}/src/souffle-profile -j "${profile}" &> "${prof_log}"
      code=$?
      if [ ${code} -eq 124 ]; then
          debug "Profiler timed out"
      fi
      if [ ${code} -ne 0 ] && [ ${code} -ne 124 ]; then
        # https://github.com/souffle-lang/souffle/issues/1759#issuecomment-732540547
        if grep -c "File cannot be floaded" "${prof_log}" > /dev/null; then
          debug "Skipping uninteresting bug: Issue #1759"
          continue
        fi

        info "Profile reporting failed! Input: ${input}"
        info "Log: $(cat "${prof_log}")"
        cp "${input}" "${persist}"
        cp "${log}" "${persist}"
        cp "${prof_log}" "${persist}"
        cp "${profile}" "${persist}"
      fi
    fi

    cpp=test-${suffix}.cpp
    run_log=test-${suffix}-run.log
    compile_log=test-${suffix}-compile.log
    prog="${eval}/${cpp%.cpp}"
    if [[ -f ${cpp} ]]; then

      debug "Testing compilation of ${input}"

      if ! souffle-compile "${cpp}" &> "${compile_log}"; then
        if grep -c "No space left on device" "${compile_log}" > /dev/null; then
          info "Ran out of space!"
          rm -rf "${tmp}"
          mkdir -p "${tmp}"
        fi
        if grep -c "signal terminated program cc1plus" "${compile_log}" > /dev/null; then
          continue
        fi
        info "Compile failed! Input: ${input}"
        cp "${input}" "${persist}"
        cp "${cpp}" "${persist}"
        cp "${log}" "${persist}"
        cp "${compile_log}" "${persist}"
        continue
      fi

      debug "Testing execution of compiled ${input}"

      cp ${cpp%.cpp} "${prog}"
      instrument "${prog}" &> "${run_log}"
      code=$?
      if [ ${code} -ne 0 ] && [ ${code} -ne 124 ]; then
        if grep -c "Error loading data:" "${run_log}" > /dev/null; then
          debug "Skipping uninteresting issue: Couldn't load data"
          continue
        elif grep -c "wrong index position" "${run_log}" > /dev/null; then
          debug "Skipping uninteresting issue: Wrong index position"
          continue
        fi

        info "Run failed! Input: ${input}"
        info "Log: $(cat "${run_log}")"
        cp "${input}" "${persist}"
        cp "${cpp}" "${persist}"
        cp "${log}" "${persist}"
        cp "${run_log}" "${persist}"
        cp "${prog}" "${persist}"
      fi
    fi

    rm -f "${input}" "${run_log}" "${compile_log}" "${log}" "${synthesis_log}" "${profile}" "${html_report}" "${cpp}" "${prog}" "${prof_log}"
  done
}

pids=""
for i in $(seq 0 $(nproc)); do
  run "${i}" &
  pids="${pids} ${!}"
done
for pid in ${pids}; do
  info "Waiting..."
  wait "${pid}"
done
