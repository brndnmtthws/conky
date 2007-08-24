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

if BUILD_OPENBSD
openbsd = openbsd.c
PTHREAD_LIBS =  -pthread
endif

if BUILD_PORT_MONITORS
port_monitors = libtcp-portmon.h libtcp-portmon.c
endif

if BUILD_X11
x11 = x11.c
endif

if BUILD_HDDTEMP
hddtemp = hddtemp.c
endif

if BUILD_RSS
rss = rss.c prss.c prss.h
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
	$(openbsd)              \
	$(port_monitors)        \
	$(rss)			\
	$(solaris) 		\
	timed_thread.c		\
	timed_thread.h		\
	mboxscan.c		\
	mboxscan.h		\
	$(x11)                  \
	$(xmms2) 

AM_LDFLAGS = $(PTHREAD_LIBS) -lm

EXTRA_DIST = 			\
	audacious.c		\
	audacious.h		\
	bmpx.c			\
	freebsd.c		\
	hddtemp.c		\
	linux.c			\
	libmpdclient.c          \
	libmpdclient.h          \
	libtcp-portmon.c	\
	libtcp-portmon.h	\
	mpd.c			\
	netbsd.c		\
	openbsd.c		\
	solaris.c 		\
	top.h                   \
	x11.c                   \
	xmms2.c

# vi:set ts=8 sw=8 noet ai nocindent syntax=automake:
