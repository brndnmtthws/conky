FROM ubuntu:focal AS builder

RUN apt-get update \
  && DEBIAN_FRONTEND=noninteractive \
  apt-get install -qy --no-install-recommends \
  audacious-dev \
  ca-certificates \
  clang \
  git \
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
  libncurses-dev \
  libpulse-dev \
  librsvg2-dev \
  libssl-dev \
  libsystemd-dev \
  libxdamage-dev \
  libxext-dev \
  libxft-dev \
  libxinerama-dev \
  libxml2-dev \
  libxmmsclient-dev \
  libxnvctrl-dev \
  make \
  patch \
  wget \
  && apt-get clean \
  && rm -rf /var/lib/apt/lists/*

# Compile CMake, we need the latest because the bug here (for armv7 builds):
# https://gitlab.kitware.com/cmake/cmake/-/issues/20568
WORKDIR /cmake
RUN wget -q https://github.com/Kitware/CMake/releases/download/v3.19.6/cmake-3.19.6.tar.gz \
  && tar xf cmake-3.19.6.tar.gz \
  && cd cmake-3.19.6 \
  && ./bootstrap \
  && make -j5 \
  && make -j5 install \
  && cd \
  && rm -rf /cmake

COPY . /conky
WORKDIR /conky/build

ARG X11=yes

RUN sh -c 'if [ "$X11" = "yes" ] ; then \
  cmake \
  -DCMAKE_C_COMPILER=clang \
  -DCMAKE_CXX_COMPILER=clang++ \
  -DCMAKE_INSTALL_PREFIX=/opt/conky \
  -DBUILD_AUDACIOUS=ON \
  -DBUILD_HTTP=ON \
  -DBUILD_ICAL=ON \
  -DBUILD_ICONV=ON \
  -DBUILD_IRC=ON \
  -DBUILD_JOURNAL=ON \
  -DBUILD_LUA_CAIRO=ON \
  -DBUILD_LUA_CAIRO=ON \
  -DBUILD_LUA_IMLIB2=ON \
  -DBUILD_LUA_RSVG=ON \
  -DBUILD_MYSQL=ON \
  -DBUILD_NVIDIA=ON \
  -DBUILD_PULSEAUDIO=ON \
  -DBUILD_RSS=ON \
  -DBUILD_WLAN=ON \
  -DBUILD_XMMS2=ON \
  ../ \
  ; else \
  cmake \
  -DCMAKE_C_COMPILER=clang \
  -DCMAKE_CXX_COMPILER=clang++ \
  -DCMAKE_INSTALL_PREFIX=/opt/conky \
  -DBUILD_AUDACIOUS=ON \
  -DBUILD_HTTP=ON \
  -DBUILD_ICAL=ON \
  -DBUILD_ICONV=ON \
  -DBUILD_IRC=ON \
  -DBUILD_JOURNAL=ON \
  -DBUILD_LUA_CAIRO=ON \
  -DBUILD_LUA_CAIRO=ON \
  -DBUILD_LUA_IMLIB2=ON \
  -DBUILD_LUA_RSVG=ON \
  -DBUILD_MYSQL=ON \
  -DBUILD_PULSEAUDIO=ON \
  -DBUILD_RSS=ON \
  -DBUILD_WLAN=ON \
  -DBUILD_X11=OFF \
  -DBUILD_XMMS2=ON \
  ../ \
  ; fi' \
  && make -j5 all \
  && make -j5 install

FROM ubuntu:focal

RUN apt-get update \
  && DEBIAN_FRONTEND=noninteractive \
  apt-get install -qy --no-install-recommends \
  libaudclient2 \
  libcairo2 \
  libcurl4 \
  libical3 \
  libimlib2 \
  libircclient1 \
  libiw30 \
  liblua5.3-0 \
  libmicrohttpd12 \
  libmysqlclient21 \
  libncurses6 \
  libpulse0 \
  librsvg2-2 \
  libsm6 \
  libsystemd0 \
  libxcb-xfixes0 \
  libxdamage1 \
  libxext6 \
  libxfixes3 \
  libxft2 \
  libxinerama1 \
  libxml2 \
  libxmmsclient6 \
  libxnvctrl0 \
  && apt-get clean \
  && rm -rf /var/lib/apt/lists/*

COPY --from=builder /opt/conky /opt/conky

ENV PATH="/opt/conky/bin:${PATH}"
ENV LD_LIBRARY_PATH="/opt/conky/lib:${LD_LIBRARY_PATH}"

ENTRYPOINT [ "/opt/conky/bin/conky" ]
