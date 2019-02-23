FROM ubuntu:bionic

RUN apt-get update \
  && DEBIAN_FRONTEND=noninteractive \
  apt-get install -qy --no-install-recommends \
  software-properties-common \
  wget \
  gpg-agent \
  cmake \
  git \
  audacious-dev \
  libaudclient-dev \
  libcairo2-dev \
  libcurl4-openssl-dev \
  libical-dev \
  libimlib2-dev \
  libircclient-dev \
  libiw-dev \
  liblua5.3-dev \
  libmicrohttpd-dev \
  libmysqlclient-dev \
  libpulse-dev \
  librsvg2-dev \
  libsystemd-dev \
  libxdamage-dev \
  libxext-dev \
  libxft-dev \
  libxinerama-dev \
  libxml2-dev \
  libxmmsclient-dev \
  libxnvctrl-dev \
  ncurses-dev \
  lcov \
  docbook2x \
  man \
  less

RUN wget -q https://apt.llvm.org/llvm-snapshot.gpg.key \
  && apt-key add llvm-snapshot.gpg.key \
  && add-apt-repository 'deb http://apt.llvm.org/bionic/ llvm-toolchain-bionic-7 main' \
  && DEBIAN_FRONTEND=noninteractive \
  apt-get install -qy --no-install-recommends \
  clang-7 \
  lldb-7 \
  lld-7 \
  libc++-7-dev \
  libc++abi-7-dev \
  clang-tools-7 \
  clang-format-7 \
  clang-tidy-7 \
  && apt-get clean \
  && rm -rf /var/lib/apt/lists/*
