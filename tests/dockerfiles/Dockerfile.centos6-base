FROM centos:6

ENV SCCACHE_VER=0.2.8

RUN yum update -y -q \
    && yum -y -q install \
    epel-release \
    centos-release-scl \
    && yum -y -q install \
    cairo-devel \
    dbus-glib-devel \
    docbook2X \
    freetype-devel \
    gcc \
    git \
    imlib2-devel \
    lcov \
    libcurl-devel \
    libical-devel \
    libircclient-devel \
    libmicrohttpd-devel \
    librsvg2-devel \
    libX11-devel \
    libXdamage-devel \
    libXext-devel \
    libXft-devel \
    libXinerama-devel \
    libxml2-devel \
    make \
    man \
    mysql-devel \
    ncurses-devel \
    openssl-devel \
    patch \
    pulseaudio-libs-devel \
    readline-devel \
    && curl -sL https://github.com/mozilla/sccache/releases/download/${SCCACHE_VER}/sccache-${SCCACHE_VER}-x86_64-unknown-linux-musl.tar.gz -o sccache-${SCCACHE_VER}-x86_64-unknown-linux-musl.tar.gz \
    && tar xf sccache-${SCCACHE_VER}-x86_64-unknown-linux-musl.tar.gz \
    && cp sccache-${SCCACHE_VER}-x86_64-unknown-linux-musl/sccache /usr/bin \
    && rm -rf sccache-${SCCACHE_VER}-x86_64-unknown-linux-musl* \
    && curl -L https://github.com/Kitware/CMake/releases/download/v3.13.4/cmake-3.13.4-Linux-x86_64.sh -o cmake-3.13.4-Linux-x86_64.sh \
    && chmod +x cmake-3.13.4-Linux-x86_64.sh \
    && ./cmake-3.13.4-Linux-x86_64.sh --prefix=/usr --skip-license \
    && rm cmake-3.13.4-Linux-x86_64.sh \
    && mkdir /luabuild \
    && pushd /luabuild \
    && curl -R -O http://www.lua.org/ftp/lua-5.3.5.tar.gz \
    && tar zxf lua-5.3.5.tar.gz \
    && cd lua-5.3.5 \
    && make -j4 linux MYCFLAGS=-fPIC MYLDFLAGS=-fPIC \
    && make install INSTALL_TOP=/usr \
    && popd \
    && rm -rf /luabuild \
    && mkdir -p /usr/lib/pkgconfig \
    && echo $'\
V= 5.3\n\
R= 5.3.5\n\
prefix= /usr\n\
exec_prefix=${prefix}\n\
libdir= /usr/lib\n\
includedir=${prefix}/include\n\
\n\
Name: Lua\n\
Description: An Extensible Extension Language\n\
Version: ${R}\n\
Requires:\n\
Libs: -llua -lm -ldl\n\
Cflags: -I${includedir}' \
>> /usr/lib64/pkgconfig/lua.pc
