FROM ubuntu:focal

ARG DEBIAN_FRONTEND=noninteractive

RUN apt-get update -y && \
    apt-get upgrade -y && \
    apt-get install -y \
    autoconf \
    automake \
    bash-completion \
    bison \
    build-essential \
    clang \
    clang-format \
    clang-tidy \
    debhelper \
    default-jdk-headless \
    devscripts \
    doxygen \
    fakeroot \
    flex \
    g++ \
    gcovr \
    gdb \
    git \
    graphviz \
    libffi-dev \
    libncurses5-dev \
    libsqlite3-dev \
    libtool \
    make \
    mcpp \
    pkg-config \
    python3-dev \
    sqlite \
    swig \
    zlib1g-dev

RUN useradd --create-home --shell /bin/bash souffle

WORKDIR /home/souffle
