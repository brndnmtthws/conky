/*
 * Auto-generated header for CMake.  See config.h.in if you need to modify the
 * original.
 */

#ifndef _conky_config_h_
#define _conky_config_h_

/* #undef DEBUG */

#define SYSTEM_NAME "Darwin"
#define PACKAGE_NAME "conky"
#define VERSION "1.10.7_pre"
#define SYSTEM_CONFIG_FILE "/etc/conky/conky.conf"
#define PACKAGE_LIBDIR "/usr/local/lib/conky"
#define DEFAULTNETDEV "eth0"
#define XDG_CONFIG_FILE "$HOME/.config/conky/conky.conf"
#define CONFIG_FILE "$HOME/.conkyrc"
#define LOCALE_DIR "/usr/local/share/locale"
#define MAX_USER_TEXT_DEFAULT 16384
#define DEFAULT_TEXT_BUFFER_SIZE 256
#define MAX_NET_INTERFACES 64
#define HTTPPORT 

#define BUILD_I18N 1

/* #undef HAVE_SYS_STATFS_H */
#define HAVE_SYS_PARAM_H 1
/* #undef HAVE_SYS_INOTIFY_H */
#define HAVE_DIRENT_H 1

#define HAVE_STRNDUP 1

/* #undef HAVE_FOPENCOOKIE */
/* #undef HAVE_FUNOPEN */

/* #undef HAVE_PIPE2 */
#define HAVE_O_CLOEXEC 1

#define HAVE_CLOCK_GETTIME 1

#define BUILD_X11 1

#define OWN_WINDOW 1

#define BUILD_XDAMAGE 1

#define BUILD_XINERAMA 1

#define BUILD_XFT 1

/* #undef BUILD_XSHAPE */

#define BUILD_ARGB 1

/* #undef BUILD_XDBE */

/* #undef BUILD_PORT_MONITORS */

/* #undef BUILD_AUDACIOUS */

/* #undef NEW_AUDACIOUS_FOUND */

#define BUILD_MPD 1

/* #undef BUILD_MYSQL */

#define BUILD_MOC 1

/* #undef BUILD_NVIDIA */

/* #undef BUILD_XMMS2 */

/* #undef BUILD_HDDTEMP */

/* #undef BUILD_EVE */

/* #undef BUILD_LIBXML2 */

/* #undef BUILD_CURL */

/* #undef BUILD_WEATHER_XOAP */
/* #undef BUILD_WEATHER_METAR */

#define XOAP_FILE ""

#define BUILD_IMLIB2 1

#define BUILD_MATH 1

#define BUILD_BUILTIN_CONFIG 1

#define BUILD_OLD_CONFIG 1

#define BUILD_NCURSES 1
/* #undef LEAKFREE_NCURSES */

#define BUILD_APCUPSD 1

#define BUILD_IOSTATS 1

/* #undef BUILD_WLAN */

/* #undef BUILD_ICAL */

/* #undef BUILD_IRC */

/* #undef BUILD_PULSEAUDIO */

/* #undef BUILD_IPV6 */

/* #undef BUILD_HTTP */

/* #undef BUILD_ICONV */

/* #undef BUILD_LUA_CAIRO */

/* #undef BUILD_LUA_IMLIB2 */

/* #undef BUILD_LUA_RSVG */

#if defined(BUILD_LUA_CAIRO) || defined(BUILD_LUA_IMLIB2) || defined(BUILD_LUA_RSVG)
#define BUILD_LUA_EXTRAS
#endif

/* #undef BUILD_IBM */

/* #undef BUILD_RSS */

/* #undef BUILD_CMUS */

/* #undef BUILD_JOURNAL */

#define HAVE_STATFS64 1
#ifndef HAVE_STATFS64
#define statfs64 statfs
#endif

#endif /* _conky_config_h_ */
