FROM debian:latest
RUN apt-get update \
  && apt-get install -y \
       cmake \
       git \
       g++ \
       libimlib2-dev \
       liblua5.3-dev \
       libxext-dev \
       libxft-dev \
       libxdamage-dev \
       libxinerama-dev \
       ncurses-dev

COPY . /conky
WORKDIR /conky/build
ARG X11=yes

RUN sh -c 'if [ "$X11" = "yes" ] ; then cmake ../ ; else cmake -DBUILD_X11=OFF ../ ; fi' \
  && make -j5 all \
  && make -j5 install \
  && apt-get remove -y \
       cmake \
       git \
       g++ \
       libimlib2-dev \
       liblua5.3-dev \
       libxext-dev \
       libxft-dev \
       libxdamage-dev \
       libxinerama-dev \
       ncurses-dev \
  && rm -rf /var/lib/apt/lists/* \
  && rm -rf /conky \

CMD conky
