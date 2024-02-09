sh -c "cd /souffle && ./bootstrap \
&& ./configure --enable-sanitise-memory \
&& make -j4 \
&& TESTSUITEFLAGS=-j4 SOUFFLE_CATEGORY=Syntactic,Semantic,FastEvaluation,Profile,Interface,Provenance make check -j4 \
|| (sh/after_failure.sh && false)"
