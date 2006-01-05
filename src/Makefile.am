bin_PROGRAMS = conky

if BUILD_BMPX
bmpx = bmpx.c
endif

if BUILD_INFOPIPE
infopipe = infopipe.c
endif

if BUILD_SETI
seti = seti.c
endif

if BUILD_MPD
mpd = mpd.c libmpdclient.c
endif

if BUILD_LINUX
linux = linux.c top.c
PTHREAD_LIBS =  -lpthread
endif

if BUILD_SOLARIS
solaris = solaris.c
endif

if BUILD_FREEBSD
freebsd = freebsd.c
PTHREAD_LIBS =  -pthread
endif

if BUILD_NETBSD
netbsd = netbsd.c
endif

if BUILD_PORT_MONITORS
port_monitors = libtcp-portmon.h libtcp-portmon.c hash.h hash.c
endif

if BUILD_MLDONKEY
mldonkey = mldonkey.c
endif

if BUILD_X11
x11 = x11.c
endif

conky_SOURCES = common.c fs.c $(linux) mail.c mixer.c $(seti) $(mpd) $(solaris) $(freebsd) $(netbsd) $(port_monitors) conky.c conky.h $(x11) $(mldonkey) remoted.c remoted.h remotec.c remotec.h $(bmpx) $(infopipe)

AM_LDFLAGS = $(X11_LIBS) $(XFT_LIBS) $(CAIRO_LIBS) $(PTHREAD_LIBS) -lm

EXTRA_DIST = seti.c linux.c solaris.c freebsd.c netbsd.c mpd.c libmpdclient.c \
libmpdclient.h top.h mldonkey.c ftp.c ftp.h x11.c
