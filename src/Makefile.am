bin_PROGRAMS = conky

if BUILD_AUDACIOUS
audacious = audacious.c audacious.h
endif

if BUILD_BMPX
bmpx = bmpx.c
endif

if BUILD_MPD
mpd = mpd.c libmpdclient.c
endif

if BUILD_XMMS2
xmms2 = xmms2.c
endif

if BUILD_LINUX
linux = linux.c top.c
PTHREAD_LIBS =  -lpthread
endif

#if BUILD_SOLARIS
#solaris = solaris.c
#endif

if BUILD_FREEBSD
freebsd = freebsd.c
PTHREAD_LIBS =  -pthread
endif

#if BUILD_NETBSD
#netbsd = netbsd.c
#endif

if BUILD_PORT_MONITORS
port_monitors = libtcp-portmon.h libtcp-portmon.c hash.h hash.c
endif

if BUILD_X11
x11 = x11.c
endif

if BUILD_HDDTEMP
hddtemp = hddtemp.c
endif

conky_SOURCES = 		\
	$(audacious)            \
	$(bmpx)                 \
	common.c 		\
	conky.c                 \
	conky.h                 \
	$(freebsd)              \
	fs.c 			\
	$(hddtemp)		\
	$(linux) 		\
	mail.c 			\
	mixer.c 		\
	$(mpd)                  \
	$(netbsd)               \
	$(port_monitors)        \
	$(solaris) 		\
	remotec.c               \
	remotec.h               \
	remoted.c 		\
	remoted.h 		\
	$(x11)                  \
	$(xmms2) 

AM_LDFLAGS = $(CAIRO_LIBS) $(PTHREAD_LIBS) -lm

EXTRA_DIST = 			\
	audacious.c		\
	audacious.h		\
	bmpx.c			\
	freebsd.c		\
	ftp.c                   \
	ftp.h                   \
	hash.c			\
	hash.h			\
	hddtemp.c		\
	linux.c			\
	libmpdclient.c          \
	libmpdclient.h          \
	libtcp-portmon.c	\
	libtcp-portmon.h	\
	mpd.c			\
	netbsd.c		\
	solaris.c 		\
	top.h                   \
	x11.c                   \
	xmms2.c
