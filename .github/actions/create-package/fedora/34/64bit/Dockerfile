FROM fedora:34

# Create a souffle directory
WORKDIR /souffle

# Install souffle build dependencies
RUN yum -y install \
    bash-completion \
    autoconf \
    automake \
    bison \
    clang \
    doxygen \
    flex \
    g++ \
    git \
    libffi \
    libffi-devel \
    ncurses-devel \
    libtool \
    libsqlite3x \
    make \
    mcpp \
    python \
    sqlite \
    sqlite-devel \
    zlib-devel \
    cmake

# For CMakeLists.txt to figure out the specific version of Ubuntu. Also installing rpmbuild so that we can package our app into an rpm file
RUN yum -y install redhat-lsb rpm-build

# Install dependencies for packagecloud CLI as well as package_cloud CLI
RUN gpg --keyserver keyserver.ubuntu.com --recv-keys 409B6B1796C275462A1703113804BB82D39DC0E3 7D2BAF1CF37B13E2069D6956105BD0E739499BDB && \
	curl -sSL https://get.rvm.io | bash -s stable
RUN /bin/bash -l -c ". /etc/profile.d/rvm.sh && rvm install 2.7.4 && gem install package_cloud"

# Copy everything into souffle directory
COPY . .

ENV DOMAIN_SIZE "64bit"
ENV PKG_EXTENSION ".rpm"
ENV PKG_CLOUD_OS_NAME "fedora/34"

ENTRYPOINT ["/bin/bash", "-l", "-c", ".github/actions/create-package/entrypoint.sh"]