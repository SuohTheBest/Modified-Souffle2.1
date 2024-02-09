# --------------------------------------------------
# Installing ruby for packagecloud
# --------------------------------------------------

FROM ruby:latest AS ruby-img

RUN gem install package_cloud

WORKDIR /souffle

COPY . .

# --------------------------------------------------
# CPack configuration
# --------------------------------------------------
FROM i386/ubuntu:18.04

ARG DEBIAN_FRONTEND=noninteractive

# Create a souffle directory
WORKDIR /souffle

# Makes sure we are installing the latest version of CMake
RUN apt-get update && \
	apt-get -y install gpg wget sudo && \
    wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | gpg --dearmor - | sudo tee /usr/share/keyrings/kitware-archive-keyring.gpg >/dev/null && \
	apt-key adv --keyserver keyserver.ubuntu.com --recv-keys DE19EB17684BA42D && \
	echo 'deb [signed-by=/usr/share/keyrings/kitware-archive-keyring.gpg] https://apt.kitware.com/ubuntu/ bionic main' | sudo tee /etc/apt/sources.list.d/kitware.list >/dev/null

# Install souffle build dependencies
RUN apt-get update && \
	apt-get -y install bash-completion autoconf automake bison build-essential clang doxygen flex g++ git libffi-dev libncurses5-dev libtool libsqlite3-dev make mcpp python sqlite zlib1g-dev cmake

# For CMakeLists.txt to figure out the specific version of Ubuntu
RUN apt-get -y install lsb-release

# Get package cloud CLI tool
COPY --from=ruby-img /usr/local/bundle/bin/package_cloud /usr/local/bin

# Copy everything into souffle directory
COPY --from=ruby-img /souffle .

# Install dependencies to build souffle
# RUN sudo sh/setup/install_ubuntu_deps.sh

ENV DOMAIN_SIZE "32bit"
ENV PKG_EXTENSION ".deb"
ENV PKG_CLOUD_OS_NAME "ubuntu/bionic"

ENTRYPOINT [".github/actions/create-package/entrypoint.sh"]