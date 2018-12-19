FROM ubuntu:latest
RUN apt-get update \
  && DEBIAN_FRONTEND=noninteractive \
  apt-get install -qy --no-install-recommends \
      cmake \
      git \
      g++ \
      libimlib2-dev \
      libxext-dev \
      libxft-dev \
      libxdamage-dev \
      libxinerama-dev \
      libmysqlclient-dev \
      libical-dev \
      libircclient-dev \
      libcairo2-dev \
      libmicrohttpd-dev \
      ncurses-dev \
      liblua5.3-dev \
      librsvg2-dev \
      libaudclient-dev \
      libxmmsclient-dev \
      libpulse-dev \
      libcurl4-gnutls-dev \
      audacious-dev \
      libsystemd-dev \
      libxml2-dev \
      libxnvctrl-dev \
      libiw-dev

COPY . /conky
WORKDIR /conky/build
ARG X11=yes

RUN sh -c 'if [ "$X11" = "yes" ] ; then \
    cmake \
      -DBUILD_MYSQL=ON \
      -DBUILD_LUA_CAIRO=ON \
      -DBUILD_LUA_IMLIB2=ON \
      -DBUILD_LUA_RSVG=ON \
      -DBUILD_LUA_CAIRO=ON \
      -DBUILD_AUDACIOUS=ON \
      -DBUILD_XMMS2=ON \
      -DBUILD_ICAL=ON \
      -DBUILD_IRC=ON \
      -DBUILD_HTTP=ON \
      -DBUILD_ICONV=ON \
      -DBUILD_PULSEAUDIO=ON \
      -DBUILD_JOURNAL=ON \
      -DBUILD_RSS=ON \
      -DBUILD_NVIDIA=ON \
      -DBUILD_WLAN=ON \
    ../ \
  ; else \
    cmake \
      -DBUILD_X11=OFF \
      -DBUILD_MYSQL=ON \
      -DBUILD_LUA_CAIRO=ON \
      -DBUILD_LUA_IMLIB2=ON \
      -DBUILD_LUA_RSVG=ON \
      -DBUILD_LUA_CAIRO=ON \
      -DBUILD_AUDACIOUS=ON \
      -DBUILD_XMMS2=ON \
      -DBUILD_ICAL=ON \
      -DBUILD_IRC=ON \
      -DBUILD_HTTP=ON \
      -DBUILD_ICONV=ON \
      -DBUILD_PULSEAUDIO=ON \
      -DBUILD_JOURNAL=ON \
      -DBUILD_RSS=ON \
      -DBUILD_WLAN=ON \
    ../ \
  ; fi' \
  && make -j5 all \
  && make -j5 install \
  && apt-get remove -y \
      cmake \
      git \
      g++ \
      libimlib2-dev \
      libxext-dev \
      libxft-dev \
      libxdamage-dev \
      libxinerama-dev \
      libmysqlclient-dev \
      libical-dev \
      libircclient-dev \
      libcairo2-dev \
      libmicrohttpd-dev \
      ncurses-dev \
      liblua5.3-dev \
      librsvg2-dev \
      audacious-dev \
      libaudclient-dev \
      libxmmsclient-dev \
      libpulse-dev \
      libcurl4-gnutls-dev \
      libsystemd-dev \
      libxml2-dev \
      libxnvctrl-dev \
  && rm -rf /var/lib/apt/lists/* \
  && rm -rf /conky

CMD conky
