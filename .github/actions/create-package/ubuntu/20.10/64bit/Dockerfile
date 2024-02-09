FROM ubuntu:20.10

ARG DEBIAN_FRONTEND=noninteractive

# Create a souffle directory
WORKDIR /souffle

# Install souffle build dependencies
RUN apt-get update && \
	apt-get -y install \
	bash-completion \
	sudo \
	autoconf \
	automake \
	bison \
	build-essential \
	clang \
	doxygen \
	flex \
	g++ \
	git \
	libffi-dev \
	libncurses5-dev \
	libtool \
	libsqlite3-dev \
	make \
	mcpp \
	python \
	sqlite \
	zlib1g-dev \
	cmake

# For CMakeLists.txt to figure out the specific version of Ubuntu
RUN apt-get -y install lsb-release

# Install dependencies for packagecloud CLI
RUN apt-get -y install ruby-full
RUN sudo gem install package_cloud

# Copy everything into souffle directory
COPY . .

ENV DOMAIN_SIZE "64bit"
ENV PKG_EXTENSION ".deb"
ENV PKG_CLOUD_OS_NAME "ubuntu/groovy"

ENTRYPOINT [".github/actions/create-package/entrypoint.sh"]