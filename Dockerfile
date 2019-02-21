FROM ubuntu:bionic AS builder
RUN apt-get update \
  && DEBIAN_FRONTEND=noninteractive \
  apt-get install -qy --no-install-recommends \
  cmake \
  git \
  g++ \
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
  ncurses-dev 

COPY . /conky
WORKDIR /conky/build
ARG X11=yes

RUN sh -c 'if [ "$X11" = "yes" ] ; then \
  cmake \
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

FROM ubuntu:bionic

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
  libmysqlclient20 \
  libncurses5 \
  libpulse0 \
  librsvg2-2 \
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
