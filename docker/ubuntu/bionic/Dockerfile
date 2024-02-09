FROM souffle/ubuntu:bionic-base

ARG SOUFFLE_CC="gcc"
ARG SOUFFLE_CXX="g++"
ARG SOUFFLE_CONFIGURE_OPTIONS=""
ARG SOUFFLE_GITHUB_USER="souffle-lang"
ARG SOUFFLE_GIT_BRANCH="master"
ARG SOUFFLE_GIT_REVISION="HEAD"
ARG SOUFFLE_MAKE_JOBS="2"

ENV CC "${SOUFFLE_CC}"
ENV CXX "${SOUFFLE_CXX}"

RUN apt install -y git

USER souffle

RUN git clone https://github.com/${SOUFFLE_GITHUB_USER}/souffle /home/souffle/souffle

WORKDIR /home/souffle/souffle

RUN git pull
RUN git checkout ${SOUFFLE_GIT_BRANCH}
RUN git reset --hard ${SOUFFLE_GIT_REVISION}
RUN git clean -xdf
RUN ./bootstrap
RUN ./configure ${SOUFFLE_CONFIGURE_OPTIONS}
RUN make -j${SOUFFLE_MAKE_JOBS}
RUN ./src/souffle --version

USER root

RUN make install -j${SOUFFLE_MAKE_JOBS}

USER souffle

WORKDIR /home/souffle

RUN souffle -h
