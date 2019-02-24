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
 * Copyright (c) 2004, Hannu Saransaari and Lauri Hakkarainen
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

#include "conky.h"
#include <algorithm>
#include <cerrno>
#include <climits>
#include <clocale>
#include <cmath>
#include <cstdarg>
#include <ctime>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>
#include "common.h"
#include "config.h"
#include "text_object.h"
#ifdef HAVE_DIRENT_H
#include <dirent.h>
#endif /* HAVE_DIRENT_H */
#include <sys/param.h>
#include <sys/time.h>
#ifdef HAVE_SYS_INOTIFY_H
#include <sys/inotify.h>
#endif /* HAVE_SYS_INOTIFY_H */
#ifdef BUILD_X11
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wvariadic-macros"
#include <X11/Xutil.h>
#pragma GCC diagnostic pop
#include "x11.h"
#ifdef BUILD_XDAMAGE
#include <X11/extensions/Xdamage.h>
#endif
#ifdef BUILD_IMLIB2
#include "imlib2.h"
#endif /* BUILD_IMLIB2 */
#endif /* BUILD_X11 */
#ifdef BUILD_NCURSES
#include <ncurses.h>
#endif
#include <fcntl.h>
#include <getopt.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <sys/types.h>
#if defined BUILD_RSS
#include <libxml/parser.h>
#endif
#ifdef BUILD_CURL
#include <curl/curl.h>
#endif

/* local headers */
#include "colours.h"
#include "core.h"
#include "diskio.h"
#include "exec.h"
#ifdef BUILD_X11
#include "fonts.h"
#endif
#include "fs.h"
#ifdef BUILD_ICONV
#include "iconv_tools.h"
#endif
#include "llua.h"
#include "logging.h"
#include "mail.h"
#include "nc.h"
#include "net_stat.h"
#include "temphelper.h"
#include "template.h"
#include "timeinfo.h"
#include "top.h"
#ifdef BUILD_MYSQL
#include "mysql.h"
#endif /* BUILD_MYSQL */
#ifdef BUILD_NVIDIA
#include "nvidia.h"
#endif
#ifdef BUILD_CURL
#include "ccurl_thread.h"
#endif /* BUILD_CURL */
#ifdef BUILD_WEATHER_METAR
#include "weather.h"
#endif /* BUILD_WEATHER_METAR */

#include "lua-config.hh"
#include "setting.hh"

/* check for OS and include appropriate headers */
#if defined(__linux__)
#include "linux.h"
#elif defined(__FreeBSD__) || defined(__FreeBSD_kernel__)
#include "freebsd.h"
#elif defined(__DragonFly__)
#include "dragonfly.h"
#elif defined(__OpenBSD__)
#include "openbsd.h"
#endif /* __OpenBSD__ */
#ifdef BUILD_HTTP
#include <microhttpd.h>
#endif /* BUILD_HTTP */

#if defined(__FreeBSD_kernel__)
#include <bsd/bsd.h>
#endif

#ifdef BUILD_OLD_CONFIG
#include "convertconf.h"
#endif /* BUILD_OLD_CONFIG */

#ifdef BUILD_BUILTIN_CONFIG
#include "defconfig.h"

namespace {
const char builtin_config_magic[] = "==builtin==";
}  // namespace
#endif /* BUILD_BUILTIN_CONFIG */

#ifndef S_ISSOCK
#define S_ISSOCK(x) ((x & S_IFMT) == S_IFSOCK)
#endif

#define MAX_IF_BLOCK_DEPTH 5

//#define SIGNAL_BLOCKING
#undef SIGNAL_BLOCKING

/* debugging level, used by logging.h */
int global_debug_level = 0;

/* disable inotify auto reload feature if desired */
static conky::simple_config_setting<bool> disable_auto_reload(
    "disable_auto_reload", false, false);

/* two strings for internal use */
static char *tmpstring1, *tmpstring2;

#ifdef BUILD_NCURSES
extern WINDOW *ncurses_window;
#endif

enum spacer_state { NO_SPACER = 0, LEFT_SPACER, RIGHT_SPACER };
template <>
conky::lua_traits<spacer_state>::Map conky::lua_traits<spacer_state>::map = {
    {"none", NO_SPACER}, {"left", LEFT_SPACER}, {"right", RIGHT_SPACER}};
static conky::simple_config_setting<spacer_state> use_spacer("use_spacer",
                                                             NO_SPACER, false);

/* variables holding various config settings */
static conky::simple_config_setting<bool> short_units("short_units", false,
                                                      true);
static conky::simple_config_setting<bool> format_human_readable(
    "format_human_readable", true, true);

conky::simple_config_setting<bool> out_to_stdout("out_to_console",
// Default value is false, unless we are building without X
#ifdef BUILD_X11
                                                 false,
#else
                                                 true,
#endif
                                                 false);
static conky::simple_config_setting<bool> out_to_stderr("out_to_stderr", false,
                                                        false);

int top_cpu, top_mem, top_time;
#ifdef BUILD_IOSTATS
int top_io;
#endif
int top_running;
static conky::simple_config_setting<bool> extra_newline("extra_newline", false,
                                                        false);

/* Update interval */
conky::range_config_setting<double> update_interval(
    "update_interval", 0.0, std::numeric_limits<double>::infinity(), 3.0, true);
conky::range_config_setting<double> update_interval_on_battery(
    "update_interval_on_battery", 0.0, std::numeric_limits<double>::infinity(),
    NOBATTERY, true);
conky::simple_config_setting<std::string> detect_battery("detect_battery",
                                                         std::string("BAT0"),
                                                         false);
static bool on_battery = false;

double active_update_interval() {
  return (on_battery ? update_interval_on_battery : update_interval)
      .get(*state);
}

void music_player_interval_setting::lua_setter(lua::state &l, bool init) {
  lua::stack_sentry s(l, -2);

  if (l.isnil(-2)) {
    l.checkstack(1);
    l.pushnumber(update_interval.get(l));
    l.replace(-3);
  }

  Base::lua_setter(l, init);

  ++s;
}

music_player_interval_setting music_player_interval;

void *global_cpu = nullptr;
static conky::range_config_setting<unsigned int> max_text_width(
    "max_text_width", 0, std::numeric_limits<unsigned int>::max(), 0, true);

#if defined(__FreeBSD__) || defined(__FreeBSD_kernel__)
extern kvm_t *kd;
#endif

/* prototypes for internally used functions */
static void signal_handler(int /*sig*/);
static void reload_config();

static const char *suffixes[] = {_nop("B"),   _nop("KiB"), _nop("MiB"),
                                 _nop("GiB"), _nop("TiB"), _nop("PiB"),
                                 ""};

#ifdef BUILD_X11

static void X11_create_window();

struct _x11_stuff_s {
  Region region;
#ifdef BUILD_XDAMAGE
  Damage damage;
  XserverRegion region2, part;
  int event_base, error_base;
#endif
} x11_stuff;

/* text size */

static int text_start_x, text_start_y;   /* text start position in window */
static int text_offset_x, text_offset_y; /* offset for start position */
static int text_width = 1,
           text_height = 1; /* initially 1 so no zero-sized window is created */

#endif /* BUILD_X11 */

/* struct that has all info to be shared between
 * instances of the same text object */
struct information info;

/* path to config file */
std::string current_config;

/* set to 1 if you want all text to be in uppercase */
static conky::simple_config_setting<bool> stuff_in_uppercase("uppercase", false,
                                                             true);

/* Run how many times? */
static conky::range_config_setting<unsigned long> total_run_times(
    "total_run_times", 0, std::numeric_limits<unsigned long>::max(), 0, true);

/* fork? */
static conky::simple_config_setting<bool> fork_to_background("background",
                                                             false, false);

/* set to 0 after the first time conky is run, so we don't fork again after the
 * first forking */
int first_pass = 1;
int argc_copy;
char **argv_copy;

conky::range_config_setting<int> cpu_avg_samples("cpu_avg_samples", 1, 14, 2,
                                                 true);
conky::range_config_setting<int> net_avg_samples("net_avg_samples", 1, 14, 2,
                                                 true);
conky::range_config_setting<int> diskio_avg_samples("diskio_avg_samples", 1, 14,
                                                    2, true);

/* filenames for output */
static conky::simple_config_setting<std::string> overwrite_file(
    "overwrite_file", std::string(), true);
static FILE *overwrite_fpointer = nullptr;
static conky::simple_config_setting<std::string> append_file("append_file",
                                                             std::string(),
                                                             true);
static FILE *append_fpointer = nullptr;

#ifdef BUILD_HTTP
std::string webpage;
struct MHD_Daemon *httpd;
static conky::simple_config_setting<bool> http_refresh("http_refresh", false,
                                                       true);

int sendanswer(void *cls, struct MHD_Connection *connection, const char *url,
               const char *method, const char *version, const char *upload_data,
               size_t *upload_data_size, void **con_cls) {
  struct MHD_Response *response = MHD_create_response_from_buffer(
      webpage.length(), (void *)webpage.c_str(), MHD_RESPMEM_PERSISTENT);
  int ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
  MHD_destroy_response(response);
  if (cls || url || method || version || upload_data || upload_data_size ||
      con_cls) {}  // make compiler happy
  return ret;
}

class out_to_http_setting : public conky::simple_config_setting<bool> {
  typedef conky::simple_config_setting<bool> Base;

 protected:
  virtual void lua_setter(lua::state &l, bool init) {
    lua::stack_sentry s(l, -2);

    Base::lua_setter(l, init);

    if (init && do_convert(l, -1).first) {
      httpd = MHD_start_daemon(MHD_USE_SELECT_INTERNALLY, HTTPPORT, nullptr,
                               NULL, &sendanswer, nullptr, MHD_OPTION_END);
    }

    ++s;
  }

  virtual void cleanup(lua::state &l) {
    lua::stack_sentry s(l, -1);

    if (do_convert(l, -1).first) {
      MHD_stop_daemon(httpd);
      httpd = nullptr;
    }

    l.pop();
  }

 public:
  out_to_http_setting() : Base("out_to_http", false, false) {}
};
static out_to_http_setting out_to_http;
#endif /* BUILD_HTTP */

#ifdef BUILD_X11

static conky::simple_config_setting<bool> show_graph_scale("show_graph_scale",
                                                           false, false);
static conky::simple_config_setting<bool> show_graph_range("show_graph_range",
                                                           false, false);

/* Position on the screen */
static conky::simple_config_setting<int> gap_x("gap_x", 5, true);
static conky::simple_config_setting<int> gap_y("gap_y", 60, true);

/* border */
static conky::simple_config_setting<bool> draw_borders("draw_borders", false,
                                                       false);
static conky::simple_config_setting<bool> draw_graph_borders(
    "draw_graph_borders", true, false);

conky::range_config_setting<char> stippled_borders(
    "stippled_borders", 0, std::numeric_limits<char>::max(), 0, true);

static conky::simple_config_setting<bool> draw_shades("draw_shades", true,
                                                      false);
static conky::simple_config_setting<bool> draw_outline("draw_outline", false,
                                                       false);

#ifdef OWN_WINDOW
/* fixed size/pos is set if wm/user changes them */
static int fixed_size = 0, fixed_pos = 0;
#endif

static conky::range_config_setting<int> minimum_height(
    "minimum_height", 0, std::numeric_limits<int>::max(), 5, true);
static conky::range_config_setting<int> minimum_width(
    "minimum_width", 0, std::numeric_limits<int>::max(), 5, true);
static conky::range_config_setting<int> maximum_width(
    "maximum_width", 0, std::numeric_limits<int>::max(), 0, true);

static bool isutf8(const char *envvar) {
  char *s = getenv(envvar);
  if (s != nullptr) {
    std::string temp = s;
    std::transform(temp.begin(), temp.end(), temp.begin(), ::tolower);
    if ((temp.find("utf-8") != std::string::npos) ||
        (temp.find("utf8") != std::string::npos)) {
      return true;
    }
  }
  return false;
}

/* UTF-8 */
conky::simple_config_setting<bool> utf8_mode("override_utf8_locale",
                                             isutf8("LC_ALL") ||
                                                 isutf8("LC_CTYPE") ||
                                                 isutf8("LANG"),
                                             false);

#endif /* BUILD_X11 */

/* maximum size of config TEXT buffer, i.e. below TEXT line. */
conky::range_config_setting<unsigned int> max_user_text(
    "max_user_text", 47, std::numeric_limits<unsigned int>::max(),
    MAX_USER_TEXT_DEFAULT, false);

/* maximum size of individual text buffers, ie $exec buffer size */
conky::range_config_setting<unsigned int> text_buffer_size(
    "text_buffer_size", DEFAULT_TEXT_BUFFER_SIZE,
    std::numeric_limits<unsigned int>::max(), DEFAULT_TEXT_BUFFER_SIZE, false);

/* pad percentages to decimals? */
static conky::simple_config_setting<int> pad_percents("pad_percents", 0, false);

static char *global_text = nullptr;

char *get_global_text() { return global_text; }

long global_text_lines;

static int total_updates;
static int updatereset;

std::unique_ptr<lua::state> state;

void set_updatereset(int i) { updatereset = i; }

int get_updatereset() { return updatereset; }

int get_total_updates() { return total_updates; }

int calc_text_width(const char *s) {
  size_t slen = strlen(s);

#ifdef BUILD_X11
  if (!out_to_x.get(*state)) {
#endif /* BUILD_X11 */
    return slen;
#ifdef BUILD_X11
  }
#ifdef BUILD_XFT
  if (use_xft.get(*state)) {
    XGlyphInfo gi;

    if (utf8_mode.get(*state)) {
      XftTextExtentsUtf8(display, fonts[selected_font].xftfont,
                         reinterpret_cast<const FcChar8 *>(s), slen, &gi);
    } else {
      XftTextExtents8(display, fonts[selected_font].xftfont,
                      reinterpret_cast<const FcChar8 *>(s), slen, &gi);
    }
    return gi.xOff;
  }
#endif /* BUILD_XFT */

  return XTextWidth(fonts[selected_font].font, s, slen);

#endif /* BUILD_X11 */
}

/* formatted text to render on screen, generated in generate_text(),
 * drawn in draw_stuff() */

static char *text_buffer;

/* quite boring functions */

static inline void for_each_line(char *b, int f(char *, int)) {
  char *ps, *pe;
  int special_index = 0; /* specials index */

  if (b == nullptr) { return; }
  for (ps = b, pe = b; *pe != 0; pe++) {
    if (*pe == '\n') {
      *pe = '\0';
      special_index = f(ps, special_index);
      *pe = '\n';
      ps = pe + 1;
    }
  }

  if (ps < pe) { f(ps, special_index); }
}

static void convert_escapes(char *buf) {
  char *p = buf, *s = buf;

  while (*s != 0) {
    if (*s == '\\') {
      s++;
      if (*s == 'n') {
        *p++ = '\n';
      } else if (*s == '\\') {
        *p++ = '\\';
      }
      s++;
    } else {
      *p++ = *s++;
    }
  }
  *p = '\0';
}

/* Prints anything normally printed with snprintf according to the current value
 * of use_spacer.  Actually slightly more flexible than snprintf, as you can
 * safely specify the destination buffer as one of your inputs.  */
int spaced_print(char *buf, int size, const char *format, int width, ...) {
  int len = 0;
  va_list argp;
  char *tempbuf;

  if (size < 1) { return 0; }
  tempbuf = new char[size];

  // Passes the varargs along to vsnprintf
  va_start(argp, width);
  vsnprintf(tempbuf, size, format, argp);
  va_end(argp);

  switch (use_spacer.get(*state)) {
    case NO_SPACER:
      len = snprintf(buf, size, "%s", tempbuf);
      break;
    case LEFT_SPACER:
      len = snprintf(buf, size, "%*s", width, tempbuf);
      break;
    case RIGHT_SPACER:
      len = snprintf(buf, size, "%-*s", width, tempbuf);
      break;
  }
  delete[] tempbuf;
  return len;
}

/* print percentage values
 *
 * - i.e., unsigned values between 0 and 100
 * - respect the value of pad_percents */
int percent_print(char *buf, int size, unsigned value) {
  return spaced_print(buf, size, "%u", pad_percents.get(*state), value);
}

/* converts from bytes to human readable format (K, M, G, T)
 *
 * The algorithm always divides by 1024, as unit-conversion of byte
 * counts suggests. But for output length determination we need to
 * compare with 1000 here, as we print in decimal form. */
void human_readable(long long num, char *buf, int size) {
  const char **suffix = suffixes;
  float fnum;
  int precision;
  int width;
  const char *format;

  /* Possibly just output as usual, for example for stdout usage */
  if (!format_human_readable.get(*state)) {
    spaced_print(buf, size, "%lld", 6, num);
    return;
  }
  if (short_units.get(*state)) {
    width = 5;
    format = "%.*f%.1s";
  } else {
    width = 7;
    format = "%.*f%-.3s";
  }

  if (llabs(num) < 1000LL) {
    spaced_print(buf, size, format, width, 0, static_cast<float>(num),
                 _(*suffix));
    return;
  }

  while (llabs(num / 1024) >= 1000LL && (**(suffix + 2) != 0)) {
    num /= 1024;
    suffix++;
  }

  suffix++;
  fnum = num / 1024.0;

  /* fnum should now be < 1000, so looks like 'AAA.BBBBB'
   *
   * The goal is to always have a significance of 3, by
   * adjusting the decimal part of the number. Sample output:
   *  123MiB
   * 23.4GiB
   * 5.12B
   * so the point of alignment resides between number and unit. The
   * upside of this is that there is minimal padding necessary, though
   * there should be a way to make alignment take place at the decimal
   * dot (then with fixed width decimal part).
   *
   * Note the repdigits below: when given a precision value, printf()
   * rounds the float to it, not just cuts off the remaining digits. So
   * e.g. 99.95 with a precision of 1 gets 100.0, which again should be
   * printed with a precision of 0. Yay. */

  precision = 0; /* print 100-999 without decimal part */
  if (fnum < 99.95) { precision = 1; /* print 10-99 with one decimal place */ }
  if (fnum < 9.995) { precision = 2; /* print 0-9 with two decimal places */ }

  spaced_print(buf, size, format, width, precision, fnum, _(*suffix));
}

/* global object list root element */
static struct text_object global_root_object;

static long current_text_color;

void set_current_text_color(long colour) { current_text_color = colour; }

long get_current_text_color() { return current_text_color; }

static void extract_variable_text(const char *p) {
  free_text_objects(&global_root_object);
  delete_block_and_zero(tmpstring1);
  delete_block_and_zero(tmpstring2);
  delete_block_and_zero(text_buffer);

  extract_variable_text_internal(&global_root_object, p);
}

void parse_conky_vars(struct text_object *root, const char *txt, char *p,
                      int p_max_size) {
  extract_variable_text_internal(root, txt);
  generate_text_internal(p, p_max_size, *root);
}

/* IFBLOCK jumping algorithm
 *
 * This is easier as it looks like:
 * - each IF checks it's condition
 *   - on FALSE: jump
 *   - on TRUE: don't care
 * - each ELSE jumps unconditionally
 * - each ENDIF is silently being ignored
 *
 * Why this works (or: how jumping works):
 * Jumping means to overwrite the "obj" variable of the loop and set it to the
 * target (i.e. the corresponding ELSE or ENDIF). After that, the for-loop does
 * the rest: as regularly, "obj" is being updated to point to obj->next, so
 * object parsing continues right after the corresponding ELSE or ENDIF. This
 * means that if we find an ELSE, it's corresponding IF must not have jumped,
 * so we need to jump always. If we encounter an ENDIF, it's corresponding IF
 * or ELSE has not jumped, and there is nothing to do.
 */
void generate_text_internal(char *p, int p_max_size, struct text_object root) {
  struct text_object *obj;
  size_t a;

  if (p == nullptr) { return; }

#ifdef BUILD_ICONV
  char *buff_in;

  buff_in = new char[p_max_size];
  memset(buff_in, 0, p_max_size);
#endif /* BUILD_ICONV */

  p[0] = 0;
  obj = root.next;
  while ((obj != nullptr) && p_max_size > 0) {
    /* check callbacks for existence and act accordingly */
    if (obj->callbacks.print != nullptr) {
      (*obj->callbacks.print)(obj, p, p_max_size);
    } else if (obj->callbacks.iftest != nullptr) {
      if ((*obj->callbacks.iftest)(obj) == 0) {
        DBGP2("jumping");
        if (obj->ifblock_next != nullptr) { obj = obj->ifblock_next; }
      }
    } else if (obj->callbacks.barval != nullptr) {
      new_bar(obj, p, p_max_size, (*obj->callbacks.barval)(obj));
    } else if (obj->callbacks.gaugeval != nullptr) {
      new_gauge(obj, p, p_max_size, (*obj->callbacks.gaugeval)(obj));
#ifdef BUILD_X11
    } else if (obj->callbacks.graphval != nullptr) {
      new_graph(obj, p, p_max_size, (*obj->callbacks.graphval)(obj));
#endif /* BUILD_X11 */
    } else if (obj->callbacks.percentage != nullptr) {
      percent_print(p, p_max_size, (*obj->callbacks.percentage)(obj));
    }

    a = strlen(p);
#ifdef BUILD_ICONV
    iconv_convert(&a, buff_in, p, p_max_size);
#endif /* BUILD_ICONV */
    p += a;
    p_max_size -= a;
    (*p) = 0;

    obj = obj->next;
  }
#ifdef BUILD_X11
  /* load any new fonts we may have had */
  load_fonts(utf8_mode.get(*state));
#endif /* BUILD_X11 */
#ifdef BUILD_ICONV
  delete[] buff_in;
#endif /* BUILD_ICONV */
}

void evaluate(const char *text, char *p, int p_max_size) {
  struct text_object subroot {};

  parse_conky_vars(&subroot, text, p, p_max_size);
  DBGP2("evaluated '%s' to '%s'", text, p);

  free_text_objects(&subroot);
}

double current_update_time, next_update_time, last_update_time;

static void generate_text() {
  char *p;
  unsigned int i, j, k;
  special_count = 0;

  current_update_time = get_time();

  /* clears netstats info, calls conky::run_all_callbacks(), and changes
   * some info.mem entries */
  update_stuff();

  /* populate the text buffer; generate_text_internal() iterates through
   * global_root_object (an instance of the text_object struct) and calls
   * any callbacks that were set on startup by construct_text_object(). */
  p = text_buffer;

  generate_text_internal(p, max_user_text.get(*state), global_root_object);
  unsigned int mw = max_text_width.get(*state);
  unsigned int tbs = text_buffer_size.get(*state);
  if (mw > 0) {
    for (i = 0, j = 0; p[i] != 0; i++) {
      if (p[i] == '\n') {
        j = 0;
      } else if (j == mw) {
        k = i + strlen(p + i) + 1;
        if (k < tbs) {
          while (k != i) {
            p[k] = p[k - 1];
            k--;
          }
          p[k] = '\n';
          j = 0;
        } else {
          NORM_ERR(
              "The end of the text_buffer is reached, increase "
              "\"text_buffer_size\"");
        }
      } else {
        j++;
      }
    }
  }

  if (stuff_in_uppercase.get(*state)) {
    char *tmp_p;

    tmp_p = text_buffer;
    while (*tmp_p != 0) {
      *tmp_p = toupper(static_cast<unsigned char>(*tmp_p));
      tmp_p++;
    }
  }

  double ui = active_update_interval();
  double time = get_time();
  next_update_time += ui;
  if (next_update_time < time || next_update_time > time + ui) {
    next_update_time = time - fmod(time, ui) + ui;
  }
  last_update_time = current_update_time;
  total_updates++;
}

int get_string_width(const char *s) { return *s != 0 ? calc_text_width(s) : 0; }

#ifdef BUILD_X11
static inline int get_border_total() {
  return border_inner_margin.get(*state) + border_outer_margin.get(*state) +
         border_width.get(*state);
}

static int get_string_width_special(char *s, int special_index) {
  char *p, *final;
  special_t *current = specials;
  int idx = 1;
  int width = 0;
  long i;

  if (s == nullptr) { return 0; }

  if (!out_to_x.get(*state)) { return strlen(s); }

  p = strndup(s, text_buffer_size.get(*state));
  final = p;

  for (i = 0; i < special_index; i++) { current = current->next; }
  for (i = 0; i < idx; i++) { current = current->next; }

  while (*p != 0) {
    if (*p == SPECIAL_CHAR) {
      /* shift everything over by 1 so that the special char
       * doesn't mess up the size calculation */
      for (i = 0; i < static_cast<long>(strlen(p)); i++) {
        *(p + i) = *(p + i + 1);
      }
      if (current->type == GRAPH || current->type == GAUGE ||
          current->type == BAR) {
        width += current->width;
      }
      if (current->type == FONT) {
        // put all following text until the next fontchange/stringend in
        // influenced_by_font but do not include specials
        char *influenced_by_font = strdup(p);
        special_t *current_after_font = current;
        for (i = 0; influenced_by_font[i] != 0; i++) {
          if (influenced_by_font[i] == SPECIAL_CHAR) {
            // remove specials and stop at fontchange
            current_after_font = current_after_font->next;
            if (current_after_font->type == FONT) {
              influenced_by_font[i] = 0;
              break;
            }
            {
              memmove(&influenced_by_font[i], &influenced_by_font[i + 1],
                      strlen(&influenced_by_font[i + 1]) + 1);
            }
          }
        }
        // add the length of influenced_by_font in the new font to width
        int orig_font = selected_font;
        selected_font = current->font_added;
        width += calc_text_width(influenced_by_font);
        selected_font = orig_font;
        free(influenced_by_font);
        // make sure there chars counted in the new font are not again counted
        // in the old font
        int specials_skipped = 0;
        while (i > 0) {
          if (p[specials_skipped] != 1) {
            memmove(&p[specials_skipped], &p[specials_skipped + 1],
                    strlen(&p[specials_skipped + 1]) + 1);
          } else {
            specials_skipped++;
          }
          i--;
        }
      }
      idx++;
      current = current->next;
    } else {
      p++;
    }
  }
  if (strlen(final) > 1) { width += calc_text_width(final); }
  free(final);
  return width;
}

static int text_size_updater(char *s, int special_index);

int last_font_height;
static void update_text_area() {
  int x = 0, y = 0;

  if (!out_to_x.get(*state)) { return; }
  /* update text size if it isn't fixed */
#ifdef OWN_WINDOW
  if (fixed_size == 0)
#endif
  {
    text_width = minimum_width.get(*state);
    text_height = 0;
    last_font_height = font_height();
    for_each_line(text_buffer, text_size_updater);
    text_width += 1;
    if (text_height < minimum_height.get(*state)) {
      text_height = minimum_height.get(*state);
    }
    int mw = maximum_width.get(*state);
    if (text_width > mw && mw > 0) { text_width = mw; }
  }

  alignment align = text_alignment.get(*state);
  /* get text position on workarea */
  switch (align) {
    case TOP_LEFT:
    case TOP_RIGHT:
    case TOP_MIDDLE:
      y = workarea[1] + gap_y.get(*state);
      break;

    case BOTTOM_LEFT:
    case BOTTOM_RIGHT:
    case BOTTOM_MIDDLE:
    default:
      y = workarea[3] - text_height - gap_y.get(*state);
      break;

    case MIDDLE_LEFT:
    case MIDDLE_RIGHT:
    case MIDDLE_MIDDLE:
      y = workarea[1] + (workarea[3] - workarea[1]) / 2 - text_height / 2 -
          gap_y.get(*state);
      break;
  }
  switch (align) {
    case TOP_LEFT:
    case BOTTOM_LEFT:
    case MIDDLE_LEFT:
    default:
      x = workarea[0] + gap_x.get(*state);
      break;

    case TOP_RIGHT:
    case BOTTOM_RIGHT:
    case MIDDLE_RIGHT:
      x = workarea[2] - text_width - gap_x.get(*state);
      break;

    case TOP_MIDDLE:
    case BOTTOM_MIDDLE:
    case MIDDLE_MIDDLE:
      x = workarea[0] + (workarea[2] - workarea[0]) / 2 - text_width / 2 -
          gap_x.get(*state);
      break;
  }
#ifdef OWN_WINDOW
  if (align == NONE) {  // Let the WM manage the window
    x = window.x;
    y = window.y;

    fixed_pos = 1;
    fixed_size = 1;
  }
#endif /* OWN_WINDOW */
#ifdef OWN_WINDOW

  if (own_window.get(*state) && (fixed_pos == 0)) {
    int border_total = get_border_total();
    text_start_x = text_start_y = border_total;
    window.x = x - border_total;
    window.y = y - border_total;
  } else
#endif
  {
    text_start_x = x;
    text_start_y = y;
  }
  /* update lua window globals */
  llua_update_window_table(text_start_x, text_start_y, text_width, text_height);
}

/* drawing stuff */

static int cur_x, cur_y; /* current x and y for drawing */
#endif
// draw_mode also without BUILD_X11 because we only need to print to stdout with
// FG
static int draw_mode; /* FG, BG or OUTLINE */
#ifdef BUILD_X11
static long current_color;

static int text_size_updater(char *s, int special_index) {
  int w = 0;
  char *p;
  special_t *current = specials;

  for (int i = 0; i < special_index; i++) { current = current->next; }

  if (!out_to_x.get(*state)) { return 0; }
  /* get string widths and skip specials */
  p = s;
  while (*p != 0) {
    if (*p == SPECIAL_CHAR) {
      *p = '\0';
      w += get_string_width(s);
      *p = SPECIAL_CHAR;

      if (current->type == BAR || current->type == GAUGE ||
          current->type == GRAPH) {
        w += current->width;
        if (current->height > last_font_height) {
          last_font_height = current->height;
          last_font_height += font_height();
        }
      } else if (current->type == OFFSET) {
        if (current->arg > 0) { w += current->arg; }
      } else if (current->type == VOFFSET) {
        last_font_height += current->arg;
      } else if (current->type == GOTO) {
        if (current->arg > cur_x) { w = static_cast<int>(current->arg); }
      } else if (current->type == TAB) {
        int start = current->arg;
        int step = current->width;

        if ((step == 0) || step < 0) { step = 10; }
        w += step - (cur_x - text_start_x - start) % step;
      } else if (current->type == FONT) {
        selected_font = current->font_added;
        if (font_height() > last_font_height) {
          last_font_height = font_height();
        }
      }

      special_index++;
      current = current->next;
      s = p + 1;
    }
    p++;
  }

  w += get_string_width(s);

  if (w > text_width) { text_width = w; }
  int mw = maximum_width.get(*state);
  if (text_width > mw && mw > 0) { text_width = mw; }

  text_height += last_font_height;
  last_font_height = font_height();
  return special_index;
}
#endif /* BUILD_X11 */

static inline void set_foreground_color(long c) {
#ifdef BUILD_X11
  if (out_to_x.get(*state)) {
#ifdef BUILD_ARGB
    if (have_argb_visual) {
      current_color = c | (own_window_argb_value.get(*state) << 24);
    } else {
#endif /* BUILD_ARGB */
      current_color = c;
#ifdef BUILD_ARGB
    }
#endif /* BUILD_ARGB */
    XSetForeground(display, window.gc, current_color);
  }
#endif /* BUILD_X11 */
#ifdef BUILD_NCURSES
  if (out_to_ncurses.get(*state)) { attron(COLOR_PAIR(c)); }
#endif /* BUILD_NCURSES */
  UNUSED(c);
}

std::string string_replace_all(std::string original, const std::string &oldpart,
                               const std::string &newpart,
                               std::string::size_type start) {
  std::string::size_type i = start;
  int oldpartlen = oldpart.length();
  while (1) {
    i = original.find(oldpart, i);
    if (i == std::string::npos) { break; }
    original.replace(i, oldpartlen, newpart);
  }
  return original;
}

static void draw_string(const char *s) {
  int i;
  int i2;
  int pos;
#ifdef BUILD_X11
  int width_of_s;
#endif /* BUILD_X11 */
  int max = 0;
  int added;

  if (s[0] == '\0') { return; }

#ifdef BUILD_X11
  width_of_s = get_string_width(s);
#endif /* BUILD_X11 */
  if (out_to_stdout.get(*state) && draw_mode == FG) {
    printf("%s\n", s);
    if (extra_newline.get(*state)) { fputc('\n', stdout); }
    fflush(stdout); /* output immediately, don't buffer */
  }
  if (out_to_stderr.get(*state) && draw_mode == FG) {
    fprintf(stderr, "%s\n", s);
    fflush(stderr); /* output immediately, don't buffer */
  }
  if (draw_mode == FG && (overwrite_fpointer != nullptr)) {
    fprintf(overwrite_fpointer, "%s\n", s);
  }
  if (draw_mode == FG && (append_fpointer != nullptr)) {
    fprintf(append_fpointer, "%s\n", s);
  }
#ifdef BUILD_NCURSES
  if (out_to_ncurses.get(*state) && draw_mode == FG) { printw("%s", s); }
#endif /* BUILD_NCURSES */
#ifdef BUILD_HTTP
  if (out_to_http.get(*state) && draw_mode == FG) {
    std::string::size_type origlen = webpage.length();
    webpage.append(s);
    webpage = string_replace_all(webpage, "\n", "<br />", origlen);
    webpage = string_replace_all(webpage, "  ", "&nbsp;&nbsp;", origlen);
    webpage = string_replace_all(webpage, "&nbsp; ", "&nbsp;&nbsp;", origlen);
    webpage.append("<br />");
  }
#endif /* BUILD_HTTP */
  int tbs = text_buffer_size.get(*state);
  memset(tmpstring1, 0, tbs);
  memset(tmpstring2, 0, tbs);
  strncpy(tmpstring1, s, tbs - 1);
  pos = 0;
  added = 0;

#ifdef BUILD_X11
  if (out_to_x.get(*state)) {
    max = ((text_width - width_of_s) / get_string_width(" "));
  }
#endif /* BUILD_X11 */
  /* This code looks for tabs in the text and coverts them to spaces.
   * The trick is getting the correct number of spaces, and not going
   * over the window's size without forcing the window larger. */
  for (i = 0; i < tbs; i++) {
    if (tmpstring1[i] == '\t') {
      i2 = 0;
      for (i2 = 0; i2 < (8 - (1 + pos) % 8) && added <= max; i2++) {
        /* guard against overrun */
        tmpstring2[std::min(pos + i2, tbs - 1)] = ' ';
        added++;
      }
      pos += i2;
    } else {
      /* guard against overrun */
      tmpstring2[std::min(pos, tbs - 1)] = tmpstring1[i];
      pos++;
    }
  }
#ifdef BUILD_X11
  if (out_to_x.get(*state)) {
    int mw = maximum_width.get(*state);
    if (text_width == mw) {
      /* this means the text is probably pushing the limit,
       * so we'll chop it */
      while (cur_x + get_string_width(tmpstring2) - text_start_x > mw &&
             strlen(tmpstring2) > 0) {
        tmpstring2[strlen(tmpstring2) - 1] = '\0';
      }
    }
  }
#endif /* BUILD_X11 */
  s = tmpstring2;
#ifdef BUILD_X11
  if (out_to_x.get(*state)) {
#ifdef BUILD_XFT
    if (use_xft.get(*state)) {
      XColor c;
      XftColor c2;

      c.pixel = current_color;
      // query color on custom colormap
      XQueryColor(display, window.colourmap, &c);

      c2.pixel = c.pixel;
      c2.color.red = c.red;
      c2.color.green = c.green;
      c2.color.blue = c.blue;
      c2.color.alpha = fonts[selected_font].font_alpha;
      if (utf8_mode.get(*state)) {
        XftDrawStringUtf8(window.xftdraw, &c2, fonts[selected_font].xftfont,
                          text_offset_x + cur_x, text_offset_y + cur_y,
                          reinterpret_cast<const XftChar8 *>(s), strlen(s));
      } else {
        XftDrawString8(window.xftdraw, &c2, fonts[selected_font].xftfont,
                       text_offset_x + cur_x, text_offset_y + cur_y,
                       reinterpret_cast<const XftChar8 *>(s), strlen(s));
      }
    } else
#endif
    {
      if (utf8_mode.get(*state)) {
        Xutf8DrawString(display, window.drawable, fonts[selected_font].fontset,
                        window.gc, text_offset_x + cur_x, text_offset_y + cur_y,
                        s, strlen(s));
      } else {
        XDrawString(display, window.drawable, window.gc, text_offset_x + cur_x,
                    text_offset_y + cur_y, s, strlen(s));
      }
    }
    cur_x += width_of_s;
  }
#endif /* BUILD_X11 */
  memcpy(tmpstring1, s, tbs);
}

int draw_each_line_inner(char *s, int special_index, int last_special_applied) {
#ifndef BUILD_X11
  static int cur_x, cur_y; /* current x and y for drawing */
  (void)cur_y;
#endif
#ifdef BUILD_X11
  int font_h = 0;
  int cur_y_add = 0;
  int mw = maximum_width.get(*state);
#endif /* BUILD_X11 */
  char *p = s;
  int orig_special_index = special_index;

#ifdef BUILD_X11
  if (out_to_x.get(*state)) {
    font_h = font_height();
    cur_y += font_ascent();
  }
  cur_x = text_start_x;
#endif /* BUILD_X11 */

  while (*p != 0) {
    if (*p == SPECIAL_CHAR || last_special_applied > -1) {
#ifdef BUILD_X11
      int w = 0;
#endif /* BUILD_X11 */

      /* draw string before special, unless we're dealing multiline
       * specials */
      if (last_special_applied > -1) {
        special_index = last_special_applied;
      } else {
        *p = '\0';
        draw_string(s);
        *p = SPECIAL_CHAR;
        s = p + 1;
      }
      /* draw special */
      special_t *current = specials;
      for (int i = 0; i < special_index; i++) { current = current->next; }
      switch (current->type) {
#ifdef BUILD_X11
        case HORIZONTAL_LINE:
          if (out_to_x.get(*state)) {
            int h = current->height;
            int mid = font_ascent() / 2;

            w = text_start_x + text_width - cur_x;

            XSetLineAttributes(display, window.gc, h, LineSolid, CapButt,
                               JoinMiter);
            XDrawLine(display, window.drawable, window.gc,
                      text_offset_x + cur_x, text_offset_y + cur_y - mid / 2,
                      text_offset_x + cur_x + w,
                      text_offset_y + cur_y - mid / 2);
          }
          break;

        case STIPPLED_HR:
          if (out_to_x.get(*state)) {
            int h = current->height;
            char tmp_s = current->arg;
            int mid = font_ascent() / 2;
            char ss[2] = {tmp_s, tmp_s};

            w = text_start_x + text_width - cur_x - 1;
            XSetLineAttributes(display, window.gc, h, LineOnOffDash, CapButt,
                               JoinMiter);
            XSetDashes(display, window.gc, 0, ss, 2);
            XDrawLine(display, window.drawable, window.gc,
                      text_offset_x + cur_x, text_offset_y + cur_y - mid / 2,
                      text_offset_x + cur_x + w,
                      text_offset_x + cur_y - mid / 2);
          }
          break;

        case BAR:
          if (out_to_x.get(*state)) {
            int h, by;
            double bar_usage, scale;
            if (cur_x - text_start_x > mw && mw > 0) { break; }
            h = current->height;
            bar_usage = current->arg;
            scale = current->scale;
            by = cur_y - (font_ascent() / 2) - 1;

            if (h < font_h) { by -= h / 2 - 1; }
            w = current->width;
            if (w == 0) { w = text_start_x + text_width - cur_x - 1; }
            if (w < 0) { w = 0; }

            XSetLineAttributes(display, window.gc, 1, LineSolid, CapButt,
                               JoinMiter);

            XDrawRectangle(display, window.drawable, window.gc,
                           text_offset_x + cur_x, text_offset_y + by, w, h);
            XFillRectangle(display, window.drawable, window.gc,
                           text_offset_x + cur_x, text_offset_y + by,
                           w * bar_usage / scale, h);
            if (h > cur_y_add && h > font_h) { cur_y_add = h; }
          }
          break;

        case GAUGE: /* new GAUGE  */
          if (out_to_x.get(*state)) {
            int h, by = 0;
            unsigned long last_colour = current_color;
#ifdef BUILD_MATH
            float angle, px, py;
            double usage, scale;
#endif /* BUILD_MATH */

            if (cur_x - text_start_x > mw && mw > 0) { break; }

            h = current->height;
            by = cur_y - (font_ascent() / 2) - 1;

            if (h < font_h) { by -= h / 2 - 1; }
            w = current->width;
            if (w == 0) { w = text_start_x + text_width - cur_x - 1; }
            if (w < 0) { w = 0; }

            XSetLineAttributes(display, window.gc, 1, LineSolid, CapButt,
                               JoinMiter);

            XDrawArc(display, window.drawable, window.gc, text_offset_x + cur_x,
                     text_offset_y + by, w, h * 2, 0, 180 * 64);

#ifdef BUILD_MATH
            usage = current->arg;
            scale = current->scale;
            angle = M_PI * usage / scale;
            px = static_cast<float>(cur_x + (w / 2.)) -
                 static_cast<float>(w / 2.) * cos(angle);
            py = static_cast<float>(by + (h)) -
                 static_cast<float>(h) * sin(angle);

            XDrawLine(display, window.drawable, window.gc,
                      text_offset_x + cur_x + (w / 2.),
                      text_offset_y + by + (h),
                      text_offset_x + static_cast<int>(px),
                      text_offset_y + static_cast<int>(py));
#endif /* BUILD_MATH */

            if (h > cur_y_add && h > font_h) { cur_y_add = h; }

            set_foreground_color(last_colour);
          }
          break;

        case GRAPH:
          if (out_to_x.get(*state)) {
            int h, by, i = 0, j = 0;
            int colour_idx = 0;
            unsigned long last_colour = current_color;
            if (cur_x - text_start_x > mw && mw > 0) { break; }
            h = current->height;
            by = cur_y - (font_ascent() / 2) - 1;

            if (h < font_h) { by -= h / 2 - 1; }
            w = current->width;
            if (w == 0) {
              w = text_start_x + text_width - cur_x - 1;
              current->graph_width = std::max(w - 1, 0);
              if (current->graph_width != current->graph_allocated) {
                w = current->graph_allocated + 1;
              }
            }
            if (w < 0) { w = 0; }
            if (draw_graph_borders.get(*state)) {
              XSetLineAttributes(display, window.gc, 1, LineSolid, CapButt,
                                 JoinMiter);
              XDrawRectangle(display, window.drawable, window.gc,
                             text_offset_x + cur_x, text_offset_y + by, w, h);
            }
            XSetLineAttributes(display, window.gc, 1, LineSolid, CapButt,
                               JoinMiter);

            /* in case we don't have a graph yet */
            if (current->graph != nullptr) {
              unsigned long *tmpcolour = nullptr;

              if (current->last_colour != 0 || current->first_colour != 0) {
                tmpcolour = do_gradient(w - 1, current->last_colour,
                                        current->first_colour);
              }
              colour_idx = 0;
              for (i = w - 2; i > -1; i--) {
                if (current->last_colour != 0 || current->first_colour != 0) {
                  if (current->tempgrad != 0) {
                    set_foreground_color(tmpcolour[static_cast<int>(
                        static_cast<float>(w - 2) -
                        current->graph[j] * (w - 2) /
                            std::max(static_cast<float>(current->scale),
                                     1.0F))]);
                  } else {
                    set_foreground_color(tmpcolour[colour_idx++]);
                  }
                }
                /* this is mugfugly, but it works */
                XDrawLine(
                    display, window.drawable, window.gc,
                    text_offset_x + cur_x + i + 1, text_offset_y + by + h,
                    text_offset_x + cur_x + i + 1,
                    text_offset_y + round_to_int(static_cast<double>(by) + h -
                                                 current->graph[j] * (h - 1) /
                                                     current->scale));
                ++j;
              }
              free_and_zero(tmpcolour);
            }
            if (h > cur_y_add && h > font_h) { cur_y_add = h; }
            if (show_graph_range.get(*state)) {
              int tmp_x = cur_x;
              int tmp_y = cur_y;
              unsigned short int seconds = active_update_interval() * w;
              char *tmp_day_str;
              char *tmp_hour_str;
              char *tmp_min_str;
              char *tmp_sec_str;
              char *tmp_str;
              unsigned short int timeunits;
              if (seconds != 0) {
                timeunits = seconds / 86400;
                seconds %= 86400;
                if (timeunits <= 0 ||
                    asprintf(&tmp_day_str, _("%dd"), timeunits) == -1) {
                  tmp_day_str = strdup("");
                }
                timeunits = seconds / 3600;
                seconds %= 3600;
                if (timeunits <= 0 ||
                    asprintf(&tmp_hour_str, _("%dh"), timeunits) == -1) {
                  tmp_hour_str = strdup("");
                }
                timeunits = seconds / 60;
                seconds %= 60;
                if (timeunits <= 0 ||
                    asprintf(&tmp_min_str, _("%dm"), timeunits) == -1) {
                  tmp_min_str = strdup("");
                }
                if (seconds <= 0 ||
                    asprintf(&tmp_sec_str, _("%ds"), seconds) == -1) {
                  tmp_sec_str = strdup("");
                }
                if (asprintf(&tmp_str, "%s%s%s%s", tmp_day_str, tmp_hour_str,
                             tmp_min_str, tmp_sec_str) == -1) {
                  tmp_str = strdup("");
                }
                free(tmp_day_str);
                free(tmp_hour_str);
                free(tmp_min_str);
                free(tmp_sec_str);
              } else {
                tmp_str = strdup(
                    _("Range not possible"));  // should never happen, but
                                               // better safe then sorry
              }
              cur_x += (w / 2) - (font_ascent() * (strlen(tmp_str) / 2));
              cur_y += font_h / 2;
              draw_string(tmp_str);
              free(tmp_str);
              cur_x = tmp_x;
              cur_y = tmp_y;
            }
#ifdef BUILD_MATH
            if (show_graph_scale.get(*state) && (current->show_scale == 1)) {
              int tmp_x = cur_x;
              int tmp_y = cur_y;
              char *tmp_str;
              cur_x += font_ascent() / 2;
              cur_y += font_h / 2;
              if (asprintf(&tmp_str, "%.1f", current->scale)) {
                draw_string(tmp_str);
                free(tmp_str);
              }
              cur_x = tmp_x;
              cur_y = tmp_y;
            }
#endif
            set_foreground_color(last_colour);
          }
          break;

        case FONT:
          if (out_to_x.get(*state)) {
            int old = font_ascent();

            cur_y -= font_ascent();
            selected_font = current->font_added;
            set_font();
            if (cur_y + font_ascent() < cur_y + old) {
              cur_y += old;
            } else {
              cur_y += font_ascent();
            }
            font_h = font_height();
          }
          break;
#endif /* BUILD_X11 */
        case FG:
          if (draw_mode == FG) { set_foreground_color(current->arg); }
          break;

#ifdef BUILD_X11
        case BG:
          if (draw_mode == BG) { set_foreground_color(current->arg); }
          break;

        case OUTLINE:
          if (draw_mode == OUTLINE) { set_foreground_color(current->arg); }
          break;

        case OFFSET:
          w += current->arg;
          break;

        case VOFFSET:
          cur_y += current->arg;
          break;

        case TAB: {
          int start = current->arg;
          int step = current->width;

          if ((step == 0) || step < 0) { step = 10; }
          w = step - (cur_x - text_start_x - start) % step;
          break;
        }

        case ALIGNR: {
          /* TODO: add back in "+ window.border_inner_margin" to the end of
           * this line? */
          int pos_x = text_start_x + text_width -
                      get_string_width_special(s, special_index);

          /* printf("pos_x %i text_start_x %i text_width %i cur_x %i "
            "get_string_width(p) %i gap_x %i "
            "current->arg %i window.border_inner_margin %i "
            "window.border_width %i\n", pos_x, text_start_x, text_width,
            cur_x, get_string_width_special(s), gap_x,
            current->arg, window.border_inner_margin,
            window.border_width); */
          if (pos_x > current->arg && pos_x > cur_x) {
            cur_x = pos_x - current->arg;
          }
          break;
        }

        case ALIGNC: {
          int pos_x = (text_width) / 2 -
                      get_string_width_special(s, special_index) / 2 -
                      (cur_x - text_start_x);
          /* int pos_x = text_start_x + text_width / 2 -
            get_string_width_special(s) / 2; */

          /* printf("pos_x %i text_start_x %i text_width %i cur_x %i "
            "get_string_width(p) %i gap_x %i "
            "current->arg %i\n", pos_x, text_start_x,
            text_width, cur_x, get_string_width(s), gap_x,
            current->arg); */
          if (pos_x > current->arg) { w = pos_x - current->arg; }
          break;
        }
#endif /* BUILD_X11 */
        case GOTO:
          if (current->arg >= 0) {
#ifdef BUILD_X11
            cur_x = static_cast<int>(current->arg);
            // make sure shades are 1 pixel to the right of the text
            if (draw_mode == BG) { cur_x++; }
#endif /* BUILD_X11 */
#ifdef BUILD_NCURSES
            cur_x = static_cast<int>(current->arg);
            if (out_to_ncurses.get(*state)) {
              int x, y;
              getyx(ncurses_window, y, x);
              move(y, cur_x);
            }
#endif /* BUILD_NCURSES */
          }
          break;
      }

#ifdef BUILD_X11
      cur_x += w;
#endif /* BUILD_X11 */

      if (special_index != last_special_applied) {
        special_index++;
      } else {
        special_index = orig_special_index;
        last_special_applied = -1;
      }
    }
    p++;
  }

#ifdef BUILD_X11
  cur_y += cur_y_add;
#endif /* BUILD_X11 */
  draw_string(s);
#ifdef BUILD_NCURSES
  if (out_to_ncurses.get(*state)) { printw("\n"); }
#endif /* BUILD_NCURSES */
#ifdef BUILD_X11
  if (out_to_x.get(*state)) { cur_y += font_descent(); }
#endif /* BUILD_X11 */
  return special_index;
}

static int draw_line(char *s, int special_index) {
#ifdef BUILD_X11
  if (out_to_x.get(*state)) {
    return draw_each_line_inner(s, special_index, -1);
  }
#endif /* BUILD_X11 */
#ifdef BUILD_NCURSES
  if (out_to_ncurses.get(*state)) {
    return draw_each_line_inner(s, special_index, -1);
  }
#endif /* BUILD_NCURSES */
  draw_string(s);
  UNUSED(special_index);
  return 0;
}

static void draw_text() {
#ifdef BUILD_HTTP
#define WEBPAGE_START1                                             \
  "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\" "    \
  "\"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\">\n<html " \
  "xmlns=\"http://www.w3.org/1999/xhtml\"><head><meta "            \
  "http-equiv=\"Content-type\" content=\"text/html;charset=UTF-8\" />"
#define WEBPAGE_START2 \
  "<title>Conky</title></head><body style=\"font-family: monospace\"><p>"
#define WEBPAGE_END "</p></body></html>"
  if (out_to_http.get(*state)) {
    webpage = WEBPAGE_START1;
    if (http_refresh.get(*state)) {
      webpage.append("<meta http-equiv=\"refresh\" content=\"");
      std::stringstream update_interval_str;
      update_interval_str << update_interval.get(*state);
      webpage.append(update_interval_str.str());
      webpage.append("\" />");
    }
    webpage.append(WEBPAGE_START2);
  }
#endif /* BUILD_HTTP */
#ifdef BUILD_X11
  if (out_to_x.get(*state)) {
    cur_y = text_start_y;
    int bw = border_width.get(*state);

    /* draw borders */
    if (draw_borders.get(*state) && bw > 0) {
      if (stippled_borders.get(*state) != 0) {
        char ss[2] = {stippled_borders.get(*state),
                      stippled_borders.get(*state)};
        XSetLineAttributes(display, window.gc, bw, LineOnOffDash, CapButt,
                           JoinMiter);
        XSetDashes(display, window.gc, 0, ss, 2);
      } else {
        XSetLineAttributes(display, window.gc, bw, LineSolid, CapButt,
                           JoinMiter);
      }

      int offset = border_inner_margin.get(*state) + bw;
      XDrawRectangle(display, window.drawable, window.gc,
                     text_offset_x + text_start_x - offset,
                     text_offset_y + text_start_y - offset,
                     text_width + 2 * offset, text_height + 2 * offset);
    }

    /* draw text */
  }
  setup_fonts();
#endif /* BUILD_X11 */
#ifdef BUILD_NCURSES
  init_pair(COLOR_WHITE, COLOR_WHITE, COLOR_BLACK);
  attron(COLOR_PAIR(COLOR_WHITE));
#endif /* BUILD_NCURSES */
  for_each_line(text_buffer, draw_line);
#ifdef BUILD_HTTP
  if (out_to_http.get(*state)) { webpage.append(WEBPAGE_END); }
#endif /* BUILD_HTTP */
}

static void draw_stuff() {
#ifdef BUILD_X11
  text_offset_x = text_offset_y = 0;
#ifdef BUILD_IMLIB2
  cimlib_render(text_start_x, text_start_y, window.width, window.height);
#endif /* BUILD_IMLIB2 */
#endif /* BUILD_X11 */
  if (static_cast<unsigned int>(!overwrite_file.get(*state).empty()) != 0u) {
    overwrite_fpointer = fopen(overwrite_file.get(*state).c_str(), "we");
    if (overwrite_fpointer == nullptr) {
      NORM_ERR("Cannot overwrite '%s'", overwrite_file.get(*state).c_str());
    }
  }
  if (static_cast<unsigned int>(!append_file.get(*state).empty()) != 0u) {
    append_fpointer = fopen(append_file.get(*state).c_str(), "ae");
    if (append_fpointer == nullptr) {
      NORM_ERR("Cannot append to '%s'", append_file.get(*state).c_str());
    }
  }
#ifdef BUILD_X11
  llua_draw_pre_hook();
  if (out_to_x.get(*state)) {
    selected_font = 0;
    if (draw_shades.get(*state) && !draw_outline.get(*state)) {
      text_offset_x = text_offset_y = 1;
      text_start_y++;
      set_foreground_color(default_shade_color.get(*state));
      draw_mode = BG;
      draw_text();
      text_offset_x = text_offset_y = 0;
    }

    if (draw_outline.get(*state)) {
      selected_font = 0;

      for (text_offset_x = -1; text_offset_x < 2; text_offset_x++) {
        for (text_offset_y = -1; text_offset_y < 2; text_offset_y++) {
          if (text_offset_x == 0 && text_offset_y == 0) { continue; }
          set_foreground_color(default_outline_color.get(*state));
          draw_mode = OUTLINE;
          draw_text();
        }
      }
      text_offset_x = text_offset_y = 0;
    }

    set_foreground_color(default_color.get(*state));
  }
#endif /* BUILD_X11 */
  draw_mode = FG;
  draw_text();
#if defined(BUILD_X11)
  llua_draw_post_hook();
#if defined(BUILD_XDBE)
  if (out_to_x.get(*state)) { xdbe_swap_buffers(); }
#else
  if (out_to_x.get(*state)) { xpmdb_swap_buffers(); }
#endif
#endif /* BUILD_X11 && BUILD_XDBE */
  if (overwrite_fpointer != nullptr) {
    fclose(overwrite_fpointer);
    overwrite_fpointer = nullptr;
  }
  if (append_fpointer != nullptr) {
    fclose(append_fpointer);
    append_fpointer = nullptr;
  }
}

#ifdef BUILD_X11
static void clear_text(int exposures) {
#ifdef BUILD_XDBE
  if (use_xdbe.get(*state)) {
    /* The swap action is XdbeBackground, which clears */
    return;
  }
#else
  if (use_xpmdb.get(*state)) {
    return;
  } else
#endif
  if ((display != nullptr) &&
      (window.window != 0u)) {  // make sure these are !null
    /* there is some extra space for borders and outlines */
    int border_total = get_border_total();

    XClearArea(display, window.window, text_start_x - border_total,
               text_start_y - border_total, text_width + 2 * border_total,
               text_height + 2 * border_total, exposures != 0 ? True : 0);
  }
}
#endif /* BUILD_X11 */

static int need_to_update;

/* update_text() generates new text and clears old text area */
static void update_text() {
#ifdef BUILD_IMLIB2
  cimlib_cleanup();
#endif /* BUILD_IMLIB2 */
  generate_text();
#ifdef BUILD_X11
  if (out_to_x.get(*state)) { clear_text(1); }
#endif /* BUILD_X11 */
  need_to_update = 1;
  llua_update_info(&info, active_update_interval());
}

#ifdef HAVE_SYS_INOTIFY_H
int inotify_fd = -1;
#endif

template <typename Out>
void split(const std::string &s, char delim, Out result) {
  std::stringstream ss(s);
  std::string item;
  while (std::getline(ss, item, delim)) { *(result++) = item; }
}
std::vector<std::string> split(const std::string &s, char delim) {
  std::vector<std::string> elems;
  split(s, delim, std::back_inserter(elems));
  return elems;
}

bool is_on_battery() {  // checks if at least one battery specified in
                        // "detect_battery" is discharging
  char buf[64];
  std::vector<std::string> b_items = split(detect_battery.get(*state), ',');

  for (auto const &value : b_items) {
    get_battery_short_status(buf, 64, value.c_str());
    if (buf[0] == 'D') { return true; }
  }
  return false;
}

volatile sig_atomic_t g_sigterm_pending, g_sighup_pending, g_sigusr2_pending;

void main_loop() {
  int terminate = 0;
#ifdef SIGNAL_BLOCKING
  sigset_t newmask, oldmask;
#endif
#ifdef BUILD_X11
  double t;
#endif /* BUILD_X11 */
#ifdef HAVE_SYS_INOTIFY_H
  int inotify_config_wd = -1;
#define INOTIFY_EVENT_SIZE (sizeof(struct inotify_event))
#define INOTIFY_BUF_LEN (20 * (INOTIFY_EVENT_SIZE + 16)) + 1
  char inotify_buff[INOTIFY_BUF_LEN];
#endif /* HAVE_SYS_INOTIFY_H */

#ifdef SIGNAL_BLOCKING
  sigemptyset(&newmask);
  sigaddset(&newmask, SIGINT);
  sigaddset(&newmask, SIGTERM);
  sigaddset(&newmask, SIGUSR1);
#endif

  last_update_time = 0.0;
  next_update_time = get_time() - fmod(get_time(), active_update_interval());
  info.looped = 0;
  while (terminate == 0 && (total_run_times.get(*state) == 0 ||
                            info.looped < total_run_times.get(*state))) {
    if ((update_interval_on_battery.get(*state) != NOBATTERY)) {
      on_battery = is_on_battery();
    }
    info.looped++;

#ifdef SIGNAL_BLOCKING
    /* block signals.  we will inspect for pending signals later */
    if (sigprocmask(SIG_BLOCK, &newmask, &oldmask) < 0) {
      CRIT_ERR(nullptr, NULL, "unable to sigprocmask()");
    }
#endif

#ifdef BUILD_X11
    if (out_to_x.get(*state)) {
      /* wait for X event or timeout */

      if (XPending(display) == 0) {
        fd_set fdsr;
        struct timeval tv {};
        int s;
        t = next_update_time - get_time();

        t = std::min(std::max(t, 0.0), active_update_interval());

        tv.tv_sec = static_cast<long>(t);
        tv.tv_usec = static_cast<long>(t * 1000000) % 1000000;
        FD_ZERO(&fdsr);
        FD_SET(ConnectionNumber(display), &fdsr);

        s = select(ConnectionNumber(display) + 1, &fdsr, nullptr, nullptr, &tv);
        if (s == -1) {
          if (errno != EINTR) {
            NORM_ERR("can't select(): %s", strerror(errno));
          }
        } else {
          /* timeout */
          if (s == 0) { update_text(); }
        }
      }

      if (need_to_update != 0) {
#ifdef OWN_WINDOW
        int wx = window.x, wy = window.y;
#endif

        need_to_update = 0;
        selected_font = 0;
        update_text_area();

#ifdef OWN_WINDOW
        if (own_window.get(*state)) {
          int changed = 0;
          int border_total = get_border_total();

          /* resize window if it isn't right size */
          if ((fixed_size == 0) &&
              (text_width + 2 * border_total != window.width ||
               text_height + 2 * border_total != window.height)) {
            window.width = text_width + 2 * border_total;
            window.height = text_height + 2 * border_total;
            draw_stuff(); /* redraw everything in our newly sized window */
            XResizeWindow(display, window.window, window.width,
                          window.height); /* resize window */
            set_transparent_background(window.window);
#ifdef BUILD_XDBE
            /* swap buffers */
            xdbe_swap_buffers();
#else
            if (use_xpmdb.get(*state)) {
              XFreePixmap(display, window.back_buffer);
              window.back_buffer =
                  XCreatePixmap(display, window.window, window.width,
                                window.height, DefaultDepth(display, screen));

              if (window.back_buffer != None) {
                window.drawable = window.back_buffer;
              } else {
                // this is probably reallllly bad
                NORM_ERR("Failed to allocate back buffer");
              }
              XSetForeground(display, window.gc, 0);
              XFillRectangle(display, window.drawable, window.gc, 0, 0,
                             window.width, window.height);
            }
#endif

            changed++;
            /* update lua window globals */
            llua_update_window_table(text_start_x, text_start_y, text_width,
                                     text_height);
          }

          /* move window if it isn't in right position */
          if ((fixed_pos == 0) && (window.x != wx || window.y != wy)) {
            XMoveWindow(display, window.window, window.x, window.y);
            changed++;
          }

          /* update struts */
          if ((changed != 0) && own_window_type.get(*state) == TYPE_PANEL) {
            int sidenum = -1;

            DBGP("%s", _(PACKAGE_NAME ": defining struts\n"));
            fflush(stderr);

            switch (text_alignment.get(*state)) {
              case TOP_LEFT:
              case TOP_RIGHT:
              case TOP_MIDDLE: {
                sidenum = 2;
                break;
              }
              case BOTTOM_LEFT:
              case BOTTOM_RIGHT:
              case BOTTOM_MIDDLE: {
                sidenum = 3;
                break;
              }
              case MIDDLE_LEFT: {
                sidenum = 0;
                break;
              }
              case MIDDLE_RIGHT: {
                sidenum = 1;
                break;
              }

              case NONE:
              case MIDDLE_MIDDLE: /* XXX What about these? */;
            }

            set_struts(sidenum);
          }
        }
#endif

        clear_text(1);

#if defined(BUILD_XDBE)
        if (use_xdbe.get(*state)) {
#else
        if (use_xpmdb.get(*state)) {
#endif
          XRectangle r;
          int border_total = get_border_total();

          r.x = text_start_x - border_total;
          r.y = text_start_y - border_total;
          r.width = text_width + 2 * border_total;
          r.height = text_height + 2 * border_total;
          XUnionRectWithRegion(&r, x11_stuff.region, x11_stuff.region);
        }
      }

      /* handle X events */
      while (XPending(display) != 0) {
        XEvent ev;

        XNextEvent(display, &ev);
        switch (ev.type) {
          case Expose: {
            XRectangle r;
            r.x = ev.xexpose.x;
            r.y = ev.xexpose.y;
            r.width = ev.xexpose.width;
            r.height = ev.xexpose.height;
            XUnionRectWithRegion(&r, x11_stuff.region, x11_stuff.region);
            XSync(display, False);
            break;
          }

          case PropertyNotify: {
            if (ev.xproperty.state == PropertyNewValue) {
              get_x11_desktop_info(ev.xproperty.display, ev.xproperty.atom);
            }
#ifdef USE_ARGB
            if (!have_argb_visual) {
#endif
              if (ev.xproperty.atom == ATOM(_XROOTPMAP_ID) ||
                  ev.xproperty.atom == ATOM(_XROOTMAP_ID)) {
               if (forced_redraw.get(*state)) {
                  draw_stuff();
                  next_update_time = get_time();
                  need_to_update = 1;
                 }
              }
#ifdef USE_ARGB
            }
#endif
            break;
          }

#ifdef OWN_WINDOW
          case ReparentNotify:
            /* make background transparent */
            if (own_window.get(*state)) {
              set_transparent_background(window.window);
            }
            break;

          case ConfigureNotify:
            if (own_window.get(*state)) {
              /* if window size isn't what expected, set fixed size */
              if (ev.xconfigure.width != window.width ||
                  ev.xconfigure.height != window.height) {
                if (window.width != 0 && window.height != 0) { fixed_size = 1; }

                /* clear old stuff before screwing up
                 * size and pos */
                clear_text(1);

                {
                  XWindowAttributes attrs;
                  if (XGetWindowAttributes(display, window.window, &attrs) !=
                      0) {
                    window.width = attrs.width;
                    window.height = attrs.height;
                  }
                }

                int border_total = get_border_total();

                text_width = window.width - 2 * border_total;
                text_height = window.height - 2 * border_total;
                int mw = maximum_width.get(*state);
                if (text_width > mw && mw > 0) { text_width = mw; }
              }

              /* if position isn't what expected, set fixed pos
               * total_updates avoids setting fixed_pos when window
               * is set to weird locations when started */
              /* // this is broken
              if (total_updates >= 2 && !fixed_pos
                  && (window.x != ev.xconfigure.x
                  || window.y != ev.xconfigure.y)
                  && (ev.xconfigure.x != 0
                  || ev.xconfigure.y != 0)) {
                fixed_pos = 1;
              } */
            }
            break;

          case ButtonPress:
            if (own_window.get(*state)) {
              /* if an ordinary window with decorations */
              if ((own_window_type.get(*state) == TYPE_NORMAL &&
                   !TEST_HINT(own_window_hints.get(*state),
                              HINT_UNDECORATED)) ||
                  own_window_type.get(*state) == TYPE_DESKTOP) {
                /* allow conky to hold input focus. */
                break;
              }
              /* forward the click to the desktop window */
              XUngrabPointer(display, ev.xbutton.time);
              ev.xbutton.window = window.desktop;
              ev.xbutton.x = ev.xbutton.x_root;
              ev.xbutton.y = ev.xbutton.y_root;
              XSendEvent(display, ev.xbutton.window, False, ButtonPressMask,
                         &ev);
              XSetInputFocus(display, ev.xbutton.window, RevertToParent,
                             ev.xbutton.time);
            }
            break;

          case ButtonRelease:
            if (own_window.get(*state)) {
              /* if an ordinary window with decorations */
              if ((own_window_type.get(*state) == TYPE_NORMAL) &&
                  not TEST_HINT(own_window_hints.get(*state),
                                HINT_UNDECORATED)) {
                /* allow conky to hold input focus. */
                break;
              }
              /* forward the release to the desktop window */
              ev.xbutton.window = window.desktop;
              ev.xbutton.x = ev.xbutton.x_root;
              ev.xbutton.y = ev.xbutton.y_root;
              XSendEvent(display, ev.xbutton.window, False, ButtonReleaseMask,
                         &ev);
            }
            break;

#endif

          default:
#ifdef BUILD_XDAMAGE
            if (ev.type == x11_stuff.event_base + XDamageNotify) {
              auto *dev = reinterpret_cast<XDamageNotifyEvent *>(&ev);

              XFixesSetRegion(display, x11_stuff.part, &dev->area, 1);
              XFixesUnionRegion(display, x11_stuff.region2, x11_stuff.region2,
                                x11_stuff.part);
            }
#endif /* BUILD_XDAMAGE */
            break;
        }
      }

#ifdef BUILD_XDAMAGE
      if (x11_stuff.damage) {
        XDamageSubtract(display, x11_stuff.damage, x11_stuff.region2, None);
        XFixesSetRegion(display, x11_stuff.region2, nullptr, 0);
      }
#endif /* BUILD_XDAMAGE */

      /* XDBE doesn't seem to provide a way to clear the back buffer
       * without interfering with the front buffer, other than passing
       * XdbeBackground to XdbeSwapBuffers. That means that if we're
       * using XDBE, we need to redraw the text even if it wasn't part of
       * the exposed area. OTOH, if we're not going to call draw_stuff at
       * all, then no swap happens and we can safely do nothing. */

      if (XEmptyRegion(x11_stuff.region) == 0) {
#if defined(BUILD_XDBE)
        if (use_xdbe.get(*state)) {
#else
        if (use_xpmdb.get(*state)) {
#endif
          XRectangle r;
          int border_total = get_border_total();

          r.x = text_start_x - border_total;
          r.y = text_start_y - border_total;
          r.width = text_width + 2 * border_total;
          r.height = text_height + 2 * border_total;
          XUnionRectWithRegion(&r, x11_stuff.region, x11_stuff.region);
        }
        XSetRegion(display, window.gc, x11_stuff.region);
#ifdef BUILD_XFT
        if (use_xft.get(*state)) {
          XftDrawSetClip(window.xftdraw, x11_stuff.region);
        }
#endif
        draw_stuff();
        XDestroyRegion(x11_stuff.region);
        x11_stuff.region = XCreateRegion();
      }
    } else {
#endif /* BUILD_X11 */
      struct timespec req, rem;
      auto time_to_sleep = next_update_time - get_time();
      auto seconds = static_cast<time_t>(std::floor(time_to_sleep));
      auto nanos = (time_to_sleep - seconds) * 1000000000L;
      req.tv_sec = seconds;
      req.tv_nsec = nanos;
      nanosleep(&req, &rem);
      update_text();
      draw_stuff();
#ifdef BUILD_NCURSES
      if (out_to_ncurses.get(*state)) {
        refresh();
        clear();
      }
#endif
#ifdef BUILD_X11
    }
#endif /* BUILD_X11 */

#ifdef SIGNAL_BLOCKING
    /* unblock signals of interest and let handler fly */
    if (sigprocmask(SIG_SETMASK, &oldmask, nullptr) < 0) {
      CRIT_ERR(nullptr, NULL, "unable to sigprocmask()");
    }
#endif

    if (g_sighup_pending != 0) {
      g_sighup_pending = 0;
      NORM_ERR("received SIGUSR1. reloading the config file.");

      reload_config();
    }

    if (g_sigusr2_pending != 0) {
      g_sigusr2_pending = 0;
      // refresh view;
      NORM_ERR("recieved SIGUSR2. refreshing.");
      update_text();
      draw_stuff();
#ifdef BUILD_NCURSES
      if (out_to_ncurses.get(*state)) {
        refresh();
        clear();
      }
#endif
    }

    if (g_sigterm_pending != 0) {
      g_sigterm_pending = 0;
      NORM_ERR("received SIGHUP, SIGINT, or SIGTERM to terminate. bye!");
      terminate = 1;
#ifdef BUILD_X11
      if (out_to_x.get(*state)) {
        XDestroyRegion(x11_stuff.region);
        x11_stuff.region = nullptr;
#ifdef BUILD_XDAMAGE
        if (x11_stuff.damage) {
          XDamageDestroy(display, x11_stuff.damage);
          XFixesDestroyRegion(display, x11_stuff.region2);
          XFixesDestroyRegion(display, x11_stuff.part);
        }
#endif /* BUILD_XDAMAGE */
      }
#endif /* BUILD_X11 */
    }
#ifdef HAVE_SYS_INOTIFY_H
    if (!disable_auto_reload.get(*state) && inotify_fd != -1 &&
        inotify_config_wd == -1 && !current_config.empty()) {
      inotify_config_wd =
          inotify_add_watch(inotify_fd, current_config.c_str(), IN_MODIFY);
    }
    if (!disable_auto_reload.get(*state) && inotify_fd != -1 &&
        inotify_config_wd != -1 && !current_config.empty()) {
      int len = 0, idx = 0;
      fd_set descriptors;
      struct timeval time_to_wait;

      FD_ZERO(&descriptors);
      FD_SET(inotify_fd, &descriptors);

      time_to_wait.tv_sec = time_to_wait.tv_usec = 0;

      select(inotify_fd + 1, &descriptors, nullptr, NULL, &time_to_wait);
      if (FD_ISSET(inotify_fd, &descriptors)) {
        /* process inotify events */
        len = read(inotify_fd, inotify_buff, INOTIFY_BUF_LEN - 1);
        inotify_buff[len] = 0;
        while (len > 0 && idx < len) {
          struct inotify_event *ev = (struct inotify_event *)&inotify_buff[idx];
          if (ev->wd == inotify_config_wd &&
              (ev->mask & IN_MODIFY || ev->mask & IN_IGNORED)) {
            /* current_config should be reloaded */
            NORM_ERR("'%s' modified, reloading...", current_config.c_str());
            reload_config();
            if (ev->mask & IN_IGNORED) {
              /* for some reason we get IN_IGNORED here
               * sometimes, so we need to re-add the watch */
              inotify_config_wd = inotify_add_watch(
                  inotify_fd, current_config.c_str(), IN_MODIFY);
            }
            break;
          } else {
            llua_inotify_query(ev->wd, ev->mask);
          }
          idx += INOTIFY_EVENT_SIZE + ev->len;
        }
      }
    } else if (disable_auto_reload.get(*state) && inotify_fd != -1) {
      inotify_rm_watch(inotify_fd, inotify_config_wd);
      close(inotify_fd);
      inotify_fd = inotify_config_wd = -1;
    }
#endif /* HAVE_SYS_INOTIFY_H */

    llua_update_info(&info, active_update_interval());
  }
  clean_up(nullptr, nullptr);

#ifdef HAVE_SYS_INOTIFY_H
  if (inotify_fd != -1) {
    inotify_rm_watch(inotify_fd, inotify_config_wd);
    close(inotify_fd);
    inotify_fd = inotify_config_wd = -1;
  }
#endif /* HAVE_SYS_INOTIFY_H */
}

/* reload the config file */
static void reload_config() {
  struct stat sb {};
  if ((stat(current_config.c_str(), &sb) != 0) ||
      (!S_ISREG(sb.st_mode) && !S_ISLNK(sb.st_mode))) {
    NORM_ERR(_("Config file '%s' is gone, continuing with config from "
               "memory.\nIf you recreate this file sent me a SIGUSR1 to tell "
               "me about it. ( kill -s USR1 %d )"),
             current_config.c_str(), getpid());
    return;
  }
  clean_up(nullptr, nullptr);
  state = std::make_unique<lua::state>();
  conky::export_symbols(*state);
  sleep(1); /* slight pause */
  initialisation(argc_copy, argv_copy);
}

#ifdef BUILD_X11
void clean_up_x11() {
  if (window_created == 1) {
    int border_total = get_border_total();

    XClearArea(display, window.window, text_start_x - border_total,
               text_start_y - border_total, text_width + 2 * border_total,
               text_height + 2 * border_total, 0);
  }
  destroy_window();
  free_fonts(utf8_mode.get(*state));
  if (x11_stuff.region != nullptr) {
    XDestroyRegion(x11_stuff.region);
    x11_stuff.region = nullptr;
  }
}
#endif

void free_specials(special_t *&current) {
  if (current != nullptr) {
    free_specials(current->next);
    if (current->type == GRAPH) { free(current->graph); }
    delete current;
    current = nullptr;
  }
}

void clean_up_without_threads(void *memtofree1, void *memtofree2) {
  free_and_zero(memtofree1);
  free_and_zero(memtofree2);

  free_and_zero(info.cpu_usage);
#ifdef BUILD_X11
  if (out_to_x.get(*state)) {
    clean_up_x11();
  } else {
    fonts.clear();  // in set_default_configurations a font is set but not
  }
  // loaded
#endif /* BUILD_X11 */

  if (info.first_process != nullptr) {
    free_all_processes();
    info.first_process = nullptr;
  }

  free_text_objects(&global_root_object);
  delete_block_and_zero(tmpstring1);
  delete_block_and_zero(tmpstring2);
  delete_block_and_zero(text_buffer);
  free_and_zero(global_text);

#ifdef BUILD_PORT_MONITORS
  tcp_portmon_clear();
#endif
  llua_shutdown_hook();
#if defined BUILD_RSS
  xmlCleanupParser();
#endif

  free_specials(specials);

  clear_net_stats();
  clear_fs_stats();
  clear_diskio_stats();
  free_and_zero(global_cpu);

  conky::cleanup_config_settings(*state);
  state.reset();
}

void clean_up(void *memtofree1, void *memtofree2) {
  /* free_update_callbacks(); XXX: some new equivalent of this? */
  clean_up_without_threads(memtofree1, memtofree2);
}

static void set_default_configurations() {
  update_uname();
  info.memmax = 0;
  top_cpu = 0;
  top_mem = 0;
  top_time = 0;
#ifdef BUILD_IOSTATS
  top_io = 0;
#endif
  top_running = 0;
#ifdef BUILD_XMMS2
  info.xmms2.artist = nullptr;
  info.xmms2.album = nullptr;
  info.xmms2.title = nullptr;
  info.xmms2.genre = nullptr;
  info.xmms2.comment = nullptr;
  info.xmms2.url = nullptr;
  info.xmms2.status = nullptr;
  info.xmms2.playlist = nullptr;
#endif /* BUILD_XMMS2 */
  state->pushboolean(true);
#ifdef BUILD_X11
  out_to_x.lua_set(*state);
#else
  out_to_stdout.lua_set(*state);
#endif

  info.users.number = 1;
}

#ifdef BUILD_X11
static void X11_create_window() {
  if (out_to_x.get(*state)) {
    setup_fonts();
    load_fonts(utf8_mode.get(*state));
    update_text_area(); /* to position text/window on screen */

#ifdef OWN_WINDOW
    if (own_window.get(*state)) {
      if (fixed_pos == 0) {
        XMoveWindow(display, window.window, window.x, window.y);
      }

      set_transparent_background(window.window);
    }
#endif

    create_gc();

    draw_stuff();

    x11_stuff.region = XCreateRegion();
#ifdef BUILD_XDAMAGE
    if (XDamageQueryExtension(display, &x11_stuff.event_base,
                              &x11_stuff.error_base) == 0) {
      NORM_ERR("Xdamage extension unavailable");
      x11_stuff.damage = 0;
    } else {
      x11_stuff.damage =
          XDamageCreate(display, window.window, XDamageReportNonEmpty);
      x11_stuff.region2 =
          XFixesCreateRegionFromWindow(display, window.window, 0);
      x11_stuff.part = XFixesCreateRegionFromWindow(display, window.window, 0);
    }
#endif /* BUILD_XDAMAGE */

    selected_font = 0;
    update_text_area(); /* to get initial size of the window */
  }
  /* setup lua window globals */
  llua_setup_window_table(text_start_x, text_start_y, text_width, text_height);
}
#endif /* BUILD_X11 */

void load_config_file() {
  DBGP(_("reading contents from config file '%s'"), current_config.c_str());

  lua::state &l = *state;
  lua::stack_sentry s(l);
  l.checkstack(2);

  try {
#ifdef BUILD_BUILTIN_CONFIG
    if (current_config == builtin_config_magic) {
      l.loadstring(defconfig);
    } else {
#endif
      l.loadfile(current_config.c_str());
#ifdef BUILD_BUILTIN_CONFIG
    }
#endif
  } catch (lua::syntax_error &e) {
#define SYNTAX_ERR_READ_CONF "Syntax error (%s) while reading config file. "
#ifdef BUILD_OLD_CONFIG
    NORM_ERR(_(SYNTAX_ERR_READ_CONF), e.what());
    NORM_ERR(_("Assuming it's in old syntax and attempting conversion."));
    // the strchr thingy skips the first line (#! /usr/bin/lua)
    l.loadstring(strchr(convertconf, '\n'));
    l.pushstring(current_config.c_str());
    l.call(1, 1);
#else
    char *syntaxerr;
    if (asprintf(&syntaxerr, _(SYNTAX_ERR_READ_CONF), e.what())) {
      std::string syntaxerrobj(syntaxerr);
      free(syntaxerr);
      throw conky::error(syntaxerrobj);
    }
#endif
  }
  l.call(0, 0);

  l.getglobal("conky");
  l.getfield(-1, "text");
  l.replace(-2);
  if (l.type(-1) != lua::TSTRING) {
    throw conky::error(_("missing text block in configuration"));
  }

  /* Remove \\-\n. */
  l.gsub(l.tocstring(-1), "\\\n", "");
  l.replace(-2);
  global_text = strdup(l.tocstring(-1));
  l.pop();
}

inline void reset_optind() {
#if defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || \
    defined(__OpenBSD__) || defined(__NetBSD__) || defined(__DragonFly__)
  optind = optreset = 1;
#else
  optind = 0;
#endif
}

void set_current_config() {
  /* load current_config, CONFIG_FILE or SYSTEM_CONFIG_FILE */
  struct stat s {};

  if (current_config.empty()) {
    /* Try to use personal config file first */
    std::string buf = to_real_path(XDG_CONFIG_FILE);
    if (stat(buf.c_str(), &s) == 0) { current_config = buf; }
  }

  if (current_config.empty()) {
    /* Try to use personal config file first */
    std::string buf = to_real_path(CONFIG_FILE);
    if (stat(buf.c_str(), &s) == 0) { current_config = buf; }
  }

  /* Try to use system config file if personal config does not exist */
  if (current_config.empty() && (stat(SYSTEM_CONFIG_FILE, &s) == 0)) {
    current_config = SYSTEM_CONFIG_FILE;
  }

  /* No readable config found */
  if (current_config.empty()) {
#define NOCFGFILEFOUND "no personal or system-wide config file found"
#ifdef BUILD_BUILTIN_CONFIG
    current_config = builtin_config_magic;
    NORM_ERR(NOCFGFILEFOUND ", using builtin default");
#else
    throw conky::error(NOCFGFILEFOUND);
#endif
  }

  // "-" stands for "read from stdin"
  if (current_config == "-") { current_config = "/dev/stdin"; }
}

/* : means that character before that takes an argument */
const char *getopt_string =
    "vVqdDSs:t:u:i:hc:p:"
#ifdef BUILD_X11
    "x:y:w:a:X:m:f:"
#ifdef OWN_WINDOW
    "o"
#endif
    "b"
#endif /* BUILD_X11 */
#ifdef BUILD_BUILTIN_CONFIG
    "C"
#endif
    ;

const struct option longopts[] = {
    {"help", 0, nullptr, 'h'},          {"version", 0, nullptr, 'V'},
    {"quiet", 0, nullptr, 'q'},         {"debug", 0, nullptr, 'D'},
    {"config", 1, nullptr, 'c'},
#ifdef BUILD_BUILTIN_CONFIG
    {"print-config", 0, nullptr, 'C'},
#endif
    {"daemonize", 0, nullptr, 'd'},
#ifdef BUILD_X11
    {"alignment", 1, nullptr, 'a'},     {"display", 1, nullptr, 'X'},
    {"xinerama-head", 1, nullptr, 'm'}, {"font", 1, nullptr, 'f'},
#ifdef OWN_WINDOW
    {"own-window", 0, nullptr, 'o'},
#endif
    {"double-buffer", 0, nullptr, 'b'}, {"window-id", 1, nullptr, 'w'},
#endif /* BUILD_X11 */
    {"text", 1, nullptr, 't'},          {"interval", 1, nullptr, 'u'},
    {"pause", 1, nullptr, 'p'},         {nullptr, 0, nullptr, 0}};

void setup_inotify() {
#ifdef HAVE_SYS_INOTIFY_H
  // the file descriptor will be automatically closed on exit
  inotify_fd = inotify_init();
  if (inotify_fd != -1) {
    fcntl(inotify_fd, F_SETFL, fcntl(inotify_fd, F_GETFL) | O_NONBLOCK);

    fcntl(inotify_fd, F_SETFD, fcntl(inotify_fd, F_GETFD) | FD_CLOEXEC);
  }
#endif /* HAVE_SYS_INOTIFY_H */
}
void initialisation(int argc, char **argv) {
  struct sigaction act {
  }, oact{};

  clear_net_stats();
  set_default_configurations();

  set_current_config();
  load_config_file();

  /* handle other command line arguments */

  reset_optind();

#if defined(__FreeBSD__) || defined(__FreeBSD_kernel__)
  if ((kd = kvm_open("/dev/null", "/dev/null", "/dev/null", O_RDONLY,
                     "kvm_open")) == nullptr) {
    CRIT_ERR(nullptr, NULL, "cannot read kvm");
  }
#endif

  while (1) {
    int c = getopt_long(argc, argv, getopt_string, longopts, nullptr);
    int startup_pause;
    char *conv_end;

    if (c == -1) { break; }

    switch (c) {
      case 'd':
        state->pushboolean(true);
        fork_to_background.lua_set(*state);
        break;
#ifdef BUILD_X11
      case 'f':
        state->pushstring(optarg);
        font.lua_set(*state);
        break;
      case 'a':
        state->pushstring(optarg);
        text_alignment.lua_set(*state);
        break;
      case 'm':
        state->pushinteger(strtol(optarg, &conv_end, 10));
        if (*conv_end != 0) {
          CRIT_ERR(nullptr, nullptr, "'%s' is a wrong xinerama-head index",
                   optarg);
        }
        head_index.lua_set(*state);
        break;
      case 'X':
        state->pushstring(optarg);
        display_name.lua_set(*state);
        break;

#ifdef OWN_WINDOW
      case 'o':
        state->pushboolean(true);
        own_window.lua_set(*state);
        break;
#endif
#ifdef BUILD_XDBE
      case 'b':
        state->pushboolean(true);
        use_xdbe.lua_set(*state);
        break;
#else
      case 'b':
        state->pushboolean(true);
        use_xpmdb.lua_set(*state);
        break;
#endif
#endif /* BUILD_X11 */
      case 't':
        free_and_zero(global_text);
        global_text = strndup(optarg, max_user_text.get(*state));
        convert_escapes(global_text);
        break;

      case 'u':
        state->pushinteger(strtol(optarg, &conv_end, 10));
        if (*conv_end != 0) {
          CRIT_ERR(nullptr, nullptr, "'%s' is a wrong update-interval", optarg);
        }
        update_interval.lua_set(*state);
        break;

      case 'i':
        state->pushinteger(strtol(optarg, &conv_end, 10));
        if (*conv_end != 0) {
          CRIT_ERR(nullptr, nullptr, "'%s' is a wrong number of update-times",
                   optarg);
        }
        total_run_times.lua_set(*state);
        break;
#ifdef BUILD_X11
      case 'x':
        state->pushinteger(strtol(optarg, &conv_end, 10));
        if (*conv_end != 0) {
          CRIT_ERR(nullptr, nullptr, "'%s' is a wrong value for the X-position",
                   optarg);
        }
        gap_x.lua_set(*state);
        break;

      case 'y':
        state->pushinteger(strtol(optarg, &conv_end, 10));
        if (*conv_end != 0) {
          CRIT_ERR(nullptr, nullptr, "'%s' is a wrong value for the Y-position",
                   optarg);
        }
        gap_y.lua_set(*state);
        break;
#endif /* BUILD_X11 */
      case 'p':
        if (first_pass != 0) {
          startup_pause = atoi(optarg);
          sleep(startup_pause);
        }
        break;

      case '?':
        throw unknown_arg_throw();
    }
  }

  conky::set_config_settings(*state);

#ifdef BUILD_X11
  if (out_to_x.get(*state)) { current_text_color = default_color.get(*state); }
#endif

  /* generate text and get initial size */
  extract_variable_text(global_text);
  free_and_zero(global_text);
  /* fork */
  if (fork_to_background.get(*state) && (first_pass != 0)) {
    int pid = fork();

    switch (pid) {
      case -1:
        NORM_ERR(PACKAGE_NAME ": couldn't fork() to background: %s",
                 strerror(errno));
        break;

      case 0:
        /* child process */
        usleep(25000);
        fprintf(stderr, "\n");
        fflush(stderr);
        break;

      default:
        /* parent process */
        fprintf(stderr, PACKAGE_NAME ": forked to background, pid is %d\n",
                pid);
        fflush(stderr);
        throw fork_throw();
    }
  }

  text_buffer = new char[max_user_text.get(*state)];
  memset(text_buffer, 0, max_user_text.get(*state));
  tmpstring1 = new char[text_buffer_size.get(*state)];
  memset(tmpstring1, 0, text_buffer_size.get(*state));
  tmpstring2 = new char[text_buffer_size.get(*state)];
  memset(tmpstring2, 0, text_buffer_size.get(*state));

#ifdef BUILD_X11
  X11_create_window();
#endif /* BUILD_X11 */
  llua_setup_info(&info, active_update_interval());

  /* Set signal handlers */
  act.sa_handler = signal_handler;
  sigemptyset(&act.sa_mask);
  act.sa_flags = 0;
#ifdef SA_RESTART
  act.sa_flags |= SA_RESTART;
#endif

  if (sigaction(SIGINT, &act, &oact) < 0 ||
      sigaction(SIGALRM, &act, &oact) < 0 ||
      sigaction(SIGUSR1, &act, &oact) < 0 ||
      sigaction(SIGUSR2, &act, &oact) < 0 ||
      sigaction(SIGHUP, &act, &oact) < 0 ||
      sigaction(SIGTERM, &act, &oact) < 0) {
    NORM_ERR("error setting signal handler: %s", strerror(errno));
  }

  llua_startup_hook();
}

static void signal_handler(int sig) {
  /* signal handler is light as a feather, as it should be.
   * we will poll g_signal_pending with each loop of conky
   * and do any signal processing there, NOT here */

  switch (sig) {
    case SIGHUP:
    case SIGINT:
    case SIGTERM:
      g_sigterm_pending = 1;
      break;
    case SIGUSR1:
      g_sighup_pending = 1;
      break;
    case SIGUSR2:
      g_sigusr2_pending = 1;
    default:
      /* Reaching here means someone set a signal
       * (SIGXXXX, signal_handler), but didn't write any code
       * to deal with it.
       * If you don't want to handle a signal, don't set a handler on
       * it in the first place.
       * We cannot print debug messages from a sighandler, so simply ignore.
       */
      break;
  }
}
