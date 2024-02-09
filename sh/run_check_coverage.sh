#Do not test in parallel to ensure profiling is recorded correctly
#Runs interpreted tests (generate coverage logs for interpreted souffle)
#Runs compiled tests and preserve test directory(for coverage of the new binaries)
#Runs unit tests
#Runs gcov from each compilation directory so all relative links work
#Erase gcda/gcno files to prevent codecov from overwriting gcov files
#(New gcov files overwrite old, so gcov souffle;gcov souffle_prof is bad

# use codecov.yml to instruct codecov to convert /souffle/include/souffle to src

#codecov.sh options: -K to remove colour from output, -Z to exit with an error code
#  -F to set descriptive flag, -U to pass options to curl

#TODO: codecov.yml can be configured to require two (or more) coverage reports, so this could be split up into multiple jobs.
#TODO2: Work out why combining the two coverage tests changes coverage results
curl --connect-timeout 30 --retry 17 -s -S https://codecov.io/env -o codecovEnv.sh \
&& CI_ENV="`bash codecovEnv.sh`" \
&& curl --connect-timeout 30 --retry 17 -s -S https://codecov.io/bash -o codecov.sh \
&& ./bootstrap \
&& export CXXFLAGS=--coverage \
&& BASEDIR=\$(pwd) \
&& ./configure --enable-debug --enable-swig && make -j4 \
&& cd tests \
&& TESTSUITEFLAGS='-j1' SOUFFLE_CATEGORY=Evaluation SOUFFLE_CONFS='-j4 -ra.html' make check -j1 \
; rm testsuite \
&& TESTSUITEFLAGS='-j1 -d' SOUFFLE_CATEGORY=Swig,Syntactic,Semantic,Evaluation,Interface,Profile,Provenance SOUFFLE_CONFS='-j1 -ra.html,-c -j4 -ra.html' make check -j1 \
&& cd \$BASEDIR/src \
&& make check -j1 \
; set -x \
; gcov -bp *gcda .libs/*gcda \
; rm *gcda *gcno .libs/*gcda .libs/*gcno \
; cd \$BASEDIR/src/tests \
; gcov -bp *gcda \
; rm *gcda *gcno .libs/*gcda .libs/*gcno \
; cd \$BASEDIR/src/interpreter/tests \
; gcov -bp *gcda \
; rm *gcda *gcno .libs/*gcda .libs/*gcno \
; cd \$BASEDIR/src/ast/tests \
; gcov -bp *gcda \
; rm *gcda *gcno .libs/*gcda .libs/*gcno \
; cd \$BASEDIR/src/ram/tests \
; gcov -bp *gcda \
; rm *gcda *gcno .libs/*gcda .libs/*gcno \
; cd \$BASEDIR/tests/testsuite.dir \
&& cd \$(ls --group-directories-first |head -n1) \
&& find .. -name '*gcda'|xargs gcov -bp \
&& cd \$BASEDIR/src \
&& find . -name '*gcda'|xargs gcov -bp \
&& cd \$BASEDIR \
; find . -name '*gcda' -delete \
; find . -name '*gcno' -delete \
&& bash codecov.sh -K -Z -F full
