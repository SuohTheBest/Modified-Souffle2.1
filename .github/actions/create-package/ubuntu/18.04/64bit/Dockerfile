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
FROM ubuntu:18.04

ARG DEBIAN_FRONTEND=noninteractive

# Create a souffle directory
WORKDIR /souffle

RUN apt-get update && \
	# Makes sure we are installing the latest version of CMake
	apt-get -y install gpg wget sudo && \
    wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | gpg --dearmor - | sudo tee /usr/share/keyrings/kitware-archive-keyring.gpg >/dev/null && \
	apt-key adv --keyserver keyserver.ubuntu.com --recv-keys DE19EB17684BA42D && \
	echo 'deb [signed-by=/usr/share/keyrings/kitware-archive-keyring.gpg] https://apt.kitware.com/ubuntu/ bionic main' | sudo tee /etc/apt/sources.list.d/kitware.list >/dev/null


# Install souffle build dependencies
RUN apt-get update && \
	apt-get -y install bash-completion autoconf automake bison build-essential clang doxygen flex g++ git libffi-dev libncurses5-dev libtool libsqlite3-dev make mcpp python sqlite zlib1g-dev cmake

# Ubuntu 18.04 ships with g++ version 7, which is problematic
RUN apt-get -y install software-properties-common && \
	add-apt-repository -y ppa:ubuntu-toolchain-r/test && \
	apt-get update && \
	apt-get -y install gcc-10 g++-10 && \
	update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-10 800 --slave /usr/bin/g++ g++ /usr/bin/g++-10


# For CMakeLists.txt to figure out the specific version of Ubuntu
RUN apt-get -y install lsb-release

# Get package cloud CLI tool
COPY --from=ruby-img /usr/local/bundle/bin/package_cloud /usr/local/bin

# Copy everything into souffle directory
COPY --from=ruby-img /souffle .

# Install dependencies to build souffle
# RUN sudo sh/setup/install_ubuntu_deps.sh

ENV DOMAIN_SIZE "64bit"
ENV PKG_EXTENSION ".deb"
ENV PKG_CLOUD_OS_NAME "ubuntu/bionic"

CMD [".github/actions/create-package/entrypoint.sh"]