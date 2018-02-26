FROM debian:latest
RUN apt-get update && apt-get install -y cmake git g++ libimlib2-dev liblua5.3-dev libxext-dev libxft-dev libxdamage-dev libxinerama-dev ncurses-dev
COPY . /root/
RUN mkdir /root/build
WORKDIR /root/build
ARG X11=yes
RUN sh -c 'if [ "$X11" = "yes" ] ; then cmake ../ ; else cmake -DBUILD_X11=OFF ../ ; fi'
RUN make all
RUN make install
CMD conky
