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
 *      (see AUTHORS)
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

#include "config.h"

#include <sys/types.h>

#include <unistd.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <string>
#include <utility>
#include <vector>

#include <pwd.h>

#ifdef HAVE_UTMP
#include <utmp.h>
#endif
#ifdef HAVE_SYSTEMD
#include <systemd/sd-daemon.h>
#include <systemd/sd-login.h>
#endif

#include "../conky.h"
#include "../logging.h"

#define BUFLEN 512

// One logged-in user session, as reported by utmp or systemd-logind.
struct user_session {
  std::string name;  // login name
  std::string term;  // terminal device without "/dev/" prefix, may be empty
  time_t login{0};   // login time, 0 when unknown
};

// Append `value` to the space-separated list in `ptr`, truncated at `len`.
static void append_field(char *ptr, size_t len, const char *value) {
  if (value == nullptr || value[0] == '\0') { return; }

  const size_t used = strnlen(ptr, len);
  if (used == 0) {
    snprintf(ptr, len, "%s", value);
  } else if (used + 1 < len) {
    snprintf(ptr + used, len - used, " %s", value);
  }
}

#ifdef HAVE_UTMP
static void collect_utmp_sessions(std::vector<user_session> &sessions) {
  const struct utmp *usr = nullptr;

  setutent();
  while ((usr = getutent()) != nullptr) {
    if (usr->ut_type == USER_PROCESS) {
      user_session s;
      s.name.assign(usr->ut_name, strnlen(usr->ut_name, UT_NAMESIZE));
      s.term.assign(usr->ut_line, strnlen(usr->ut_line, UT_LINESIZE));
      s.login = usr->ut_time;
      sessions.push_back(std::move(s));
    }
  }
  endutent();
}
#endif /* HAVE_UTMP */

#ifdef HAVE_SYSTEMD
// Resolve a uid to a user name.  getpwuid() is not reentrant, so use
// getpwuid_r() with a buffer sized per sysconf(_SC_GETPW_R_SIZE_MAX).
static bool uid_to_name(uid_t uid, std::string &out) {
  long size = sysconf(_SC_GETPW_R_SIZE_MAX);
  if (size <= 0) { size = 4096; }

  std::vector<char> buf(static_cast<size_t>(size));
  struct passwd pw {};
  struct passwd *result = nullptr;

  if (getpwuid_r(uid, &pw, buf.data(), buf.size(), &result) != 0 ||
      result == nullptr) {
    return false;
  }
  out = pw.pw_name;
  return true;
}

static void collect_logind_sessions(std::vector<user_session> &sessions) {
  char **logind_sessions = nullptr;
  const int count = sd_get_sessions(&logind_sessions);

  if (count < 0) { return; }

  for (int i = 0; i < count; ++i) {
    user_session s;
    uid_t uid = 0;

    if (sd_session_get_uid(logind_sessions[i], &uid) >= 0) {
      std::string name;
      if (uid_to_name(uid, name)) { s.name = std::move(name); }

#ifdef HAVE_SYSTEMD_LOGIN_TIME
      // logind reports the start of the user's continuous login (the
      // first session of the login they stayed logged in with), not
      // per-terminal times, so all sessions of one user share a value.
      // This is the closest equivalent of utmp's ut_time.  Needs
      // systemd >= 254, checked at configure time; without it the login
      // time stays unknown and ${user_time}/${user_times} report
      // "broken" rather than making something up.
      uint64_t usec = 0;
      if (sd_uid_get_login_time(uid, &usec) >= 0) {
        s.login = static_cast<time_t>(usec / 1000000ULL);
      }
#endif
    }

    char *tty = nullptr;
    if (sd_session_get_tty(logind_sessions[i], &tty) >= 0 && tty != nullptr) {
      // utmp reports bare device names ("tty1", "pts/0"); strip the
      // "/dev/" prefix logind may use so the formats match.
      const std::string term = tty;
      s.term = term.rfind("/dev/", 0) == 0 ? term.substr(5) : term;
      free(tty);
    }

    // Sessions without a resolvable user name cannot be attributed to
    // anyone, so skip them instead of reporting garbage.
    if (!s.name.empty()) { sessions.push_back(std::move(s)); }

    free(logind_sessions[i]);
  }
  free(logind_sessions);
}
#endif /* HAVE_SYSTEMD */

// Collect the logged-in user sessions.
//
// utmp is the preferred source so behaviour on systems that still write
// it is unchanged.  Distributions are phasing utmp out though (Ubuntu
// >= 25.04 writes no utmp records, musl never had the interface), so
// when utmp yields nothing and the system is booted with systemd, fall
// back to systemd-logind instead.  When neither source reports anything
// the callers above report "broken" instead of fabricating a user.
static void collect_user_sessions(std::vector<user_session> &sessions) {
#if defined(HAVE_UTMP) || defined(HAVE_SYSTEMD)
#ifdef HAVE_UTMP
  collect_utmp_sessions(sessions);
#endif
#ifdef HAVE_SYSTEMD
  // sd_booted() (i.e. /run/systemd/system exists) keeps containers and
  // other non-systemd environments from asking logind for sessions it
  // will never know about.
  if (sessions.empty() && sd_booted() > 0) {
    collect_logind_sessions(sessions);
  }
#endif
#else
  (void)sessions;
#endif
}

static void user_name(char *ptr, size_t len) {
  std::vector<user_session> sessions;

  collect_user_sessions(sessions);
  ptr[0] = '\0';
  for (const auto &session : sessions) {
    append_field(ptr, len, session.name.c_str());
  }
}

static void user_num(int *ptr) {
  std::vector<user_session> sessions;

  collect_user_sessions(sessions);
  *ptr = static_cast<int>(sessions.size());
}

static void user_term(char *ptr, size_t len) {
  std::vector<user_session> sessions;

  collect_user_sessions(sessions);
  ptr[0] = '\0';
  for (const auto &session : sessions) {
    append_field(ptr, len, session.term.c_str());
  }
}

static void user_time(char *ptr, size_t len) {
  std::vector<user_session> sessions;
  const time_t real = time(nullptr);

  collect_user_sessions(sessions);
  ptr[0] = '\0';
  for (const auto &session : sessions) {
    if (session.login == 0) { continue; }  // login time unknown
    char buf[BUFLEN] = "";
    format_seconds(buf, BUFLEN, difftime(real, session.login));
    append_field(ptr, len, buf);
  }
}

static void tty_user_time(char *ptr, size_t len, const char *tty) {
  std::vector<user_session> sessions;

  collect_user_sessions(sessions);
  for (const auto &session : sessions) {
    if (session.login != 0 && session.term == tty) {
      char buf[BUFLEN] = "";
      format_seconds(buf, BUFLEN, difftime(time(nullptr), session.login));
      snprintf(ptr, len, "%s", buf);
      return;
    }
  }
  LOG_DEBUG("no session found for tty '{}'", tty);
}

static void users_alloc(struct information *ptr) {
  if (ptr->users.names == nullptr) {
    ptr->users.names = (char *)malloc(text_buffer_size.get(*state));
    if (!ptr->users.names) {
      LOG_ERROR("failed to allocate user names buffer");
    }
  }
  if (ptr->users.terms == nullptr) {
    ptr->users.terms = (char *)malloc(text_buffer_size.get(*state));
    if (!ptr->users.terms) {
      LOG_ERROR("failed to allocate user terms buffer");
    }
  }
  if (ptr->users.times == nullptr) {
    ptr->users.times = (char *)malloc(text_buffer_size.get(*state));
    if (!ptr->users.times) {
      LOG_ERROR("failed to allocate user times buffer");
    }
  }
}

static void update_user_time(char *tty) {
  struct information *current_info = &info;
  char temp[BUFLEN] = "";

  if (current_info->users.ctime == nullptr) {
    current_info->users.ctime = (char *)malloc(text_buffer_size.get(*state));
  }

  tty_user_time(temp, BUFLEN, tty);

  if (*temp != 0) {
    free_and_zero(current_info->users.ctime);
    current_info->users.ctime = (char *)malloc(text_buffer_size.get(*state));
    strncpy(current_info->users.ctime, temp, text_buffer_size.get(*state));
  } else {
    LOG_WARNING("failed to get user time for tty, using fallback");
    free_and_zero(current_info->users.ctime);
    current_info->users.ctime = (char *)malloc(text_buffer_size.get(*state));
    strncpy(current_info->users.ctime, "broken", text_buffer_size.get(*state));
  }
}

int update_users(void) {
  struct information *current_info = &info;
  char temp[BUFLEN] = "";
  int t;
  users_alloc(current_info);
  user_name(temp, BUFLEN);
  if (*temp != 0) {
    free_and_zero(current_info->users.names);
    current_info->users.names = (char *)malloc(text_buffer_size.get(*state));
    strncpy(current_info->users.names, temp, text_buffer_size.get(*state));
  } else {
    LOG_WARNING("no logged-in users found, using fallback");
    free_and_zero(current_info->users.names);
    current_info->users.names = (char *)malloc(text_buffer_size.get(*state));
    strncpy(current_info->users.names, "broken", text_buffer_size.get(*state));
  }
  user_num(&t);
  if (t != 0) {
    if (current_info->users.number) { current_info->users.number = 0; }
    current_info->users.number = t;
  } else {
    current_info->users.number = 0;
  }
  temp[0] = 0;
  user_term(temp, BUFLEN);
  if (*temp != 0) {
    free_and_zero(current_info->users.terms);
    current_info->users.terms = (char *)malloc(text_buffer_size.get(*state));
    strncpy(current_info->users.terms, temp, text_buffer_size.get(*state));
  } else {
    LOG_WARNING("no user terminals found, using fallback");
    free_and_zero(current_info->users.terms);
    current_info->users.terms = (char *)malloc(text_buffer_size.get(*state));
    strncpy(current_info->users.terms, "broken", text_buffer_size.get(*state));
  }
  temp[0] = 0;
  user_time(temp, BUFLEN);
  if (*temp != 0) {
    free_and_zero(current_info->users.times);
    current_info->users.times = (char *)malloc(text_buffer_size.get(*state));
    strncpy(current_info->users.times, temp, text_buffer_size.get(*state));
  } else {
    LOG_WARNING("no user login times found, using fallback");
    free_and_zero(current_info->users.times);
    current_info->users.times = (char *)malloc(text_buffer_size.get(*state));
    strncpy(current_info->users.times, "broken", text_buffer_size.get(*state));
  }
  return 0;
}

void print_user_names(struct text_object *obj, char *p,
                      unsigned int p_max_size) {
  (void)obj;
  snprintf(p, p_max_size, "%s", info.users.names);
}

void print_user_terms(struct text_object *obj, char *p,
                      unsigned int p_max_size) {
  (void)obj;
  snprintf(p, p_max_size, "%s", info.users.terms);
}

void print_user_times(struct text_object *obj, char *p,
                      unsigned int p_max_size) {
  (void)obj;
  snprintf(p, p_max_size, "%s", info.users.times);
}

void print_user_time(struct text_object *obj, char *p,
                     unsigned int p_max_size) {
  update_user_time(obj->data.s);
  snprintf(p, p_max_size, "%s", info.users.ctime);
}

void print_user_number(struct text_object *obj, char *p,
                       unsigned int p_max_size) {
  (void)obj;
  snprintf(p, p_max_size, "%d", info.users.number);
}

void free_user_names(struct text_object *obj) {
  (void)obj;
  free_and_zero(info.users.names);
}

void free_user_terms(struct text_object *obj) {
  (void)obj;
  free_and_zero(info.users.terms);
}

void free_user_times(struct text_object *obj) {
  (void)obj;
  free_and_zero(info.users.times);
}

void free_user_time(struct text_object *obj) {
  free_and_zero(info.users.ctime);
  free_and_zero(obj->data.s);
}
