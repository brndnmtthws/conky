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

#include "common.h"

#include <fcntl.h>
#include <net/if.h>
#include <netinet/in.h>
#include <pthread.h>
#include <pwd.h>
#include <semaphore.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <cctype>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <string>
#include <vector>

#include "config.h"
#include "conky.h"
#include "content/specials.h"
#include "content/temphelper.h"
#include "core.h"
#include "data/fs.h"
#include "data/misc.h"
#include "data/network/net_stat.h"
#include "data/timeinfo.h"
#include "data/top.h"
#include "logging.h"
#include "parse/variables.hh"

#if defined(_POSIX_C_SOURCE) && !defined(__OpenBSD__) && !defined(__HAIKU__)
#include <wordexp.h>
#endif

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
#elif defined(__APPLE__) && defined(__MACH__)
#include "data/os/darwin.h"  // strings.h
#endif

#include "update-cb.hh"

#ifdef BUILD_CURL
#include "data/network/ccurl_thread.h"
#endif /* BUILD_CURL */

/* folds a string over top of itself, like so:
 *
 * if start is "blah", and you call it with count = 1, the result will be "lah"
 */
void strfold(char *start, int count) {
  char *curplace;
  for (curplace = start + count; *curplace != 0; curplace++) {
    *(curplace - count) = *curplace;
  }
  *(curplace - count) = 0;
}

#ifndef HAVE_STRNDUP
// use our own strndup() if it's not available
char *strndup(const char *s, size_t n) {
  if (strlen(s) > n) {
    char *ret = malloc(n + 1);
    strncpy(ret, s, n);
    ret[n] = 0;
    return ret;
  } else {
    return strdup(s);
  }
}
#endif /* HAVE_STRNDUP */

int update_uname() {
  uname(&info.uname_s);

#if defined(__DragonFly__)
  {
    size_t desc_n;
    char desc[256];

    if (sysctlbyname("kern.version", nullptr, &desc_n, NULL, 0) == -1 ||
        sysctlbyname("kern.version", desc, &desc_n, nullptr, 0) == -1)
      LOG_ERROR("kern.version: {}", strerror(errno));
    else {
      char *start = desc;
      strsep(&start, " ");
      strcpy(info.uname_v, strsep(&start, " "));
    }

    if (errno == ENOMEM) printf("desc_n %zu\n", desc_n);
  }
#endif

  return 0;
}

double get_time() {
  struct timespec tv {};
#ifdef _POSIX_MONOTONIC_CLOCK
  clock_gettime(CLOCK_MONOTONIC, &tv);
#else
  clock_gettime(CLOCK_REALTIME, &tv);
#endif
  return tv.tv_sec + (tv.tv_nsec * 1e-9);
}

#if defined(_POSIX_C_SOURCE) && !defined(__OpenBSD__) && !defined(__HAIKU__)
std::filesystem::path to_real_path(const std::string &source) {
  wordexp_t p;
  char **w;
  int i;
  std::string checked = std::string(source);
  std::string::size_type n = 0;
  while ((n = checked.find(" ", n)) != std::string::npos) {
    checked.replace(n, 1, "\\ ");
    n += 2;
  }
  const char *csource = source.c_str();
  if (wordexp(checked.c_str(), &p, 0) != 0) { return std::string(); }
  w = p.we_wordv;
  const char *resolved_path = strdup(w[0]);
  wordfree(&p);
  return std::filesystem::weakly_canonical(resolved_path);
}
#else
// TODO: Use this implementation once it's finished.
// `wordexp` calls shell which is inconsistent across different environments.
std::filesystem::path to_real_path(const std::string &source) {
  /*
  Wordexp (via default shell) does:
  - [x] tilde substitution `~`
  - [x] variable substitution (via `variable_substitute`)
  - [ ] command substitution `$(command)`
    - exec.cc does execution already; missing recursive descent parser for
      $(...) because they can be nested and mixed with self & other expressions
      from this list
  - [ ] [arithmetic
    expansion](https://www.gnu.org/software/bash/manual/html_node/Arithmetic-Expansion.html)
    `$((10 + 2))`
    - would be nice to have for other things as well, could possibly use lua and
      replace stuff like $VAR and $(...) with equivalent functions.
  - [ ] [field
    splitting](https://www.gnu.org/software/bash/manual/html_node/Word-Splitting.html)
  - [ ] wildcard expansion
  - [ ] quote removal Extra:
  - canonicalization added
  */
  try {
    std::string input = tilde_expand(source);
    std::string expanded = variable_substitute(input);
    std::filesystem::path absolute = std::filesystem::absolute(expanded);
    return std::filesystem::weakly_canonical(absolute);
  } catch (const std::filesystem::filesystem_error &e) {
    // file not found or permission issues
    LOG_WARNING("can't canonicalize path '{}': {}", source, e.what());
    return source;
  }
}
#endif

int open_fifo(const char *file, int *reported) {
  int fd = 0;

  fd = open(file, O_RDONLY | O_NONBLOCK | O_CLOEXEC);

  if (fd == -1) {
    if ((reported == nullptr) || *reported == 0) {
      LOG_ERROR("can't open fifo '{}': {}", file, strerror(errno));
      if (reported != nullptr) { *reported = 1; }
    }
    return -1;
  }

  return fd;
}

FILE *open_file(const char *file, int *reported) {
  FILE *fp = nullptr;

  fp = fopen(file, "re");

  if (fp == nullptr) {
    if ((reported == nullptr) || *reported == 0) {
      LOG_ERROR("can't open file '{}': {}", file, strerror(errno));
      if (reported != nullptr) { *reported = 1; }
    }
    return nullptr;
  }

  return fp;
}

std::filesystem::path get_cwd() {
  char *cwd;
  char buffer[1024];

  // Attempt to get the current working directory
  cwd = getcwd(buffer, sizeof(buffer));
  if (cwd == NULL) {
    LOG_ERROR("can't get current working directory: {}", strerror(errno));
    LOG_DEBUG("returning '.' as PWD fallback");
    return std::string(".");
  }

  return std::string(buffer);
}

std::string current_username() {
  const char *user = std::getenv("USER");

  if (!user) {
    LOG_ERROR("can't determine current user (USER environment variable not set)");
    return std::string();
  }

  return std::string(user);
}

std::optional<std::filesystem::path> user_home(const std::string &username) {
  if (username == current_username()) {
    const char *home = std::getenv("HOME");
    if (home) { return std::filesystem::path(home); }
  }

  struct passwd *pw = getpwnam(username.c_str());
  if (!pw) {
    LOG_DEBUG("can't determine HOME directory for user '{}' (neither w/ HOME nor getpwnam)", username);
    return std::nullopt;
  }
  return std::filesystem::path(pw->pw_dir);
}
std::optional<std::filesystem::path> user_home() {
  return user_home(current_username());
}

std::string tilde_expand(const std::string &unexpanded) {
  if (unexpanded.compare(0, 1, "~") != 0) { return unexpanded; }
  if (unexpanded.length() == 1) {
    auto home = user_home();
    if (home->empty()) {
      LOG_WARNING("can't expand '~' path because user_home couldn't locate home directory");
      return unexpanded;
    }
    return home.value();
  }
  char next = unexpanded.at(1);
  if (next == '/') {
    auto home = user_home();
    if (home->empty()) {
      LOG_WARNING("can't expand '~' path because user_home couldn't locate home directory");
      return unexpanded;
    }
    return home.value().string() + unexpanded.substr(1);
  }
  if (next == '+') { return get_cwd().string() + unexpanded.substr(2); }
  // if (next == '-') {
  //   auto oldpwd = std::getenv("OLDPWD");
  //   if (oldpwd == nullptr) {
  //     return unexpanded;
  //   }
  //   return std::string(oldpwd) + unexpanded.substr(2);
  // }
  // ~+/-N is tied to bash functionality
  if (std::isalpha(next)) {  // handles ~USERNAME
    auto name_end = unexpanded.find_first_of('/', 1);
    std::string name;
    if (name_end == std::string::npos) {
      name = unexpanded.substr(1);
      name_end = unexpanded.length();
    } else {
      name = unexpanded.substr(1, name_end - 1);
    }
    auto home = user_home(name);
    if (home->empty()) { return unexpanded; }
    return home.value().string() + unexpanded.substr(name_end);
  }
  return unexpanded;
}

std::string variable_substitute(std::string s) {
  std::string::size_type pos = 0;
  while ((pos = s.find('$', pos)) != std::string::npos) {
    if (pos + 1 >= s.size()) { break; }

    if (s[pos + 1] == '$') {
      s.erase(pos, 1);
      ++pos;
    } else {
      std::string var;
      std::string::size_type l = 0;

      if (isalpha(static_cast<unsigned char>(s[pos + 1])) != 0) {
        l = 1;
        while (pos + l < s.size() &&
               (isalnum(static_cast<unsigned char>(s[pos + l])) != 0)) {
          ++l;
        }
        var = s.substr(pos + 1, l - 1);
      } else if (s[pos + 1] == '{') {
        l = s.find('}', pos);
        if (l == std::string::npos) { break; }
        l -= pos - 1;
        var = s.substr(pos + 2, l - 3);
      } else {
        ++pos;
      }

      if (l != 0u) {
        s.erase(pos, l);
        const char *val = getenv(var.c_str());
        if (val != nullptr) {
          s.insert(pos, val);
          pos += strlen(val);
        }
      }
    }
  }

  return s;
}

void format_seconds(char *buf, unsigned int n, long seconds) {
  long days;
  int hours, minutes;

  if (times_in_seconds.get(*state)) {
    snprintf(buf, n, "%ld", seconds);
    return;
  }

  days = seconds / 86400;
  seconds %= 86400;
  hours = seconds / 3600;
  seconds %= 3600;
  minutes = seconds / 60;
  seconds %= 60;

  if (days > 0) {
    snprintf(buf, n, "%ldd %dh %dm", days, hours, minutes);
  } else {
    snprintf(buf, n, "%dh %dm %lds", hours, minutes, seconds);
  }
}

void format_seconds_short(char *buf, unsigned int n, long seconds) {
  long days;
  int hours, minutes;

  if (times_in_seconds.get(*state)) {
    snprintf(buf, n, "%ld", seconds);
    return;
  }

  days = seconds / 86400;
  seconds %= 86400;
  hours = seconds / 3600;
  seconds %= 3600;
  minutes = seconds / 60;
  seconds %= 60;

  if (days > 0) {
    snprintf(buf, n, "%ldd %dh", days, hours);
  } else if (hours > 0) {
    snprintf(buf, n, "%dh %dm", hours, minutes);
  } else {
    snprintf(buf, n, "%dm %lds", minutes, seconds);
  }
}

conky::simple_config_setting<bool> no_buffers("no_buffers", true, true);
conky::simple_config_setting<std::string> bar_fill("console_bar_fill", "#",
                                                   false);
conky::simple_config_setting<std::string> bar_unfill("console_bar_unfill", ".",
                                                     false);
conky::simple_config_setting<std::string> github_token("github_token", "",
                                                       false);

void update_stuff() {
  /* clear speeds, addresses and up status in case device was removed and
   *  doesn't get updated */

#ifdef HAVE_OPENMP
#pragma omp parallel for schedule(dynamic, 10)
#endif /* HAVE_OPENMP */
  for (int i = 0; i < MAX_NET_INTERFACES; ++i) {
    if (netstats[i].dev != nullptr) {
      netstats[i].up = 0;
      netstats[i].recv_speed = 0.0;
      netstats[i].trans_speed = 0.0;
      netstats[i].addr.sa_data[2] = 0;
      netstats[i].addr.sa_data[3] = 0;
      netstats[i].addr.sa_data[4] = 0;
      netstats[i].addr.sa_data[5] = 0;
    }
  }

  /* this is a stub on all platforms except solaris */
  prepare_update();

  /* if you registered a callback with conky::register_cb, this will run it */
  conky::run_all_callbacks();

#if !defined(__linux__)
  /* XXX: move the following into the update_meminfo() functions? */
  if (no_buffers.get(*state)) {
    info.mem -= info.bufmem;
    info.memeasyfree += info.bufmem;
  }
#endif
}

/* Ohkie to return negative values for temperatures */
int round_to_int_temp(float f) { return static_cast<int>(f); }
/* Don't return negative values for cpugraph, bar, gauge, percentage.
 * Causes unreasonable numbers to show */
unsigned int round_to_positive_int(float f) {
  if (f >= 0.0) { return static_cast<int>(f + 0.5); }
  return 0;
}

void scan_loadavg_arg(struct text_object *obj, const char *arg) {
  obj->data.i = 0;
  if ((arg != nullptr) && (arg[1] == 0) &&
      (isdigit(static_cast<unsigned char>(arg[0])) != 0)) {
    obj->data.i = strtol(arg, nullptr, 10);
    if (obj->data.i > 3 || obj->data.i < 1) {
      LOG_WARNING("loadavg arg '{}' out of range, expected 1-3", arg);
      obj->data.i = 0;
    }
  }
  /* convert to array index (or the default (-1)) */
  obj->data.i--;
}

void scan_no_update(struct text_object *obj, const char *arg) {
  obj->data.s = static_cast<char *>(malloc(text_buffer_size.get(*state)));
  evaluate(arg, obj->data.s, text_buffer_size.get(*state));
  obj->data.s =
      static_cast<char *>(realloc(obj->data.s, strlen(obj->data.s) + 1));
}

void free_no_update(struct text_object *obj) { free(obj->data.s); }

void print_no_update(struct text_object *obj, char *p,
                     unsigned int p_max_size) {
  snprintf(p, p_max_size, "%s", obj->data.s);
}

#ifdef BUILD_GUI
void scan_loadgraph_arg(struct text_object *obj, const char *arg) {
  scan_graph(obj, arg, 0, FALSE);
}

double loadgraphval(struct text_object *obj) {
  (void)obj;

  return info.loadavg[0];
}
#endif /* BUILD_GUI */

uint8_t cpu_percentage(struct text_object *obj) {
  if (static_cast<unsigned int>(obj->data.i) > info.cpu_count) {
    USER_ERR("attempting to use more CPUs than you have (requested CPU {}, but only {} available)", obj->data.i, info.cpu_count);
  }
  if (info.cpu_usage != nullptr) {
    return round_to_positive_int(info.cpu_usage[obj->data.i] * 100.0);
  }
  return 0;
}

double cpu_barval(struct text_object *obj) {
  if (static_cast<unsigned int>(obj->data.i) > info.cpu_count) {
    USER_ERR("attempting to use more CPUs than you have (requested CPU {}, but only {} available)", obj->data.i, info.cpu_count);
  }
  if (info.cpu_usage != nullptr) { return info.cpu_usage[obj->data.i]; }
  return 0.;
}

uint8_t mem_percentage() {
  return info.memmax != 0u
             ? round_to_positive_int(info.mem * 100 / info.memmax) : 0;
}

double mem_barval() {
  return info.memmax != 0u ? static_cast<double>(info.mem) / info.memmax : 0;
}

double mem_with_buffers_barval() {
  return info.memmax != 0u
             ? static_cast<double>(info.memwithbuffers) / info.memmax : 0;
}

uint8_t swap_percentage() {
  return info.swapmax != 0u
             ? round_to_positive_int(info.swap * 100 / info.swapmax) : 0;
}

double swap_barval() {
  return info.swapmax != 0u ? static_cast<double>(info.swap) / info.swapmax
                            : 0;
}

int if_empty_iftest(struct text_object *obj) {
  std::vector<char> buf(max_user_text.get(*state));
  int result = 1;

  generate_text_internal(&(buf[0]), max_user_text.get(*state), *obj->sub);

  if (strlen(&(buf[0])) != 0) { result = 0; }
  return result;
}

static int check_contains(char *f, char *s) {
  int ret = 0;
  FILE *where = open_file(f, nullptr);

  if (where != nullptr) {
    char buf1[256];

    while (fgets(buf1, 256, where) != nullptr) {
      if (strstr(buf1, s) != nullptr) {
        ret = 1;
        break;
      }
    }
    fclose(where);
  } else {
    LOG_DEBUG("could not open file '{}' for contains check", f);
  }
  return ret;
}

int if_existing_iftest(struct text_object *obj) {
  char *spc;
  int result = 0;

  spc = strchr(obj->data.s, ' ');
  if (spc != nullptr) { *spc = 0; }
  if (access(obj->data.s, F_OK) == 0) {
    if (spc == nullptr || (check_contains(obj->data.s, spc + 1) != 0)) {
      result = 1;
    }
  }
  if (spc != nullptr) { *spc = ' '; }
  return result;
}

int if_running_iftest(struct text_object *obj) {
  if (!is_process_running(obj->data.s)) { return 0; }
  return 1;
}

#ifndef __OpenBSD__
void print_acpitemp(struct text_object *obj, char *p, unsigned int p_max_size) {
  temp_print(p, p_max_size, get_acpi_temperature(obj->data.i), TEMP_CELSIUS, 1);
}

void free_acpitemp(struct text_object *obj) { close(obj->data.i); }
#endif /* !__OpenBSD__ */

void print_freq(struct text_object *obj, char *p, unsigned int p_max_size) {
  static int ok = 1;
  if (ok != 0) { ok = get_freq(p, p_max_size, "%.0f", 1, obj->data.i); }
}

void print_freq_g(struct text_object *obj, char *p, unsigned int p_max_size) {
  static int ok = 1;
  if (ok != 0) {
#ifndef __OpenBSD__
    ok = get_freq(p, p_max_size, "%'.2f", 1000, obj->data.i);
#else
    /* OpenBSD has no such flag (SUSv2) */
    ok = get_freq(p, p_max_size, "%.2f", 1000, obj->data.i);
#endif /* __OpenBSD */
  }
}

#ifndef __OpenBSD__
void print_acpifan(struct text_object *obj, char *p, unsigned int p_max_size) {
  (void)obj;
  get_acpi_fan(p, p_max_size);
}

void print_acpiacadapter(struct text_object *obj, char *p,
                         unsigned int p_max_size) {
  get_acpi_ac_adapter(p, p_max_size,
                      static_cast<const char *>(obj->data.opaque));
}

void print_battery(struct text_object *obj, char *p, unsigned int p_max_size) {
  get_battery_stuff(p, p_max_size, obj->data.s, BATTERY_STATUS);
}

void print_battery_time(struct text_object *obj, char *p,
                        unsigned int p_max_size) {
  get_battery_stuff(p, p_max_size, obj->data.s, BATTERY_TIME);
}

void battery_power_draw(struct text_object *obj, char *p,
                        unsigned int p_max_size) {
  get_battery_power_draw(p, p_max_size, obj->data.s);
}

uint8_t battery_percentage(struct text_object *obj) {
  return get_battery_perct(obj->data.s);
}

void print_battery_short(struct text_object *obj, char *p,
                         unsigned int p_max_size) {
  get_battery_short_status(p, p_max_size, obj->data.s);
}

void print_battery_status(struct text_object *obj, char *p,
                          unsigned int p_max_size) {
  get_battery_stuff(p, p_max_size, obj->data.s, BATTERY_STATUS);
  if (0 == strncmp("charging", p, 8)) {
    snprintf(p, p_max_size, "%s", "charging");
  } else if (0 == strncmp("discharging", p, 11) ||
             0 == strncmp("remaining", p, 9)) {
    snprintf(p, p_max_size, "%s", "discharging");
  } else if (0 == strncmp("charged", p, 7)) {
    snprintf(p, p_max_size, "%s", "charged");
  } else if (0 == strncmp("not present", p, 11) ||
             0 == strncmp("absent/on AC", p, 12)) {
    snprintf(p, p_max_size, "%s", "not present");
  } else if (0 == strncmp("empty", p, 5)) {
    snprintf(p, p_max_size, "%s", "empty");
  } else if (0 == strncmp("unknown", p, 7)) {
    snprintf(p, p_max_size, "%s", "unknown");
  }
}
#endif /* !__OpenBSD__ */

void print_blink(struct text_object *obj, char *p, unsigned int p_max_size) {
  // blinking like this can look a bit ugly if the chars in the font don't have
  // the same width
  std::vector<char> buf(max_user_text.get(*state));
  static int visible = 1;
  static int last_len = 0;
  int i;

  if (visible != 0) {
    generate_text_internal(&(buf[0]), max_user_text.get(*state), *obj->sub);
    last_len = strlen(&(buf[0]));
  } else {
    for (i = 0; i < last_len; i++) { buf[i] = ' '; }
  }

  snprintf(p, p_max_size, "%s", &(buf[0]));
  visible = static_cast<int>(static_cast<int>(visible) == 0);
}

void print_include(struct text_object *obj, char *p, unsigned int p_max_size) {
  std::vector<char> buf(max_user_text.get(*state));

  if (obj->sub == nullptr) { return; }

  generate_text_internal(&(buf[0]), max_user_text.get(*state), *obj->sub);
  snprintf(p, p_max_size, "%s", &(buf[0]));
}

#ifdef BUILD_CURL
namespace {
constexpr char kGithubNotificationsUrl[] =
    "https://api.github.com/notifications";
}

std::string github_notifications_url() { return kGithubNotificationsUrl; }

std::string github_authorization_header(const std::string &token) {
  return "Authorization: Bearer " + token;
}

#define NEW_TOKEN                       \
  "https://github.com/settings/tokens/" \
  "new?scopes=notifications&description=conky-query-github\n"
static size_t read_github_data_cb(char *, size_t, size_t, char *);
static size_t read_github_data_cb(char *data, size_t size, size_t nmemb,
                                  char *p) {
  char *ptr = data;
  size_t sz = nmemb * size;
  size_t z = 0;
  static size_t x = 0;
  static unsigned int skip = 0U;

  for (; *ptr; ptr++, z++) {
    if (z + 4 < sz) { /* Verifying up to *(ptr+4) */
      if ('u' == *ptr && 'n' == *(ptr + 1) && 'r' == *(ptr + 2) &&
          'e' == *(ptr + 3)) { /* "unread" */
        ++x;
        skip = 0U;
      }
      if ('m' == *ptr && 'e' == *(ptr + 1) && 's' == *(ptr + 2) &&
          's' == *(ptr + 3) && z + 13 < sz) { /* "message": */
        if ('B' == *(ptr + 10) && 'a' == *(ptr + 11) &&
            'd' == *(ptr + 12)) { /* "Bad credentials" */
          LOG_ERROR("github: bad credentials, generate a new token at {}", NEW_TOKEN);
          snprintf(p, 80, "%s",
                   "GitHub: Bad credentials, generate a new token.");
          skip = 1U;
          break;
        }
        if ('M' == *(ptr + 10) && 'i' == *(ptr + 11) &&
            's' == *(ptr + 12)) { /* Missing the 'notifications' scope. */
          LOG_ERROR("github: missing 'notifications' scope, generate a new token at {}", NEW_TOKEN);
          snprintf(
              p, 80, "%s",
              "GitHub: Missing the notifications scope. Generate a new token.");
          skip = 1U;
          break;
        }
      }
    }
  }
  if (0U == skip) { snprintf(p, 49, "%zu", x); }
  return sz;
}

void print_github(struct text_object *obj, char *p, unsigned int p_max_size) {
  (void)obj;
  static char cached_result[256] = {""};
  static unsigned int last_update = 1U;
  CURL *curl = nullptr;
  CURLcode res;
  struct curl_slist *headers = nullptr;
  std::string token = github_token.get(*state);

  if (token.empty()) {
    LOG_ERROR("${{github_notifications}} requires a token, generate one at {} and add github_token='TOKEN' to conky.config", NEW_TOKEN);
    snprintf(p, p_max_size, "%s",
             "GitHub notifications requires token, generate a new one.");
    return;
  }

  if (1U != last_update) {
    --last_update;
    snprintf(p, p_max_size, "%s", cached_result);
    return;
  }

  curl_global_init(CURL_GLOBAL_ALL);
  if (nullptr == (curl = curl_easy_init())) { goto error; }
  headers =
      curl_slist_append(headers, github_authorization_header(token).c_str());
  if (headers == nullptr) { goto error; }
  curl_easy_setopt(curl, CURLOPT_URL, github_notifications_url().c_str());
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
#if defined(CURLOPT_ACCEPT_ENCODING)
  curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "gzip");
#else  /* defined(CURLOPT_ACCEPT_ENCODING) */
  curl_easy_setopt(curl, CURLOPT_ENCODING, "gzip");
#endif /* defined(CURLOPT_ACCEPT_ENCODING) */
  curl_easy_setopt(curl, CURLOPT_USERAGENT, "conky-github/1.0");
  curl_easy_setopt(curl, CURLOPT_USE_SSL, (long)CURLUSESSL_ALL);
  curl_easy_setopt(curl, CURLOPT_TIMEOUT, 20L);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, read_github_data_cb);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, p);

  res = curl_easy_perform(curl);
  if (CURLE_OK != res) { goto error; }
  snprintf(cached_result, 255, "%s", p);
  last_update = 60U;

error:
  if (headers != nullptr) { curl_slist_free_all(headers); }
  if (nullptr != curl) { curl_easy_cleanup(curl); }
  curl_global_cleanup();

  if (!isdigit(static_cast<unsigned char>(*p))) { last_update = 1U; }
}

void print_stock(struct text_object *obj, char *p, unsigned int p_max_size) {
  if (!obj->data.s) {
    p[0] = 0;
    return;
  }
  ccurl_process_info(p, p_max_size, obj->data.s, 1);
}

void free_stock(struct text_object *obj) { free(obj->data.s); }
#endif /* BUILD_CURL */

void print_to_bytes(struct text_object *obj, char *p, unsigned int p_max_size) {
  std::vector<char> buf(max_user_text.get(*state));
  long double bytes;
  char unit[16];  // 16 because we can also have long names (like mega-bytes)

  generate_text_internal(&(buf[0]), max_user_text.get(*state), *obj->sub);
  if (sscanf(&(buf[0]), "%Lf%s", &bytes, unit) == 2 && strlen(unit) < 16) {
    if (strncasecmp("b", unit, 1) == 0) {
      snprintf(&(buf[0]), max_user_text.get(*state), "%Lf", bytes);
    } else if (strncasecmp("k", unit, 1) == 0) {
      snprintf(&(buf[0]), max_user_text.get(*state), "%Lf", bytes * 1024);
    } else if (strncasecmp("m", unit, 1) == 0) {
      snprintf(&(buf[0]), max_user_text.get(*state), "%Lf",
               bytes * 1024 * 1024);
    } else if (strncasecmp("g", unit, 1) == 0) {
      snprintf(&(buf[0]), max_user_text.get(*state), "%Lf",
               bytes * 1024 * 1024 * 1024);
    } else if (strncasecmp("t", unit, 1) == 0) {
      snprintf(&(buf[0]), max_user_text.get(*state), "%Lf",
               bytes * 1024 * 1024 * 1024 * 1024);
    }
  }
  snprintf(p, p_max_size, "%s", &(buf[0]));
}

int updatenr_iftest(struct text_object *obj) {
  if (get_total_updates() % get_updatereset() != obj->data.i - 1) { return 0; }
  return 1;
}

using namespace conky::text_object;
using namespace std::chrono_literals;

template <auto Member>
constexpr variable_definition info_field_variable(const char* name) {
  return {name, [](text_object *obj, const construct_context &ctx) {
      obj->data.s = strndup(ctx.arg ? ctx.arg : "", text_buffer_size.get(*state));
      obj->callbacks.print = [](text_object *o, char *p, unsigned int s) {
        human_readable(apply_base_multiplier(o->data.s, info.*Member), p, s);
      };
      obj->callbacks.free = &gen_free_opaque;
    }, &update_meminfo};
}

// clang-format off
CONKY_REGISTER_VARIABLES(
    // --- uname ---
    print_variable("kernel", [] { return info.uname_s.release; }),
    print_variable("machine", [] { return info.uname_s.machine; }),
    print_variable("nodename", [] { return info.uname_s.nodename; }),
    {"nodename_short", [](text_object *obj, const construct_context &) {
      obj->callbacks.print = [](text_object *, char *p, unsigned int s) {
        snprintf(p, s, "%s", info.uname_s.nodename);
        for (int i = 0; p[i] != 0; i++) {
          if (p[i] == '.') { p[i] = 0; break; }
        }
      };
    }},
    print_variable("sysname", [] { return info.uname_s.sysname; }),
#if defined(__DragonFly__)
    print_variable("version", [] { return info.uname_v; }),
#endif
    // --- uptime ---
    print_variable("uptime",
      [] { return std::chrono::seconds(static_cast<long>(info.uptime)); },
      &update_uptime),
    {"uptime_short", [](text_object *obj, const construct_context &) {
      obj->callbacks.print = [](text_object *, char *p, unsigned int s) {
        format_seconds_short(p, s, static_cast<int>(info.uptime));
      };
    }, &update_uptime},

    // --- memory / swap / buffers ---
    info_field_variable<&information::mem>("mem"),
    info_field_variable<&information::legacymem>("legacymem"),
    info_field_variable<&information::memwithbuffers>("memwithbuffers"),
    info_field_variable<&information::memeasyfree>("memeasyfree"),
    info_field_variable<&information::memfree>("memfree"),
    info_field_variable<&information::memmax>("memmax"),
    info_field_variable<&information::memdirty>("memdirty"),
    info_field_variable<&information::memavail>("memavail"),
    info_field_variable<&information::shmem>("shmem"),
    info_field_variable<&information::memactive>("memactive"),
    info_field_variable<&information::meminactive>("meminactive"),
    info_field_variable<&information::memwired>("memwired"),
    info_field_variable<&information::memlaundry>("memlaundry"),
    info_field_variable<&information::swap>("swap"),
    info_field_variable<&information::swapfree>("swapfree"),
    info_field_variable<&information::swapmax>("swapmax"),
    info_field_variable<&information::buffers>("buffers"),
    info_field_variable<&information::cached>("cached"),
    info_field_variable<&information::free_bufcache>("free_bufcache"),
    info_field_variable<&information::free_cached>("free_cached"),
    {"memperc", [](text_object *obj, const construct_context &) {
      obj->callbacks.percentage = [](text_object *) -> uint8_t {
        return mem_percentage();
      };
    }, &update_meminfo},
    {"membar", [](text_object *obj, const construct_context &ctx) {
      scan_bar(obj, ctx.arg, 1);
      obj->callbacks.barval = [](text_object *) -> double { return mem_barval(); };
    }, &update_meminfo},
    {"memwithbuffersbar", [](text_object *obj, const construct_context &ctx) {
      scan_bar(obj, ctx.arg, 1);
      obj->callbacks.barval = [](text_object *) -> double { return mem_with_buffers_barval(); };
    }, &update_meminfo},
#ifdef BUILD_GUI
    {"memgauge", [](text_object *obj, const construct_context &ctx) {
      scan_gauge(obj, ctx.arg, 1);
      obj->callbacks.gaugeval = [](text_object *) -> double { return mem_barval(); };
    }, &update_meminfo},
    {"memgraph", [](text_object *obj, const construct_context &ctx) {
      scan_graph(obj, ctx.arg, 1, FALSE);
      obj->callbacks.graphval = [](text_object *) -> double { return mem_barval(); };
    }, &update_meminfo},
    {"memwithbuffersgraph", [](text_object *obj, const construct_context &ctx) {
      scan_graph(obj, ctx.arg, 1, FALSE);
      obj->callbacks.graphval = [](text_object *) -> double { return mem_with_buffers_barval(); };
    }, &update_meminfo},
#endif /* BUILD_GUI */
    {"swapperc", [](text_object *obj, const construct_context &) {
      obj->callbacks.percentage = [](text_object *) -> uint8_t {
        return swap_percentage();
      };
    }, &update_meminfo},
    {"swapbar", [](text_object *obj, const construct_context &ctx) {
      scan_bar(obj, ctx.arg, 1);
      obj->callbacks.barval = [](text_object *) -> double { return swap_barval(); };
    }, &update_meminfo},

    // --- processes ---
    print_variable_w(4,"processes", [] { return info.procs; }, &update_total_processes),
    {"running_processes", [](text_object *obj, const construct_context &) {
      top_running = 1;
      obj->callbacks.print = [](text_object *, char *p, unsigned int s) {
        spaced_print(p, s, "%hu", 4, info.run_procs);
      };
    }, &update_top},
    print_variable_w(4,"threads", [] { return info.threads; }, &update_threads),
    print_variable_w(4,"running_threads", [] { return info.run_threads; },
#if defined(__linux__)
      &update_stat
#elif defined(__APPLE__) && defined(__MACH__)
      &update_running_threads
#else
      &update_running_processes
#endif
    ),

    // --- loadavg ---
    {"loadavg", [](text_object *obj, const construct_context &ctx) {
      scan_loadavg_arg(obj, ctx.arg);
      obj->callbacks.print = [](text_object *obj, char *p, unsigned int s) {
        float *v = info.loadavg;
        if (obj->data.i < 0) {
          snprintf(p, s, "%.2f %.2f %.2f", v[0], v[1], v[2]);
        } else {
          snprintf(p, s, "%.2f", v[obj->data.i]);
        }
      };
    }, &update_load_average},

    // --- updates ---
    print_variable("updates", get_total_updates),

    // --- eval ---
    {"eval", [](text_object *obj, const construct_context &ctx) {
      obj->data.s = strndup(ctx.arg ? ctx.arg : "", text_buffer_size.get(*state));
      obj->callbacks.print = [](text_object *obj, char *p, unsigned int s) {
        std::vector<char> buf(text_buffer_size.get(*state));
        evaluate(obj->data.s, &buf[0], buf.size());
        evaluate(&buf[0], p, s);
      };
      obj->callbacks.free = &gen_free_opaque;
    }},
)

#ifndef __OpenBSD__
CONKY_REGISTER_VARIABLES(
    // --- acpi ---
    {"acpitemp", [](text_object *obj, const construct_context &ctx) {
      obj->data.i = open_acpi_temperature(ctx.arg);
      obj->callbacks.print = &print_acpitemp;
      obj->callbacks.free = &free_acpitemp;
    }},
    {"acpiacadapter", [](text_object *obj, const construct_context &ctx) {
      if (ctx.arg != nullptr) {
#ifdef __linux__
        if (strpbrk(ctx.arg, "/.") != nullptr) {
          LOG_ERROR("acpiacadapter: arg must not contain '/' or '.', got '{}'", ctx.arg);
        } else {
          obj->data.opaque = strdup(ctx.arg);
        }
#else
        LOG_WARNING("acpiacadapter: arg is only used on linux");
#endif
      }
      obj->callbacks.print = &print_acpiacadapter;
      obj->callbacks.free = &gen_free_opaque;
    }},
    {"acpifan", [](text_object *obj, const construct_context &) {
      obj->callbacks.print = [](text_object *, char *p, unsigned int s) {
        get_acpi_fan(p, s);
      };
    }},

    // --- battery ---
    {"battery", [](text_object *obj, const construct_context &ctx) {
      char bat[64];
      if (ctx.arg != nullptr) { sscanf(ctx.arg, "%63s", bat); }
      else { strncpy(bat, "BAT0", 5); }
      obj->data.s = strndup(bat, text_buffer_size.get(*state));
      obj->callbacks.print = &print_battery;
      obj->callbacks.free = &gen_free_opaque;
    }},
    {"battery_short", [](text_object *obj, const construct_context &ctx) {
      char bat[64];
      if (ctx.arg != nullptr) { sscanf(ctx.arg, "%63s", bat); }
      else { strncpy(bat, "BAT0", 5); }
      obj->data.s = strndup(bat, text_buffer_size.get(*state));
      obj->callbacks.print = &print_battery_short;
      obj->callbacks.free = &gen_free_opaque;
    }},
    {"battery_status", [](text_object *obj, const construct_context &ctx) {
      obj->data.s = strndup(ctx.arg ? ctx.arg : "BAT0", text_buffer_size.get(*state));
      obj->callbacks.print = &print_battery_status;
      obj->callbacks.free = &gen_free_opaque;
    }},
    {"battery_time", [](text_object *obj, const construct_context &ctx) {
      char bat[64];
      if (ctx.arg != nullptr) { sscanf(ctx.arg, "%63s", bat); }
      else { strncpy(bat, "BAT0", 5); }
      obj->data.s = strndup(bat, text_buffer_size.get(*state));
      obj->callbacks.print = &print_battery_time;
      obj->callbacks.free = &gen_free_opaque;
    }},
    {"battery_percent", [](text_object *obj, const construct_context &ctx) {
      char bat[64];
      if (ctx.arg != nullptr) { sscanf(ctx.arg, "%63s", bat); }
      else { strncpy(bat, "BAT0", 5); }
      obj->data.s = strndup(bat, text_buffer_size.get(*state));
      obj->callbacks.percentage = &battery_percentage;
      obj->callbacks.free = &gen_free_opaque;
    }},
    {"battery_power_draw", [](text_object *obj, const construct_context &ctx) {
      char bat[64];
      if (ctx.arg != nullptr) { sscanf(ctx.arg, "%63s", bat); }
      else { strncpy(bat, "BAT0", 5); }
      obj->data.s = strndup(bat, text_buffer_size.get(*state));
      obj->callbacks.print = &battery_power_draw;
      obj->callbacks.free = &gen_free_opaque;
    }},
    {"battery_bar", [](text_object *obj, const construct_context &ctx) {
      char bat[64];
      auto *remaining = scan_bar(obj, ctx.arg, 100);
      if (remaining != nullptr && strlen(remaining) > 0) {
        sscanf(remaining, "%63s", bat);
      } else {
        strncpy(bat, "BAT0", 5);
      }
      obj->data.s = strndup(bat, text_buffer_size.get(*state));
      obj->callbacks.barval = &get_battery_perct_bar;
      obj->callbacks.free = &gen_free_opaque;
    }},
)
#endif /* !__OpenBSD__ */

CONKY_REGISTER_VARIABLES(
    // --- freq ---
    {"freq", [](text_object *obj, const construct_context &ctx) {
      get_cpu_count();
      if (ctx.arg == nullptr || strlen(ctx.arg) >= 3 ||
          strtol(&ctx.arg[0], nullptr, 10) == 0 ||
          static_cast<unsigned int>(strtol(&ctx.arg[0], nullptr, 10)) > info.cpu_count) {
        obj->data.i = 1;
        LOG_WARNING("invalid CPU number '{}', falling back to CPU 1",
                    ctx.arg ? ctx.arg : "(null)");
      } else {
        obj->data.i = strtol(&ctx.arg[0], nullptr, 10);
      }
      obj->callbacks.print = &print_freq;
    }},
    {"freq_g", [](text_object *obj, const construct_context &ctx) {
      get_cpu_count();
      if (ctx.arg == nullptr || strlen(ctx.arg) >= 3 ||
          strtol(&ctx.arg[0], nullptr, 10) == 0 ||
          static_cast<unsigned int>(strtol(&ctx.arg[0], nullptr, 10)) > info.cpu_count) {
        obj->data.i = 1;
        LOG_WARNING("invalid CPU number '{}', falling back to CPU 1",
                    ctx.arg ? ctx.arg : "(null)");
      } else {
        obj->data.i = strtol(&ctx.arg[0], nullptr, 10);
      }
      obj->callbacks.print = &print_freq_g;
    }},

    // --- no_update ---
    {"no_update", [](text_object *obj, const construct_context &ctx) {
      scan_no_update(obj, ctx.arg);
      obj->callbacks.print = &print_no_update;
      obj->callbacks.free = &free_no_update;
    }, nullptr, {}, obj_flags::arg},

    // --- conditionals ---
    {"if_empty", [](text_object *obj, const construct_context &ctx) {
      obj->sub = static_cast<text_object *>(malloc(sizeof(struct text_object)));
      extract_variable_text_internal(obj->sub, ctx.arg);
      obj->callbacks.iftest = &if_empty_iftest;
    }, nullptr, {}, obj_flags::arg | obj_flags::cond},
    {"if_existing", [](text_object *obj, const construct_context &ctx) {
      obj->data.s = strndup(ctx.arg ? ctx.arg : "", text_buffer_size.get(*state));
      obj->callbacks.iftest = &if_existing_iftest;
      obj->callbacks.free = &gen_free_opaque;
    }, nullptr, {}, obj_flags::arg | obj_flags::cond},
    {"if_running", [](text_object *obj, const construct_context &ctx) {
#if defined(__linux__) || defined(__FreeBSD__)
      top_running = 1;
#endif
      obj->data.s = strndup(ctx.arg ? ctx.arg : "", text_buffer_size.get(*state));
      obj->callbacks.iftest = &if_running_iftest;
      obj->callbacks.free = &gen_free_opaque;
    },
#if defined(__linux__) || defined(__FreeBSD__)
    &update_top,
#else
    nullptr,
#endif
    {}, obj_flags::arg | obj_flags::cond},
    {"if_updatenr", [](text_object *obj, const construct_context &ctx) {
      obj->data.i = ctx.arg != nullptr ? strtol(ctx.arg, nullptr, 10) : 0;
      if (obj->data.i == 0) {
        *ctx.status = create_status::invalid_argument;
        LOG_ERROR("if_updatenr needs a number above 0 as argument");
        return;
      }
      set_updatereset(obj->data.i > get_updatereset() ? obj->data.i : get_updatereset());
      obj->callbacks.iftest = &updatenr_iftest;
    }, nullptr, {}, obj_flags::cond},

    // --- blink / include ---
    {"blink", [](text_object *obj, const construct_context &ctx) {
      obj->sub = static_cast<text_object *>(malloc(sizeof(struct text_object)));
      extract_variable_text_internal(obj->sub, ctx.arg);
      obj->callbacks.print = &print_blink;
    }, nullptr, {}, obj_flags::arg},
    {"include", [](text_object *obj, const construct_context &ctx) {
      obj->sub = static_cast<text_object *>(malloc(sizeof(struct text_object)));
      extract_variable_text_internal(obj->sub, ctx.arg);
      obj->callbacks.print = &print_include;
    }, nullptr, {}, obj_flags::arg},
)

#ifdef BUILD_GUI
CONKY_REGISTER_VARIABLES(
    {"loadgraph", [](text_object *obj, const construct_context &ctx) {
      scan_loadgraph_arg(obj, ctx.arg);
      obj->callbacks.graphval = &loadgraphval;
    }, &update_load_average},
)
#endif /* BUILD_GUI */

#ifdef BUILD_CURL

void stock_parse_arg(struct text_object *obj, const char *arg) {
  char stock[8];
  char data[16];

  obj->data.s = nullptr;
  if (sscanf(arg, "%7s %15s", stock, data) != 2) {
    LOG_ERROR("wrong number of arguments for $stock (got '{}')", arg ? arg : "(null)");
    return;
  }
  if (!strcasecmp("ask", data)) {
    strncpy(data, "a", 3);
  } else if (!strcasecmp("adv", data)) {
    strncpy(data, "a2", 3);
  } else if (!strcasecmp("asksize", data)) {
    strncpy(data, "a5", 3);
  } else if (!strcasecmp("bid", data)) {
    strncpy(data, "b", 3);
  } else if (!strcasecmp("askrt", data)) {
    strncpy(data, "b2", 3);
  } else if (!strcasecmp("bidrt", data)) {
    strncpy(data, "b3", 3);
  } else if (!strcasecmp("bookvalue", data)) {
    strncpy(data, "b4", 3);
  } else if (!strcasecmp("bidsize", data)) {
    strncpy(data, "b6", 3);
  } else if (!strcasecmp("change", data)) {
    strncpy(data, "c1", 3);
  } else if (!strcasecmp("commission", data)) {
    strncpy(data, "c3", 3);
  } else if (!strcasecmp("changert", data)) {
    strncpy(data, "c6", 3);
  } else if (!strcasecmp("ahcrt", data)) {
    strncpy(data, "c8", 3);
  } else if (!strcasecmp("ds", data)) {
    strncpy(data, "d", 3);
  } else if (!strcasecmp("ltd", data)) {
    strncpy(data, "d1", 3);
  } else if (!strcasecmp("tradedate", data)) {
    strncpy(data, "d2", 3);
  } else if (!strcasecmp("es", data)) {
    strncpy(data, "e", 3);
  } else if (!strcasecmp("ei", data)) {
    strncpy(data, "e1", 3);
  } else if (!strcasecmp("epsecy", data)) {
    strncpy(data, "e7", 3);
  } else if (!strcasecmp("epseny", data)) {
    strncpy(data, "e8", 3);
  } else if (!strcasecmp("epsenq", data)) {
    strncpy(data, "e9", 3);
  } else if (!strcasecmp("floatshares", data)) {
    strncpy(data, "f6", 3);
  } else if (!strcasecmp("dayslow", data)) {
    strncpy(data, "g", 3);
  } else if (!strcasecmp("dayshigh", data)) {
    strncpy(data, "h", 3);
  } else if (!strcasecmp("52weeklow", data)) {
    strncpy(data, "j", 3);
  } else if (!strcasecmp("52weekhigh", data)) {
    strncpy(data, "k", 3);
  } else if (!strcasecmp("hgp", data)) {
    strncpy(data, "g1", 3);
  } else if (!strcasecmp("ag", data)) {
    strncpy(data, "g3", 3);
  } else if (!strcasecmp("hg", data)) {
    strncpy(data, "g4", 3);
  } else if (!strcasecmp("hgprt", data)) {
    strncpy(data, "g5", 3);
  } else if (!strcasecmp("hgrt", data)) {
    strncpy(data, "g6", 3);
  } else if (!strcasecmp("moreinfo", data)) {
    strncpy(data, "i", 3);
  } else if (!strcasecmp("obrt", data)) {
    strncpy(data, "i5", 3);
  } else if (!strcasecmp("mc", data)) {
    strncpy(data, "j1", 3);
  } else if (!strcasecmp("mcrt", data)) {
    strncpy(data, "j3", 3);
  } else if (!strcasecmp("ebitda", data)) {
    strncpy(data, "j4", 3);
  } else if (!strcasecmp("c52wlow", data)) {
    strncpy(data, "j5", 3);
  } else if (!strcasecmp("pc52wlow", data)) {
    strncpy(data, "j6", 3);
  } else if (!strcasecmp("cprt", data)) {
    strncpy(data, "k2", 3);
  } else if (!strcasecmp("lts", data)) {
    strncpy(data, "k3", 3);
  } else if (!strcasecmp("c52whigh", data)) {
    strncpy(data, "k4", 3);
  } else if (!strcasecmp("pc52whigh", data)) {
    strncpy(data, "k5", 3);
  } else if (!strcasecmp("ltp", data)) {
    strncpy(data, "l1", 3);
  } else if (!strcasecmp("hl", data)) {
    strncpy(data, "l2", 3);
  } else if (!strcasecmp("ll", data)) {
    strncpy(data, "l3", 3);
  } else if (!strcasecmp("dr", data)) {
    strncpy(data, "m", 3);
  } else if (!strcasecmp("drrt", data)) {
    strncpy(data, "m2", 3);
  } else if (!strcasecmp("50ma", data)) {
    strncpy(data, "m3", 3);
  } else if (!strcasecmp("200ma", data)) {
    strncpy(data, "m4", 3);
  } else if (!strcasecmp("c200ma", data)) {
    strncpy(data, "m5", 3);
  } else if (!strcasecmp("pc200ma", data)) {
    strncpy(data, "m6", 3);
  } else if (!strcasecmp("c50ma", data)) {
    strncpy(data, "m7", 3);
  } else if (!strcasecmp("pc50ma", data)) {
    strncpy(data, "m8", 3);
  } else if (!strcasecmp("name", data)) {
    strncpy(data, "n", 3);
  } else if (!strcasecmp("notes", data)) {
    strncpy(data, "n4", 3);
  } else if (!strcasecmp("open", data)) {
    strncpy(data, "o", 3);
  } else if (!strcasecmp("pc", data)) {
    strncpy(data, "p", 3);
  } else if (!strcasecmp("pricepaid", data)) {
    strncpy(data, "p1", 3);
  } else if (!strcasecmp("cip", data)) {
    strncpy(data, "p2", 3);
  } else if (!strcasecmp("ps", data)) {
    strncpy(data, "p5", 3);
  } else if (!strcasecmp("pb", data)) {
    strncpy(data, "p6", 3);
  } else if (!strcasecmp("edv", data)) {
    strncpy(data, "q", 3);
  } else if (!strcasecmp("per", data)) {
    strncpy(data, "r", 3);
  } else if (!strcasecmp("dpd", data)) {
    strncpy(data, "r1", 3);
  } else if (!strcasecmp("perrt", data)) {
    strncpy(data, "r2", 3);
  } else if (!strcasecmp("pegr", data)) {
    strncpy(data, "r5", 3);
  } else if (!strcasecmp("pepsecy", data)) {
    strncpy(data, "r6", 3);
  } else if (!strcasecmp("pepseny", data)) {
    strncpy(data, "r7", 3);
  } else if (!strcasecmp("symbol", data)) {
    strncpy(data, "s", 3);
  } else if (!strcasecmp("sharesowned", data)) {
    strncpy(data, "s1", 3);
  } else if (!strcasecmp("shortratio", data)) {
    strncpy(data, "s7", 3);
  } else if (!strcasecmp("ltt", data)) {
    strncpy(data, "t1", 3);
  } else if (!strcasecmp("tradelinks", data)) {
    strncpy(data, "t6", 3);
  } else if (!strcasecmp("tt", data)) {
    strncpy(data, "t7", 3);
  } else if (!strcasecmp("1ytp", data)) {
    strncpy(data, "t8", 3);
  } else if (!strcasecmp("volume", data)) {
    strncpy(data, "v", 3);
  } else if (!strcasecmp("hv", data)) {
    strncpy(data, "v1", 3);
  } else if (!strcasecmp("hvrt", data)) {
    strncpy(data, "v7", 3);
  } else if (!strcasecmp("52weekrange", data)) {
    strncpy(data, "w", 3);
  } else if (!strcasecmp("dvc", data)) {
    strncpy(data, "w1", 3);
  } else if (!strcasecmp("dvcrt", data)) {
    strncpy(data, "w4", 3);
  } else if (!strcasecmp("se", data)) {
    strncpy(data, "x", 3);
  } else if (!strcasecmp("dy", data)) {
    strncpy(data, "y", 3);
  } else {
    LOG_ERROR("\"{}\" is not supported by $stock. supported: 1ytp, 200ma, 50ma, "
        "52weeklow, 52weekhigh, 52weekrange, adv, ag, ahcrt, ask, askrt, "
        "asksize, bid, bidrt, bidsize, bookvalue, c200ma, c50ma, c52whigh, "
        "c52wlow, change, changert, cip, commission, cprt, dayshigh, dayslow, "
        "dpd, dr, drrt, ds, dvc, dvcrt, dy, ebitda, edv, ei, epsecy, epsenq, "
        "epseny, es, floatshares, hg, hgp, hgprt, hl, hv, hvrt, ll, ltd, ltp, "
        "lts, ltt, mc, mcrt, moreinfo, name, notes, obrt, open, pb, pc, "
        "pc200ma, pc50ma, pc52whigh, pc52wlow, pegr, pepsecy, pepseny, per, "
        "perrt, pricepaid, ps, se, sharesowned, shortratio, symbol, tradedate, "
        "tradelinks, tt, volume",
        data);
    return;
  }
#define MAX_FINYAH_URL_LENGTH 75
  obj->data.s = static_cast<char *>(malloc(MAX_FINYAH_URL_LENGTH));
  snprintf(obj->data.s, MAX_FINYAH_URL_LENGTH,
           "http://download.finance.yahoo.com/d/quotes.csv?s=%s&f=%s", stock,
           data);
}

CONKY_REGISTER_VARIABLES(
    {"github_notifications", [](text_object *obj, const construct_context &) {
      obj->callbacks.print = &print_github;
    }},
    {"stock", [](text_object *obj, const construct_context &ctx) {
      stock_parse_arg(obj, ctx.arg);
      obj->callbacks.print = &print_stock;
      obj->callbacks.free = &free_stock;
    }, nullptr, {}, obj_flags::arg},
)
#endif /* BUILD_CURL */
// clang-format on
