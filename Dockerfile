FROM ubuntu:noble AS builder

RUN apt-get update \
  && DEBIAN_FRONTEND=noninteractive \
  apt-get install -qy --no-install-recommends \
  audacious-dev \
  ca-certificates \
  clang \
  cmake \
  curl \
  gfortran \
  git \
  gperf \
  libarchive-dev \
  libaudclient-dev \
  libc++-dev \
  libc++abi-dev \
  libcairo2-dev \
  libcurl4-openssl-dev \
  libdbus-glib-1-dev \
  libical-dev \
  libimlib2-dev \
  libircclient-dev \
  libiw-dev \
  libnl-3-dev\
  libnl-genl-3-dev\
  libnl-route-3-dev\
  libjsoncpp-dev \
  liblua5.3-dev \
  libmicrohttpd-dev \
  libmysqlclient-dev \
  libncurses-dev \
  libpulse-dev \
  librhash-dev \
  librsvg2-dev \
  libssl-dev \
  libsystemd-dev \
  libuv1-dev \
  libxdamage-dev \
  libxext-dev \
  libxft-dev \
  libxinerama-dev \
  libxml2-dev \
  libxmmsclient-dev \
  libxnvctrl-dev \
  ninja-build \
  patch \
  && apt-get clean \
  && rm -rf /var/lib/apt/lists/*

COPY . /conky
WORKDIR /conky/build

ARG X11=yes

RUN sh -c 'if [ "$X11" = "yes" ] ; then \
  cmake -G Ninja \
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
  -DBUILD_LUA_CAIRO_XLIB=ON \
  -DBUILD_LUA_IMLIB2=ON \
  -DBUILD_LUA_RSVG=ON \
  -DBUILD_MYSQL=ON \
  -DBUILD_NVIDIA=ON \
  -DBUILD_PULSEAUDIO=ON \
  -DBUILD_RSS=ON \
  -DBUILD_WAYLAND=OFF \
  -DBUILD_WLAN=ON \
  -DBUILD_XMMS2=ON \
  ../ \
  ; else \
  cmake -G Ninja \
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
  -DBUILD_LUA_IMLIB2=ON \
  -DBUILD_LUA_RSVG=ON \
  -DBUILD_MYSQL=ON \
  -DBUILD_PULSEAUDIO=ON \
  -DBUILD_RSS=ON \
  -DBUILD_WAYLAND=OFF \
  -DBUILD_WLAN=ON \
  -DBUILD_X11=OFF \
  -DBUILD_XMMS2=ON \
  ../ \
  ; fi' \
  && cmake --build . \
  && cmake --install .

FROM ubuntu:noble

RUN apt-get update \
  && DEBIAN_FRONTEND=noninteractive \
  apt-get install -qy --no-install-recommends \
  libaudclient2 \
  libc++1 \
  libc++abi1 \
  libcairo2 \
  libcurl4t64 \
  libdbus-glib-1-2 \
  libical3t64 \
  libimlib2t64 \
  libircclient1 \
  libiw30t64 \
  liblua5.3-0 \
  libmicrohttpd12t64 \
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
  libxi6 \
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
