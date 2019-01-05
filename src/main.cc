/*
 *
 * Conky, a system monitor, based on torsmo
 *
 * Any original torsmo code is licensed under the BSD license
 *
 * All code written since the fork of torsmo is licensed under the GPL
 *
 * Please see COPYING for details
 *
 * Copyright (c) 2005-2019 Brenden Matthews, Philip Kovacs, et. al.
 *	(see AUTHORS)
 * All rights reserved.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <getopt.h>
#include <cstdio>
#include <iostream>

#include "build.h"
#include "config.h"
#include "conky.h"
#include "lua-config.hh"

#ifdef BUILD_X11
#include "x11.h"
#endif /* BUILD_X11 */

#ifdef BUILD_CURL
#include "ccurl_thread.h"
#endif /* BUILD_CURL */

#ifdef BUILD_BUILTIN_CONFIG
#include "defconfig.h"

#ifdef BUILD_OLD_CONFIG
#include "convertconf.h"
#endif /* BUILD_OLD_CONFIG */
#endif /* BUILD_BUILTIN_CONFIG */

static void print_version() {
  std::cout << _(PACKAGE_NAME " " VERSION " compiled " BUILD_DATE
                              " for " BUILD_ARCH
                              "\n"
                              "\nCompiled in features:\n\n"
                              "System config file: " SYSTEM_CONFIG_FILE
                              "\n"
                              "Package library path: " PACKAGE_LIBDIR "\n\n")
            << _("\n General:\n")
#ifdef HAVE_OPENMP
            << _("  * OpenMP\n")
#endif /* HAVE_OPENMP */
#ifdef BUILD_MATH
            << _("  * math\n")
#endif /* BUILD_MATH */
#ifdef BUILD_HDDTEMP
            << _("  * hddtemp\n")
#endif /* BUILD_HDDTEMP */
#ifdef BUILD_PORT_MONITORS
            << _("  * portmon\n")
#endif /* BUILD_PORT_MONITORS */
#ifdef BUILD_HTTP
            << _("  * HTTP\n")
#endif /* BUILD_HTTP */
#ifdef BUILD_IPV6
            << _("  * IPv6\n")
#endif /* BUILD_IPV6 */
#ifdef BUILD_IRC
            << _("  * IRC\n")
#endif
#ifdef BUILD_CURL
            << _("  * Curl\n")
#endif /* BUILD_CURL */
#ifdef BUILD_RSS
            << _("  * RSS\n")
#endif /* BUILD_RSS */
#ifdef BUILD_ICAL
            << _("  * ICal\n")
#endif /* BUILD_ICAL */
#ifdef BUILD_ICONV
            << _("  * iconv\n")
#endif /* BUILD_ICONV */
#ifdef BUILD_MYSQL
            << _("  * MySQL\n")
#endif /* BUILD_MYSQL */
#ifdef BUILD_WEATHER_METAR
            << _("  * Weather (METAR)\n")
#endif /* BUILD_WEATHER_METAR */
#ifdef BUILD_WLAN
            << _("  * wireless\n")
#endif /* BUILD_WLAN */
#ifdef BUILD_IBM
            << _("  * support for IBM/Lenovo notebooks\n")
#endif /* BUILD_IBM */
#ifdef BUILD_NVIDIA
            << _("  * nvidia\n")
#endif /* BUILD_NVIDIA */
#ifdef BUILD_BUILTIN_CONFIG
            << _("  * builtin default configuration\n")
#endif /* BUILD_BUILTIN_CONFIG */
#ifdef BUILD_OLD_CONFIG
            << _("  * old configuration syntax\n")
#endif /* BUILD_OLD_CONFIG */
#ifdef BUILD_IMLIB2
            << _("  * Imlib2\n")
#endif /* BUILD_IMLIB2 */
#ifdef HAVE_SOME_SOUNDCARD_H
            << _("  * OSS mixer support\n")
#endif /* HAVE_SOME_SOUNDCARD_H */
#ifdef BUILD_MIXER_ALSA
            << _("  * ALSA mixer support\n")
#endif /* BUILD_MIXER_ALSA */
#ifdef BUILD_APCUPSD
            << _("  * apcupsd\n")
#endif /* BUILD_APCUPSD */
#ifdef BUILD_IOSTATS
            << _("  * iostats\n")
#endif /* BUILD_IOSTATS */
#ifdef BUILD_NCURSES
            << _("  * ncurses\n")
#endif /* BUILD_NCURSES */
#ifdef BUILD_I18N
            << _("  * Internationalization support\n")
#endif
#ifdef BUILD_PULSEAUDIO
            << _("  * PulseAudio\n")
#endif /* BUIL_PULSEAUDIO */
#ifdef DEBUG
            << _("  * Debugging extensions\n")
#endif
#if defined BUILD_LUA_CAIRO || defined BUILD_LUA_IMLIB2 || BUILD_LUA_RSVG
            << _("\n Lua bindings:\n")
#endif
#ifdef BUILD_LUA_CAIRO
            << _("  * Cairo\n")
#endif /* BUILD_LUA_CAIRO */
#ifdef BUILD_LUA_IMLIB2
            << _("  * Imlib2\n")
#endif /* BUILD_LUA_IMLIB2 */
#ifdef BUILD_LUA_RSVG
            << _("  * RSVG\n")
#endif /* BUILD_LUA_RSVG */
#ifdef BUILD_X11
            << _(" X11:\n")
#ifdef BUILD_XDAMAGE
            << _("  * Xdamage extension\n")
#endif /* BUILD_XDAMAGE */
#ifdef BUILD_XINERAMA
            << _("  * Xinerama extension (virtual display)\n")
#endif /* BUILD_XINERAMA */
#ifdef BUILD_XSHAPE
            << _("  * Xshape extension (click through)\n")
#endif /* BUILD_XSHAPE */
#ifdef BUILD_XDBE
            << _("  * XDBE (double buffer extension)\n")
#endif /* BUILD_XDBE */
#ifdef BUILD_XFT
            << _("  * Xft\n")
#endif /* BUILD_XFT */
#ifdef BUILD_ARGB
            << _("  * ARGB visual\n")
#endif /* BUILD_ARGB */
#ifdef OWN_WINDOW
            << _("  * Own window\n")
#endif
#endif /* BUILD_X11 */
#if defined BUILD_AUDACIOUS || defined BUILD_CMUS || \
    defined BUILD_MPD || defined BUILD_MOC || defined BUILD_XMMS2
            << _("\n Music detection:\n")
#endif
#ifdef BUILD_AUDACIOUS
            << _("  * Audacious\n")
#endif /* BUILD_AUDACIOUS */
#ifdef BUILD_CMUS
            << _("  * CMUS\n")
#endif /* BUILD_CMUS */
#ifdef BUILD_MPD
            << _("  * MPD\n")
#endif /* BUILD_MPD */
#ifdef BUILD_MOC
            << _("  * MOC\n")
#endif /* BUILD_MOC */
#ifdef BUILD_XMMS2
            << _("  * XMMS2\n")
#endif /* BUILD_XMMS2 */
            << _("\n Default values:\n") << "  * Netdevice: " DEFAULTNETDEV "\n"
            << "  * Local configfile: " CONFIG_FILE "\n"
#ifdef BUILD_I18N
            << "  * Localedir: " LOCALE_DIR "\n"
#endif /* BUILD_I18N */
#ifdef BUILD_HTTP
            << "  * HTTP-port: " << HTTPPORT << "\n"
#endif /* BUILD_HTTP */
            << "  * Maximum netdevices: " << MAX_NET_INTERFACES << "\n"
            << "  * Maximum text size: " << MAX_USER_TEXT_DEFAULT << "\n"
            << "  * Size text buffer: " << DEFAULT_TEXT_BUFFER_SIZE << "\n";
}

static void print_help(const char *prog_name) {
  printf("Usage: %s [OPTION]...\n" PACKAGE_NAME
         " is a system monitor that renders text on desktop or to own "
         "transparent\n"
         "window. Command line options will override configurations defined in "
         "config\n"
         "file.\n"
         "   -v, --version             version\n"
         "   -q, --quiet               quiet mode\n"
         "   -D, --debug               increase debugging output, ie. -DD for "
         "more debugging\n"
         "   -c, --config=FILE         config file to load\n"
#ifdef BUILD_BUILTIN_CONFIG
         "   -C, --print-config        print the builtin default config to "
         "stdout\n"
         "                             e.g. 'conky -C > ~/.conkyrc' will "
         "create a new default config\n"
#endif
         "   -d, --daemonize           daemonize, fork to background\n"
         "   -h, --help                help\n"
#ifdef BUILD_X11
         "   -a, --alignment=ALIGNMENT text alignment on screen, "
         "{top,bottom,middle}_{left,right,middle}\n"
         "   -X, --display=DISPLAY     X11 display to use\n"
         "   -m, --xinerama-head=N     Xinerama monitor index (0=first)\n"
         "   -f, --font=FONT           font to use\n"
#ifdef OWN_WINDOW
         "   -o, --own-window          create own window to draw\n"
#endif
         "   -b, --double-buffer       double buffer (prevents flickering)\n"
         "   -w, --window-id=WIN_ID    window id to draw\n"
         "   -x X                      x position\n"
         "   -y Y                      y position\n"
#endif /* BUILD_X11 */
         "   -t, --text=TEXT           text to render, remember single quotes, "
         "like -t '$uptime'\n"
         "   -u, --interval=SECS       update interval\n"
         "   -i COUNT                  number of times to update " PACKAGE_NAME
         " (and quit)\n"
         "   -p, --pause=SECS          pause for SECS seconds at startup "
         "before doing anything\n",
         prog_name);
}

inline void reset_optind() {
#if defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || \
    defined(__OpenBSD__) || defined(__NetBSD__) || defined(__DragonFly__)
  optind = optreset = 1;
#else
  optind = 0;
#endif
}

int main(int argc, char **argv) {
#ifdef BUILD_I18N
  setlocale(LC_ALL, "");
  bindtextdomain(PACKAGE_NAME, LOCALE_DIR);
  textdomain(PACKAGE_NAME);
#endif
  argc_copy = argc;
  argv_copy = argv;
  g_sigterm_pending = 0;
  g_sighup_pending = 0;
  g_sigusr2_pending = 0;

#ifdef BUILD_CURL
  struct curl_global_initializer {
    curl_global_initializer() {
      if (curl_global_init(CURL_GLOBAL_ALL))
        NORM_ERR(
            "curl_global_init() failed, you may not be able to use curl "
            "variables");
    }
    ~curl_global_initializer() { curl_global_cleanup(); }
  };
  curl_global_initializer curl_global;
#endif

  /* handle command line parameters that don't change configs */
#ifdef BUILD_X11
  if (!setlocale(LC_CTYPE, "")) {
    NORM_ERR("Can't set the specified locale!\nCheck LANG, LC_CTYPE, LC_ALL.");
  }
#endif /* BUILD_X11 */
  while (1) {
    int c = getopt_long(argc, argv, getopt_string, longopts, nullptr);

    if (c == -1) { break; }

    switch (c) {
      case 'D':
        global_debug_level++;
        break;
      case 'v':
      case 'V':
        print_version();
        return EXIT_SUCCESS;
      case 'c':
        current_config = optarg;
        break;
      case 'q':
        if (freopen("/dev/null", "w", stderr) == nullptr)
          CRIT_ERR(nullptr, nullptr, "could not open /dev/null as stderr!");
        break;
      case 'h':
        print_help(argv[0]);
        return 0;
#ifdef BUILD_BUILTIN_CONFIG
      case 'C':
        std::cout << defconfig;
        return 0;
#endif
#ifdef BUILD_X11
      case 'w':
        window.window = strtol(optarg, nullptr, 0);
        break;
#endif /* BUILD_X11 */

      case '?':
        return EXIT_FAILURE;
    }
  }

  try {
    set_current_config();

    state = std::make_unique<lua::state>();

    conky::export_symbols(*state);

    setup_inotify();

    initialisation(argc, argv);

    first_pass = 0; /* don't ever call fork() again */

    main_loop();
  } catch (fork_throw &e) {
    return EXIT_SUCCESS;
  } catch (unknown_arg_throw &e) {
    return EXIT_FAILURE;
  } catch (obj_create_error &e) {
    std::cerr << e.what() << std::endl;
    clean_up(nullptr, nullptr);
    return EXIT_FAILURE;
  } catch (std::exception &e) {
    std::cerr << PACKAGE_NAME ": " << e.what() << std::endl;
    return EXIT_FAILURE;
  }

#if defined(__FreeBSD__) || defined(__FreeBSD_kernel__)
  kvm_close(kd);
#endif

#ifdef LEAKFREE_NCURSES
  _nc_free_and_exit(0);  // hide false memleaks
#endif
  return 0;
}
