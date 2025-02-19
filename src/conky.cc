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
 * Copyright (c) 2005-2024 Brenden Matthews, Philip Kovacs, et. al.
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

#include "config.h"

#include <algorithm>
#include <cerrno>
#include <climits>
#include <clocale>
#include <cmath>
#include <cstdarg>
#include <ctime>
#include <filesystem>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include <fcntl.h>
#include <getopt.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>

#ifdef HAVE_SYS_INOTIFY_H
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wc99-extensions"
#include <sys/inotify.h>
#pragma clang diagnostic pop
#endif /* HAVE_SYS_INOTIFY_H */

#ifdef HAVE_DIRENT_H
#include <dirent.h>
#endif /* HAVE_DIRENT_H */

#include "common.h"
#include "content/text_object.h"

#ifdef BUILD_WAYLAND
#include "output/wl.h"
#endif /* BUILD_WAYLAND */

#ifdef BUILD_X11
#include "lua/x11-settings.h"
#include "output/x11.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wvariadic-macros"
#include <X11/Xutil.h>
#pragma GCC diagnostic pop
#ifdef BUILD_XDAMAGE
#include <X11/extensions/Xdamage.h>
#endif
#ifdef BUILD_IMLIB2
#include "conky-imlib2.h"
#endif /* BUILD_IMLIB2 */
#endif /* BUILD_X11 */

#ifdef BUILD_NCURSES
#include <ncurses.h>
#endif /* BUILD_NCURSES */

#ifdef BUILD_CURL
#include <curl/curl.h>
#endif /* BUILD_CURL */

#ifdef BUILD_RSS
#include <libxml/parser.h>
#endif /* BUILD_RSS */

/* local headers */
#include "content/colours.hh"
#include "core.h"
#include "data/exec.h"
#include "data/hardware/diskio.h"
#ifdef BUILD_GUI
#include "lua/fonts.h"
#include "output/gui.h"
#endif /* BUILD_GUI */
#include "data/fs.h"
#ifdef BUILD_ICONV
#include "data/iconv_tools.h"
#endif /* BUILD_ICONV */
#include "content/specials.h"
#include "content/temphelper.h"
#include "content/template.h"
#include "data/network/mail.h"
#include "data/network/net_stat.h"
#include "data/timeinfo.h"
#include "data/top.h"
#include "logging.h"
#include "output/nc.h"

#ifdef BUILD_MYSQL
#include "data/mysql.h"
#endif /* BUILD_MYSQL */
#ifdef BUILD_NVIDIA
#include "data/hardware/nvidia.h"
#endif /* BUILD_NVIDIA */
#ifdef BUILD_CURL
#include "data/network/ccurl_thread.h"
#endif /* BUILD_CURL */

#include "lua/llua.h"
#include "lua/lua-config.hh"
#include "lua/setting.hh"
#include "output/display-output.hh"

/* check for OS and include appropriate headers */
#if defined(__linux__)
#include "data/os/linux.h"
#elif defined(__FreeBSD__) || defined(__FreeBSD_kernel__)
#include "data/os/freebsd.h"
#elif defined(__DragonFly__)
#include "data/os/dragonfly.h"
#elif defined(__OpenBSD__)
#include "data/os/openbsd.h"
#elif defined(__NetBSD__)
#include "data/os/netbsd.h"
#endif

#include "content/gradient.hh"

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

// #define SIGNAL_BLOCKING
#undef SIGNAL_BLOCKING

/* debugging level, used by logging.h */
int global_debug_level = 0;

/* disable inotify auto reload feature if desired */
static conky::simple_config_setting<bool> disable_auto_reload(
    "disable_auto_reload", false, false);

/* two strings for internal use */
static char *tmpstring1, *tmpstring2;

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
conky::simple_config_setting<std::string> units_spacer("units_spacer", "",
                                                       false);

conky::simple_config_setting<bool> out_to_stdout("out_to_console",
// Default value is false, unless we are building without X
#ifdef BUILD_GUI
                                                 false,
#else
                                                 true,
#endif
                                                 false);
conky::simple_config_setting<bool> out_to_stderr("out_to_stderr", false, false);

int top_cpu, top_mem, top_time;
#ifdef BUILD_IOSTATS
int top_io;
#endif
int top_running;

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

#ifdef BUILD_GUI

/* text size */

conky::vec2i text_start;  /* text start position in window */
conky::vec2i text_offset; /* offset for start position */
conky::vec2i text_size =
    conky::vec2i::One(); /* initially 1 so no zero-sized window is created */

#endif /* BUILD_GUI */

/* struct that has all info to be shared between
 * instances of the same text object */
struct information info;

/* path to config file */
std::filesystem::path current_config;

/* set to 1 if you want all text to be in uppercase */
static conky::simple_config_setting<bool> stuff_in_uppercase("uppercase", false,
                                                             true);

static conky::simple_config_setting<bool> stuff_in_lowercase("lowercase", false,
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

#ifdef BUILD_GUI
/* graph */
conky::simple_config_setting<bool> show_graph_scale("show_graph_scale", false,
                                                    false);
conky::simple_config_setting<bool> show_graph_range("show_graph_range", false,
                                                    false);

enum gradient_state { RGB_GRADIENT = 0, HSV_GRADIENT, HCL_GRADIENT };
template <>
conky::lua_traits<gradient_state>::Map conky::lua_traits<gradient_state>::map =
    {{"rgb", RGB_GRADIENT}, {"hsv", HSV_GRADIENT}, {"hcl", HCL_GRADIENT}};
static conky::simple_config_setting<gradient_state> graph_gradient_mode(
    "graph_gradient_mode", RGB_GRADIENT, false);

/* Position on the screen */
conky::simple_config_setting<int> gap_x("gap_x", 5, true);
conky::simple_config_setting<int> gap_y("gap_y", 60, true);

/* border */
conky::simple_config_setting<bool> draw_borders("draw_borders", false, false);
conky::simple_config_setting<bool> draw_graph_borders("draw_graph_borders",
                                                      true, false);

conky::range_config_setting<char> stippled_borders(
    "stippled_borders", 0, std::numeric_limits<char>::max(), 0, true);

conky::simple_config_setting<bool> draw_shades("draw_shades", true, false);
conky::simple_config_setting<bool> draw_outline("draw_outline", false, false);

#ifdef OWN_WINDOW
/* fixed size/pos is set if wm/user changes them */
int fixed_size = 0, fixed_pos = 0;
#endif

conky::range_config_setting<int> minimum_height("minimum_height", 0,
                                                std::numeric_limits<int>::max(),
                                                5, true);
conky::range_config_setting<int> minimum_width("minimum_width", 0,
                                               std::numeric_limits<int>::max(),
                                               5, true);
conky::range_config_setting<int> maximum_width("maximum_width", 0,
                                               std::numeric_limits<int>::max(),
                                               0, true);

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

#endif /* BUILD_GUI */

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
  if (display_output()) return display_output()->calc_text_width(s);

  size_t slen = strlen(s);
  return slen;
}

#ifdef BUILD_GUI
conky::gradient_factory *create_gradient_factory(int width, Colour first_colour,
                                                 Colour last_colour) {
  switch (graph_gradient_mode.get(*state)) {
    case RGB_GRADIENT:
      return new conky::rgb_gradient_factory(width, first_colour, last_colour);
    case HSV_GRADIENT:
      return new conky::hsv_gradient_factory(width, first_colour, last_colour);
    case HCL_GRADIENT:
      return new conky::hcl_gradient_factory(width, first_colour, last_colour);
  }
  return nullptr;
}
#endif /* BUILD_GUI */

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
    format = "%.*f%s%.1s";
  } else {
    width = 7;
    format = "%.*f%s%-.3s";
  }
  width += strlen(units_spacer.get(*state).c_str());

  if (llabs(num) < 1000LL) {
    spaced_print(buf, size, format, width, 0, static_cast<float>(num),
                 units_spacer.get(*state).c_str(), _(*suffix));
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

  spaced_print(buf, size, format, width, precision, fnum,
               units_spacer.get(*state).c_str(), _(*suffix));
}

/* global object list root element */
static struct text_object global_root_object;

static Colour current_text_color;

void set_current_text_color(Colour colour) { current_text_color = colour; }

Colour get_current_text_color() { return current_text_color; }

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
#ifdef BUILD_GUI
    } else if (obj->callbacks.graphval != nullptr) {
      new_graph(obj, p, p_max_size, (*obj->callbacks.graphval)(obj));
#endif /* BUILD_GUI */
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
#ifdef BUILD_GUI
  /* load any new fonts we may have had */
  load_fonts(utf8_mode.get(*state));
#endif /* BUILD_GUI */
#ifdef BUILD_ICONV
  delete[] buff_in;
#endif /* BUILD_ICONV */
}

void evaluate(const char *text, char *p, int p_max_size) {
  struct text_object subroot {};

  /**
   * Consider expressions like: ${execp echo '${execp echo hi}'}
   * These would require run extract_variable_text_internal() before
   * callbacks and generate_text_internal() after callbacks.
   */
  extract_variable_text_internal(&subroot, text);
  generate_text_internal(p, p_max_size, subroot);
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
  } else if (stuff_in_lowercase.get(*state)) {
    char *tmp_p;

    tmp_p = text_buffer;
    while (*tmp_p != 0) {
      *tmp_p = tolower(static_cast<unsigned char>(*tmp_p));
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

#ifdef BUILD_GUI
int get_border_total() {
  return dpi_scale(border_inner_margin.get(*state)) +
         dpi_scale(border_outer_margin.get(*state)) +
         dpi_scale(border_width.get(*state));
}

void remove_first_char(char *s) { memmove(s, s + 1, strlen(s)); }

static int get_string_width_special(char *s, int special_index) {
  char *p, *final;
  special_node *current = specials;
  int width = 0;
  long i;

  if (s == nullptr) { return 0; }

  if (display_output() == nullptr || !display_output()->graphical()) {
    return strlen(s);
  }

  p = strndup(s, text_buffer_size.get(*state));
  final = p;

  for (i = 0; i <= special_index; i++) { current = current->next; }

  while (*p != 0) {
    if (*p == SPECIAL_CHAR) {
      /* shift everything over by 1 so that the special char
       * doesn't mess up the size calculation */
      remove_first_char(p);
      /*for (i = 0; i < static_cast<long>(strlen(p)); i++) {
        *(p + i) = *(p + i + 1);
      }*/
      if (current->type == text_node_t::GRAPH ||
          current->type == text_node_t::GAUGE ||
          current->type == text_node_t::BAR) {
        width += current->width;
      }
      if (current->type == text_node_t::FONT) {
        // put all following text until the next fontchange/stringend in
        // influenced_by_font but do not include specials
        char *influenced_by_font = strdup(p);
        special_node *current_after_font = current;
        // influenced_by_font gets special chars removed, so after this loop i
        // counts the number of letters (not special chars) influenced by font
        for (i = 0; influenced_by_font[i] != 0; i++) {
          if (influenced_by_font[i] == SPECIAL_CHAR) {
            // remove specials and stop at fontchange
            current_after_font = current_after_font->next;
            if (current_after_font->type == text_node_t::FONT) {
              influenced_by_font[i] = 0;
              break;
            }
            remove_first_char(&influenced_by_font[i]);
          }
        }
        // add the length of influenced_by_font in the new font to width
        int orig_font = selected_font;
        selected_font = current->font_added;
        width += calc_text_width(influenced_by_font);
        selected_font = orig_font;
        free(influenced_by_font);
        // make sure the chars counted in the new font are not again counted
        // in the old font
        int specials_skipped = 0;
        while (i > 0) {
          if (p[specials_skipped] != SPECIAL_CHAR) {
            remove_first_char(&p[specials_skipped]);
            // i only counts non-special chars, so only decrement it for those
            i--;
          } else {
            specials_skipped++;
          }
        }
      }
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
void update_text_area() {
  conky::vec2i xy;

  if (display_output() == nullptr || !display_output()->graphical()) { return; }

  /* update text size if it isn't fixed */
#ifdef OWN_WINDOW
  if (fixed_size == 0)
#endif
  {
    text_size = conky::vec2i(dpi_scale(minimum_width.get(*state)), 0);
    last_font_height = font_height();
    for_each_line(text_buffer, text_size_updater);

    text_size = text_size.max(conky::vec2i(text_size.x() + 1, dpi_scale(minimum_height.get(*state))));
    int mw = dpi_scale(maximum_width.get(*state));
    if (mw > 0) text_size = text_size.min(conky::vec2i(mw, text_size.y()));
  }

  alignment align = text_alignment.get(*state);
  /* get text position on workarea */
  switch (vertical_alignment(align)) {
    case axis_align::START:
      xy.set_y(workarea.y() + dpi_scale(gap_y.get(*state)));
      break;
    case axis_align::END:
    default:
      xy.set_y(workarea.end_y() - text_size.y() - dpi_scale(gap_y.get(*state)));
      break;
    case axis_align::MIDDLE:
      xy.set_y(workarea.y() + workarea.height() / 2 - text_size.y() / 2 -
               dpi_scale(gap_y.get(*state)));
      break;
  }
  switch (horizontal_alignment(align)) {
    case axis_align::START:
    default:
      xy.set_x(workarea.x() + dpi_scale(gap_x.get(*state)));
      break;
    case axis_align::END:
      xy.set_x(workarea.end_x() - text_size.x() - dpi_scale(gap_x.get(*state)));
      break;
    case axis_align::MIDDLE:
      xy.set_x(workarea.x() + workarea.width() / 2 - text_size.x() / 2 -
               dpi_scale(gap_x.get(*state)));
      break;
  }
#ifdef OWN_WINDOW
  if (align == alignment::NONE) {  // Let the WM manage the window
    xy = window.geometry.pos();
    fixed_pos = 1;
    fixed_size = 1;
  }
#endif /* OWN_WINDOW */
#ifdef OWN_WINDOW

  if (own_window.get(*state) && (fixed_pos == 0)) {
    int border_total = get_border_total();
    text_start = conky::vec2i::uniform(border_total);
    window.geometry.set_pos(xy - text_start);
  } else
#endif
  {
    text_start = xy;
  }
  /* update lua window globals */
  llua_update_window_table(conky::rect<int>(text_start, text_size));
}

/* drawing stuff */

static int cur_x, cur_y; /* current x and y for drawing */
#endif
// draw_mode also without BUILD_GUI because we only need to print to stdout with
// FG
static draw_mode_t draw_mode; /* FG, BG or OUTLINE */
#ifdef BUILD_GUI
/*static*/ Colour current_color;

static int text_size_updater(char *s, int special_index) {
  int w = 0;
  char *p;
  special_node *current = specials;

  for (int i = 0; i < special_index; i++) { current = current->next; }

  if (display_output() == nullptr || !display_output()->graphical()) {
    return 0;
  }
  /* get string widths and skip specials */
  p = s;
  while (*p != 0) {
    if (*p == SPECIAL_CHAR) {
      *p = '\0';
      w += get_string_width(s);
      *p = SPECIAL_CHAR;

      if (current->type == text_node_t::BAR ||
          current->type == text_node_t::GAUGE ||
          current->type == text_node_t::GRAPH) {
        w += current->width;
        if (current->height > last_font_height) {
          last_font_height = current->height;
          last_font_height += font_height();
        }
      } else if (current->type == text_node_t::OFFSET) {
        if (current->arg > 0) { w += current->arg; }
      } else if (current->type == text_node_t::VOFFSET) {
        last_font_height += current->arg;
      } else if (current->type == text_node_t::GOTO) {
        if (current->arg > cur_x) { w = static_cast<int>(current->arg); }
      } else if (current->type == text_node_t::TAB) {
        int start = current->arg;
        int step = current->width;

        if ((step == 0) || step < 0) { step = 10; }
        w += step - (cur_x - text_start.x() - start) % step;
      } else if (current->type == text_node_t::FONT) {
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

  if (w > text_size.x()) { text_size.set_x(w); }
  int mw = dpi_scale(maximum_width.get(*state));
  if (mw > 0) { text_size.set_x(std::min(mw, text_size.x())); }

  text_size += conky::vec2i(0, last_font_height);
  last_font_height = font_height();
  return special_index;
}
#endif /* BUILD_GUI */

static inline void set_foreground_color(Colour c) {
  for (auto output : display_outputs()) output->set_foreground_color(c);
}

static inline void draw_graph_bars(special_node *current, std::unique_ptr<Colour[]>& tmpcolour, 
                            conky::vec2i& text_offset, int i, int &j, int w, 
                            int colour_idx, int cur_x, int by, int h) {
  double graphheight = current->graph[j] * (h - 1) / current->scale;
  /* Check if graphheight is less than the minheight threshold, if so we must change it to the threshold */
  if(graphheight > 0 && current->minheight - graphheight > 0) {
    current->graph[j] = current->minheight * current->scale / (h - 1);
  }
  if (current->colours_set) {
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
  /* Handle the case where y axis is to be inverted */
  int offsety1 = current->inverty ? by : by + h;
  int offsety2 = current->inverty ? by +  current->graph[j] * (h - 1) / current->scale
                          : round_to_positive_int(static_cast<double>(by) + h -
                          current->graph[j] * (h - 1) /
                          current->scale);
  /* this is mugfugly, but it works */
  if (display_output()) {
    display_output()->draw_line(
        text_offset.x() + cur_x + i + 1, text_offset.y() + offsety1,
        text_offset.x() + cur_x + i + 1, text_offset.y() + offsety2);
  }
  ++j;
}

static void draw_string(const char *s) {
  int i;
  int i2;
  int pos;
#ifdef BUILD_GUI
  int width_of_s;
#endif /* BUILD_GUI */
  int max = 0;
  int added;

  if (s[0] == '\0') { return; }

#ifdef BUILD_GUI
  width_of_s = get_string_width(s);
#endif /* BUILD_GUI */
  if (draw_mode == draw_mode_t::FG) {
    for (auto output : display_outputs())
      if (!output->graphical()) output->draw_string(s, 0);
  }
  int tbs = text_buffer_size.get(*state);
  memset(tmpstring1, 0, tbs);
  memset(tmpstring2, 0, tbs);
  strncpy(tmpstring1, s, tbs - 1);
  pos = 0;
  added = 0;

#ifdef BUILD_GUI
  if (display_output() && display_output()->graphical()) {
    max = ((text_size.x() - width_of_s) / std::max(1, get_string_width(" ")));
  }
#endif /* BUILD_GUI */
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
#ifdef BUILD_GUI
  if (display_output() && display_output()->graphical()) {
    int mw = dpi_scale(maximum_width.get(*state));
    if (text_size.x() == mw) {
      /* this means the text is probably pushing the limit,
       * so we'll chop it */
    }
  }
#endif /* BUILD_GUI */
  s = tmpstring2;
#ifdef BUILD_GUI
  if (display_output() && display_output()->graphical()) {
    display_output()->draw_string_at(text_offset.x() + cur_x,
                                     text_offset.y() + cur_y, s, strlen(s));

    cur_x += width_of_s;
  }
#endif /* BUILD_GUI */
  memcpy(tmpstring1, s, tbs);
}

#if defined(BUILD_MATH) && defined(BUILD_GUI)
/// Format \a size as a real followed by closest SI unit, with \a prec
/// number of digits after the decimal point.
static std::string formatSizeWithUnits(double size, int prec = 1) {
  int div = 0;
  double rem = 0;

  while (size >= 1024.0 &&
         static_cast<size_t>(div) < (sizeof suffixes / sizeof *suffixes)) {
    rem = fmod(size, 1024.0);
    div++;
    size /= 1024.0;
  }

  double size_d = size + rem / 1024.0;

  std::ostringstream result;
  result.setf(std::ios::fixed, std::ios::floatfield);
  result.precision(prec);
  result << size_d;
  result << " ";

  if (short_units.get(*state)) {
    result << suffixes[div][0];
  } else {
    result << suffixes[div];
  }

  return result.str();
}
#endif /* BUILD_MATH && BUILD_GUI */

int draw_each_line_inner(char *s, int special_index, int last_special_applied) {
#ifndef BUILD_GUI
  static int cur_x, cur_y; /* current x and y for drawing */
  (void)cur_y;
#endif
#ifdef BUILD_GUI
  int font_h = 0;
  int cur_y_add = 0;
  int mw = dpi_scale(maximum_width.get(*state));
#endif /* BUILD_GUI */
  char *p = s;
  int orig_special_index = special_index;

#ifdef BUILD_GUI
  if (display_output() && display_output()->graphical()) {
    font_h = font_height();
    cur_y += font_ascent();
  }
  cur_x = text_start.x();
#endif /* BUILD_GUI */

  while (*p != 0) {
    if (*p == SPECIAL_CHAR || last_special_applied > -1) {
#ifdef BUILD_GUI
      int w = 0;
#endif /* BUILD_GUI */

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
      special_node *current = specials;
      for (int i = 0; i < special_index; i++) { current = current->next; }
      switch (current->type) {
#ifdef BUILD_GUI
        case text_node_t::HORIZONTAL_LINE:
          if (display_output() && display_output()->graphical()) {
            int h = current->height;
            int mid = font_ascent() / 2;

            w = text_start.x() + text_size.x() - cur_x;

            if (display_output()) {
              display_output()->set_line_style(h, true);
              display_output()->draw_line(text_offset.x() + cur_x,
                                          text_offset.y() + cur_y - mid / 2,
                                          text_offset.x() + cur_x + w,
                                          text_offset.y() + cur_y - mid / 2);
            }
          }
          break;

        case text_node_t::STIPPLED_HR:
          if (display_output() && display_output()->graphical()) {
            int h = current->height;
            char tmp_s = current->arg;
            int mid = font_ascent() / 2;
            char ss[2] = {tmp_s, tmp_s};

            w = text_start.x() + text_size.x() - cur_x - 1;
            if (display_output()) {
              display_output()->set_line_style(h, false);
              display_output()->set_dashes(ss);
              display_output()->draw_line(text_offset.x() + cur_x,
                                          text_offset.y() + cur_y - mid / 2,
                                          text_offset.x() + cur_x + w,
                                          text_offset.x() + cur_y - mid / 2);
            }
          }
          break;

        case text_node_t::BAR:
          if (display_output() && display_output()->graphical()) {
            int h, by;
            double bar_usage, scale;
            if (cur_x - text_start.x() > mw && mw > 0) { break; }
            h = current->height;
            bar_usage = current->arg;
            scale = current->scale;
            by = cur_y - (font_ascent() / 2) - 1;

            if (h < font_h) { by -= h / 2 - 1; }
            w = current->width;
            if (w == 0) { w = text_start.x() + text_size.x() - cur_x - 1; }
            if (w < 0) { w = 0; }

            if (display_output()) {
              display_output()->set_line_style(dpi_scale(1), true);

              display_output()->draw_rect(text_offset.x() + cur_x,
                                          text_offset.y() + by, w, h);
              display_output()->fill_rect(text_offset.x() + cur_x,
                                          text_offset.y() + by,
                                          w * bar_usage / scale, h);
            }
            if (h > cur_y_add && h > font_h) { cur_y_add = h; }
          }
          break;

        case text_node_t::GAUGE: /* new GAUGE  */
          if (display_output() && display_output()->graphical()) {
            int h, by = 0;
            Colour last_colour = current_color;
#ifdef BUILD_MATH
            float angle, px, py;
            double usage, scale;
#endif /* BUILD_MATH */

            if (cur_x - text_start.x() > mw && mw > 0) { break; }

            h = current->height;
            by = cur_y - (font_ascent() / 2) - 1;

            if (h < font_h) { by -= h / 2 - 1; }
            w = current->width;
            if (w == 0) { w = text_start.x() + text_size.x() - cur_x - 1; }
            if (w < 0) { w = 0; }

            if (display_output()) {
              display_output()->set_line_style(1, true);
              display_output()->draw_arc(text_offset.x() + cur_x,
                                         text_offset.y() + by, w, h * 2, 0,
                                         180 * 64);
            }

#ifdef BUILD_MATH
            usage = current->arg;
            scale = current->scale;
            angle = M_PI * usage / scale;
            px = static_cast<float>(cur_x + (w / 2.)) -
                 static_cast<float>(w / 2.) * cos(angle);
            py = static_cast<float>(by + (h)) -
                 static_cast<float>(h) * sin(angle);

            if (display_output()) {
              display_output()->draw_line(
                  text_offset.x() + cur_x + (w / 2.),
                  text_offset.y() + by + (h),
                  text_offset.x() + static_cast<int>(px),
                  text_offset.y() + static_cast<int>(py));
            }

#endif /* BUILD_MATH */

            if (h > cur_y_add && h > font_h) { cur_y_add = h; }

            set_foreground_color(last_colour);
          }
          break;

        case text_node_t::GRAPH:
          if (display_output() && display_output()->graphical()) {
            int h, by, i = 0, j = 0;
            int colour_idx = 0;
            Colour last_colour = current_color;
            if (cur_x - text_start.x() > mw && mw > 0) { break; }
            h = current->height;
            by = cur_y - (font_ascent() / 2) - 1;

            if (h < font_h) { by -= h / 2 - 1; }
            w = current->width;
            if (w == 0) {
              w = text_start.x() + text_size.x() - cur_x - 1;
              current->graph_width = std::max(w - 1, 0);
              if (current->graph_width != current->graph_allocated) {
                w = current->graph_allocated + 1;
              }
            }
            if (w < 0) { w = 0; }
            if (draw_graph_borders.get(*state)) {
              if (display_output()) {
                display_output()->set_line_style(dpi_scale(1), true);
                display_output()->draw_rect(text_offset.x() + cur_x,
                                            text_offset.y() + by, w, h);
              }
            }
            if (display_output()) display_output()->set_line_style(1, true);

            /* in case we don't have a graph yet */
            if (current->graph != nullptr) {
              std::unique_ptr<Colour[]> tmpcolour;

              if (current->colours_set) {
                auto factory = create_gradient_factory(w, current->last_colour,
                                                       current->first_colour);
                tmpcolour = factory->create_gradient();
                delete factory;
              }
              colour_idx = 0;
              if(current->invertx){
                for (i = 0; i <= w - 2; i++) {
                  draw_graph_bars(current, tmpcolour, text_offset, 
                  i, j, w, colour_idx, cur_x, by, h);
                }
              }
              else{
                for (i = w - 2; i > -1; i--) {
                  draw_graph_bars(current, tmpcolour, text_offset, 
                  i, j, w, colour_idx, cur_x, by, h);
                }
              }
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
              if (current->colours_set) {
                  // Set the foreground colour to the first colour, ensures the scale text is always drawn in the same colour
                  set_foreground_color(current->first_colour);
              }
              int tmp_x = cur_x;
              int tmp_y = cur_y;
              cur_x += font_ascent() / 2;
              cur_y += font_h / 2;
              std::string tmp_str = formatSizeWithUnits(
                  current->scale_log != 0 ? std::pow(10.0, current->scale)
                                          : current->scale);
              draw_string(tmp_str.c_str());
              cur_x = tmp_x;
              cur_y = tmp_y;
            }
#endif
            set_foreground_color(last_colour);
          }
          break;

        case text_node_t::FONT:
          if (display_output() && display_output()->graphical()) {
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
#endif /* BUILD_GUI */
        case text_node_t::FG:
          if (draw_mode == draw_mode_t::FG) {
            set_foreground_color(Colour::from_argb32(current->arg));
          }
          break;

#ifdef BUILD_GUI
        case text_node_t::BG:
          if (draw_mode == draw_mode_t::BG) {
            set_foreground_color(Colour::from_argb32(current->arg));
          }
          break;

        case text_node_t::OUTLINE:
          if (draw_mode == draw_mode_t::OUTLINE) {
            set_foreground_color(Colour::from_argb32(current->arg));
          }
          break;

        case text_node_t::OFFSET:
          w += current->arg;
          break;

        case text_node_t::VOFFSET:
          cur_y += current->arg;
          break;

        case text_node_t::SAVE_COORDINATES:
#ifdef BUILD_IMLIB2
          saved_coordinates[static_cast<int>(current->arg)] =
              std::array<int, 2>{cur_x - text_start.x(),
                                 cur_y - text_start.y() - last_font_height};
#endif /* BUILD_IMLIB2 */
          break;

        case text_node_t::TAB: {
          int start = current->arg;
          int step = current->width;

          if ((step == 0) || step < 0) { step = 10; }
          w = step - (cur_x - text_start.x() - start) % step;
          break;
        }

        case text_node_t::ALIGNR: {
          /* TODO: add back in "+ window.border_inner_margin" to the end of
           * this line? */
          int pos_x = text_start.x() + text_size.x() -
                      get_string_width_special(s, special_index);

          /* printf("pos_x %i text_start.x %i text_size.x %i cur_x %i "
            "get_string_width(p) %i gap_x %i "
            "current->arg %i window.border_inner_margin %i "
            "window.border_width %i\n", pos_x, text_start.x, text_size.x,
            cur_x, get_string_width_special(s), gap_x,
            current->arg, window.border_inner_margin,
            window.border_width); */
          cur_x = pos_x - current->arg;
          break;
        }

        case text_node_t::ALIGNC: {
          int pos_x = text_size.x() / 2 -
                      get_string_width_special(s, special_index) / 2 -
                      (cur_x - text_start.x());
          /* int pos_x = text_start_x + text_size.x / 2 -
            get_string_width_special(s) / 2; */

          /* printf("pos_x %i text_start.x %i text_size.x %i cur_x %i "
            "get_string_width(p) %i gap_x %i "
            "current->arg %i\n", pos_x, text_start.x,
            text_size.x, cur_x, get_string_width(s), gap_x,
            current->arg); */
          if (pos_x > current->arg) { w = pos_x - current->arg; }
          break;
        }
#endif /* BUILD_GUI */
        case text_node_t::GOTO:
          if (current->arg >= 0) {
#ifdef BUILD_GUI
            cur_x = static_cast<int>(current->arg);
            // make sure shades are 1 pixel to the right of the text
            if (draw_mode == draw_mode_t::BG) { cur_x++; }
#endif /* BUILD_GUI */
            cur_x = static_cast<int>(current->arg);
            for (auto output : display_outputs()) output->gotox(cur_x);
          }
          break;
        default:
          // do nothing; not a special node or support not enabled
          break;
      }

#ifdef BUILD_GUI
      cur_x += w;
#endif /* BUILD_GUI */

      if (special_index != last_special_applied) {
        special_index++;
      } else {
        special_index = orig_special_index;
        last_special_applied = -1;
      }
    }
    p++;
  }

#ifdef BUILD_GUI
  cur_y += cur_y_add;
#endif /* BUILD_GUI */
  draw_string(s);
  for (auto output : display_outputs()) output->line_inner_done();
#ifdef BUILD_GUI
  if (display_output() && display_output()->graphical()) {
    cur_y += font_descent();
  }
#endif /* BUILD_GUI */
  return special_index;
}

static int draw_line(char *s, int special_index) {
  if (display_output() && display_output()->draw_line_inner_required()) {
    return draw_each_line_inner(s, special_index, -1);
  }
  draw_string(s);
  UNUSED(special_index);
  return 0;
}

static void draw_text() {
  for (auto output : display_outputs()) output->begin_draw_text();
#ifdef BUILD_GUI
  // XXX:only works if inside set_display_output()
  for (auto output : display_outputs()) {
    if (output && output->graphical()) {
      cur_y = text_start.y();
      int bw = dpi_scale(border_width.get(*state));

      /* draw borders */
      if (draw_borders.get(*state) && bw > 0) {
        if (stippled_borders.get(*state) != 0) {
          char ss[2] = {(char)dpi_scale(stippled_borders.get(*state)),
                        (char)dpi_scale(stippled_borders.get(*state))};
          output->set_line_style(bw, false);
          output->set_dashes(ss);
        } else {
          output->set_line_style(bw, true);
        }

        int offset = dpi_scale(border_inner_margin.get(*state)) + bw;
        output->draw_rect(text_offset.x() + text_start.x() - offset,
                          text_offset.y() + text_start.y() - offset,
                          text_size.x() + 2 * offset,
                          text_size.y() + 2 * offset);
      }

      /* draw text */
    }
  }
  setup_fonts();
#endif /* BUILD_GUI */
  for_each_line(text_buffer, draw_line);
  for (auto output : display_outputs()) output->end_draw_text();
}

void draw_stuff() {
  for (auto output : display_outputs()) output->begin_draw_stuff();

#ifdef BUILD_GUI
  llua_draw_pre_hook();

#ifdef BUILD_IMLIB2
  text_offset = conky::vec2i::Zero();
  cimlib_render(text_start.x(), text_start.y(), window.geometry.width(),
                window.geometry.height(),
                imlib_cache_flush_interval.get(*state),
                imlib_draw_blended.get(*state));
#endif /* BUILD_IMLIB2 */

  for (auto output : display_outputs()) {
    if (!output->graphical()) continue;
    // XXX: we assume a single graphical display
    set_display_output(output);

    selected_font = 0;
    if (draw_shades.get(*state) && !draw_outline.get(*state)) {
      text_offset = conky::vec2i::One();
      set_foreground_color(default_shade_color.get(*state));
      draw_mode = draw_mode_t::BG;
      draw_text();
      text_offset = conky::vec2i::Zero();
    }

    if (draw_outline.get(*state)) {
      selected_font = 0;

      for (int ix = -1; ix < 2; ix++) {
        for (int iy = -1; iy < 2; iy++) {
          if (ix == 0 && iy == 0) { continue; }
          text_offset = conky::vec2i(ix, iy);
          set_foreground_color(default_outline_color.get(*state));
          draw_mode = draw_mode_t::OUTLINE;
          draw_text();
        }
      }
      text_offset = conky::vec2i::Zero();
    }

    selected_font = 0;
    set_foreground_color(default_color.get(*state));
    unset_display_output();
  }

#endif /* BUILD_GUI */
  // always draw text
  draw_mode = draw_mode_t::FG;
  draw_text();
#ifdef BUILD_GUI

  llua_draw_post_hook();
#endif /* BUILD_GUI */

  for (auto output : display_outputs()) output->end_draw_stuff();
}

int need_to_update;

/* update_text() generates new text and clears old text area */
void update_text() {
#ifdef BUILD_IMLIB2
  cimlib_cleanup();
#endif /* BUILD_IMLIB2 */
  generate_text();
#ifdef BUILD_GUI
  for (auto output : display_outputs()) {
    if (output->graphical()) output->clear_text(1);
  }
#endif /* BUILD_GUI */
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

void log_system_details() {
  char *session_ty = getenv("XDG_SESSION_TYPE");
  char *session = getenv("GDMSESSION");
  char *desktop = getenv("XDG_CURRENT_DESKTOP");
  if (desktop != nullptr || session != nullptr) {
    NORM_ERR("'%s' %s session running '%s' desktop", session, session_ty,
             desktop);
  }
}

void main_loop() {
  int terminate = 0;
#ifdef SIGNAL_BLOCKING
  sigset_t newmask, oldmask;
#endif
#ifdef BUILD_GUI
  double t;
#endif /* BUILD_GUI */
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

  log_system_details();

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
      CRIT_ERR("unable to sigprocmask()");
    }
#endif

#ifdef BUILD_GUI
    if (display_output() && display_output()->graphical()) {
      t = next_update_time - get_time();
      display_output()->main_loop_wait(t);
    } else {
#endif /* BUILD_GUI */
      struct timespec req, rem;
      auto time_to_sleep = next_update_time - get_time();
      auto seconds = static_cast<time_t>(std::floor(time_to_sleep));
      auto nanos = (time_to_sleep - seconds) * 1000000000L;
      req.tv_sec = seconds;
      req.tv_nsec = nanos;
      nanosleep(&req, &rem);
      update_text();
      draw_stuff();
      for (auto output : display_outputs()) output->flush();
#ifdef BUILD_GUI
    }
#endif /* BUILD_GUI */

#ifdef SIGNAL_BLOCKING
    /* unblock signals of interest and let handler fly */
    if (sigprocmask(SIG_SETMASK, &oldmask, nullptr) < 0) {
      CRIT_ERR("unable to sigprocmask()");
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
      NORM_ERR("received SIGUSR2. refreshing.");
      update_text();
      draw_stuff();
      for (auto output : display_outputs()) output->flush();
    }

    if (g_sigterm_pending != 0) {
      g_sigterm_pending = 0;
      NORM_ERR("received SIGHUP, SIGINT, or SIGTERM to terminate. bye!");
      terminate = 1;
      for (auto output : display_outputs()) output->sigterm_cleanup();
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
  clean_up();

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
  clean_up();
  state = std::make_unique<lua::state>();
  conky::export_symbols(*state);
  sleep(1); /* slight pause */
  initialisation(argc_copy, argv_copy);
}

void free_specials(special_node *&current) {
  if (current != nullptr) {
    free_specials(current->next);
    if (current->type == text_node_t::GRAPH) { free(current->graph); }
    delete current;
    current = nullptr;
  }

  clear_stored_graphs();
}

void clean_up(void) {
  /* free_update_callbacks(); XXX: some new equivalent of this? */
  free_and_zero(info.cpu_usage);
  for (auto output : display_outputs()) output->cleanup();
  conky::shutdown_display_outputs();
#ifdef BUILD_GUI
  if (!display_output() || !display_output()->graphical()) {
    fonts.clear();  // in set_default_configurations a font is set but not
                    // loaded
    selected_font = 0;
  }
#endif /* BUILD_GUI */

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

/* Enable a single output by default based on what was enabled at build-time */
#ifdef BUILD_WAYLAND
  state->pushboolean(true);
  out_to_wayland.lua_set(*state);
#else
#ifdef BUILD_X11
  state->pushboolean(true);
  out_to_x.lua_set(*state);
#else
  state->pushboolean(true);
  out_to_stdout.lua_set(*state);
#endif
#endif

  info.users.number = 1;
}

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
    defined(__OpenBSD__) || defined(__DragonFly__)
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
#if defined(__linux__) || defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || \
    defined(__HAIKU__) || defined(__NetBSD__) || defined(__OpenBSD__)
    "U"
#endif /* Linux || FreeBSD || Haiku || NetBSD || OpenBSD */
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
    {"help", 0, nullptr, 'h'},          {"version", 0, nullptr, 'v'},
    {"short-version", 0, nullptr, 'V'}, {"quiet", 0, nullptr, 'q'},
    {"debug", 0, nullptr, 'D'},         {"config", 1, nullptr, 'c'},
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
    {"pause", 1, nullptr, 'p'},
#if defined(__linux__) || defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || \
    defined(__HAIKU__) || defined(__NetBSD__) || defined(__OpenBSD__)
    {"unique", 0, nullptr, 'U'},
#endif /* Linux || FreeBSD || Haiku || NetBSD || OpenBSD */
    {nullptr, 0, nullptr, 0}
};

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
  llua_init();
  load_config_file();

  /* handle other command line arguments */

  reset_optind();

#if defined(__FreeBSD__) || defined(__FreeBSD_kernel__)
  if ((kd = kvm_open("/dev/null", "/dev/null", "/dev/null", O_RDONLY,
                     "kvm_open")) == nullptr) {
    CRIT_ERR("cannot read kvm");
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
          CRIT_ERR("'%s' is a wrong xinerama-head index", optarg);
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
        state->pushnumber(strtod(optarg, &conv_end));
        if (*conv_end != 0) {
          CRIT_ERR("'%s' is an invalid update interval", optarg);
        }
        update_interval.lua_set(*state);
        break;

      case 'i':
        state->pushinteger(strtol(optarg, &conv_end, 10));
        if (*conv_end != 0) {
          CRIT_ERR("'%s' is an invalid number of update times", optarg);
        }
        total_run_times.lua_set(*state);
        break;
#ifdef BUILD_X11
      case 'x':
        state->pushinteger(strtol(optarg, &conv_end, 10));
        if (*conv_end != 0) {
          CRIT_ERR("'%s' is an invalid value for the X-position", optarg);
        }
        gap_x.lua_set(*state);
        break;

      case 'y':
        state->pushinteger(strtol(optarg, &conv_end, 10));
        if (*conv_end != 0) {
          CRIT_ERR("'%s' is a wrong value for the Y-position", optarg);
        }
        gap_y.lua_set(*state);
        break;
#endif /* BUILD_X11 */
      case 'p':
        if (first_pass != 0) {
          startup_pause = strtol(optarg, nullptr, 10);
          sleep(startup_pause);
        }
        break;

      case '?':
        throw unknown_arg_throw();
    }
  }

  conky::set_config_settings(*state);

#ifdef BUILD_GUI
  if (display_output() && display_output()->graphical()) {
    current_text_color = default_color.get(*state);
  }
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

  if (!conky::initialize_display_outputs()) {
    CRIT_ERR("initialize_display_outputs() failed.");
  }
#ifdef BUILD_GUI
  /* setup lua window globals */
  llua_setup_window_table(conky::rect<int>(text_start, text_size));
#endif /* BUILD_GUI */

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
