FROM ubuntu:20.10

ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update -qq && \
    apt-get install -y -qq  autoconf automake bison build-essential clang doxygen flex g++ git libncurses5-dev libtool libsqlite3-dev make mcpp python sqlite zlib1g-dev wget curl valgrind

RUN git clone --depth 1 --branch master https://gitlab.com/akihe/radamsa.git && \
    cd radamsa && \
    make -j$(nproc) && \
    make -j$(nproc) install

ENV CFLAGS="-g -fsanitize=address -fno-omit-frame-pointer"
ENV CXXFLAGS="$CFLAGS"
ENV LDFLAGS="-fsanitize=address"
CMD ./bootstrap && \
    ./configure --enable-debug && \
    make -j install && \
    bash fuzzing/radamsa/radamsa.sh
