bin_PROGRAMS = conky

if BUILD_SETI
seti = src/seti.c
endif

if BUILD_MPD
mpd = src/mpd.c src/libmpdclient.c
endif

if BUILD_LINUX
linux = src/linux.c
endif

if BUILD_SOLARIS
solaris = src/solaris.c
endif

if BUILD_FREEBSD
freebsd = src/freebsd.c
endif

if BUILD_NETBSD
netbsd = src/netbsd.c
endif

if BUILD_CAIRO
cairo = src/cairo.c
endif

if BUILD_MLDONKEY
mldonkey = src/mldonkey.c
endif

if BUILD_X11
x11 = src/x11.c
endif

conky_SOURCES = src/common.c src/fs.c src/top.c $(linux) src/mail.c src/mixer.c $(seti) $(mpd) \
	$(solaris) $(freebsd) $(netbsd) $(cairo) src/conky.c src/conky.h $(x11) \
	$(mldonkey) src/remoted.c src/remoted.h src/remotec.c src/remotec.h

conky_LDFLAGS = $(X11_LIBS) $(XFT_LIBS) $(CAIRO_LIBS)

conky_LDADD = $(METAR_LIBS)

man_MANS = doc/conky.1

LIBS = -lm -lpthread

# conky.1 is in EXTRA_DIST in case that someone doesn't have help2man
EXTRA_DIST = conkyrc.sample doc/conky.1 \
             app-admin/conky/conky-1.2.ebuild app-admin/conky/conky-1.9999.ebuild

EXTRA_conky_SOURCES = src/seti.c src/linux.c src/solaris.c src/freebsd.c src/netbsd.c src/mpd.c src/libmpdclient.c \
src/cairo.c src/libmpdclient.h src/top.h src/mldonkey.c src/ftp.c src/ftp.h src/x11.c
