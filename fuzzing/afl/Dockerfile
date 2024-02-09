FROM ubuntu:20.10

ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update -qq && \
apt-get install -y -qq  autoconf automake bison build-essential clang doxygen flex g++ git libncurses5-dev libtool libsqlite3-dev make mcpp python sqlite zlib1g-dev wget curl valgrind tmux

RUN git clone --depth 1 --branch master https://github.com/google/AFL.git && \
    cd AFL && \
    make -j$(nproc) install

# See for why ASan isn't used (yet):
# https://afl-1.readthedocs.io/en/latest/notes_for_asan.html
ENV CC=afl-clang CXX=afl-clang++ AFL_HARDEN=1 AFL_HANG_TMOUT=60 CFLAGS="-O2" CXXFLAGS="-O2"
CMD ./bootstrap && \
    ./configure --enable-debug && \
    make -j install && \
    bash fuzzing/afl/afl.sh
