set +x

sh -c "cd /souffle && ./bootstrap && ./configure --quiet && cd src \
&& make -j4 >/dev/null 2>/dev/null \
&& find . -name '*.cpp' -exec \
clang-tidy {} -fix -quiet -extra-arg=-std=c++17 \
 -- -I. -I\$(pwd)/include -std=c++17 -pthread \
 -DUSE_PROVENANCE -DUSE_LIBZ  -DUSE_SQLITE  -fopenmp \; 1>>tidy.out 2>>tidy.err"

set -x
git diff -m
#disable error notification until the codebase is more stable relative to the linter
git diff-index --quiet HEAD --
