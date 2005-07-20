bin_PROGRAMS = conky

if BUILD_SETI
seti = seti.c
endif

if BUILD_MPD
mpd = mpd.c libmpdclient.c
endif

if BUILD_LINUX
linux = linux.c
endif

if BUILD_SOLARIS
solaris = solaris.c
endif

if BUILD_FREEBSD
freebsd = freebsd.c
endif

if BUILD_NETBSD
netbsd = netbsd.c
endif

if BUILD_CAIRO
cairo = cairo.c
endif

if BUILD_METAR
metar = ftp.c metarinfo.c
METAR_LIBS = -lmetar
endif

if BUILD_MLDONKEY
mldonkey = mldonkey.c
endif

conky_SOURCES = common.c fs.c top.c $(linux) mail.c mixer.c $(seti) $(mpd) \
$(solaris) $(freebsd) $(netbsd) $(cairo) conky.c conky.h x11.c $(metar) $(mldonkey)

conky_LDFLAGS = $(X11_LIBS) $(XFT_LIBS) $(CAIRO_LIBS)

conky_LDADD = $(METAR_LIBS)

man_MANS = conky.1

LIBS = -lm -lpthread -lX11 -lXext

if HAVE_HELP2MAN
conky.1: conky conky.x
	help2man -h -h -v -V -i conky.x -o conky.1 -N ./conky
endif

# conky.1 is in EXTRA_DIST in case that someone doesn't have help2man
EXTRA_DIST = changelog.html conkyrc.sample conky.1 conky.x \
             changelog2html.py app-admin/conky/conky-1.2.ebuild

EXTRA_conky_SOURCES = seti.c linux.c solaris.c freebsd.c netbsd.c mpd.c libmpdclient.c \
cairo.c libmpdclient.h top.h mldonkey.c ftp.c metarinfo.c ftp.h metarinfo.h