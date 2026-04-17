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
 *   (see AUTHORS)
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

#include <dirent.h>
#include <unistd.h>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <memory>
#include <optional>
#include <unordered_set>
#include <string>

#include "../conky.h"
#include "../logging.h"
#include "parse/variables.hh"

namespace {
const std::filesystem::path process_directory{"/proc"};
constexpr std::size_t read_size = 128;
}  // namespace

static const char *skip_proc_stat_comm(const char *stat) {
  if (stat == nullptr) { return nullptr; }
  const char *close = strrchr(stat, ')');
  if (close == nullptr) { return nullptr; }
  const char *after = close + 1;
  while (*after == ' ') { ++after; }
  return after;
}

/// Evaluate obj->sub and return the result as a string (typically a PID).
static std::string eval_sub_arg(struct text_object *obj) {
  std::string buf(max_user_text.get(*state), '\0');
  generate_text_internal(buf.data(), buf.size(), *obj->sub);
  buf.resize(strlen(buf.c_str()));
  return buf;
}

static std::optional<std::string> readfile(const std::filesystem::path &path,
                                           bool showerror = true) {
  std::ifstream ifs(path);
  if (!ifs) {
    if (showerror) { LOG_ERROR("failed to read proc file '{}'", path.c_str()); }
    return std::nullopt;
  }
  return std::string{std::istreambuf_iterator<char>(ifs), {}};
}

static bool parse_proc_stat_times(const char *buf, unsigned long int *utime,
                                  unsigned long int *stime) {
  if (buf == nullptr || utime == nullptr || stime == nullptr) { return false; }

  const char *close_paren = strrchr(buf, ')');
  if (close_paren == nullptr) { return false; }

  const char *after = close_paren + 1;
  while (*after == ' ') { ++after; }

  int parsed =
      sscanf(after, "%*c %*d %*d %*d %*d %*d %*u %*u %*u %*u %*u %lu %lu",
             utime, stime);
  return parsed == 2;
}

static bool parse_proc_stat_prio_nice(const char *after, long int *priority,
                                      long int *nice) {
  if (after == nullptr || priority == nullptr || nice == nullptr) {
    return false;
  }

  char state;
  int ppid;
  int pgrp;
  int session;
  int tty_nr;
  int tpgid;
  unsigned int flags;
  unsigned long minflt;
  unsigned long cminflt;
  unsigned long majflt;
  unsigned long cmajflt;
  unsigned long utime;
  unsigned long stime;
  long cutime;
  long cstime;

  int parsed = sscanf(
      after, "%c %d %d %d %d %d %u %lu %lu %lu %lu %lu %lu %ld %ld %ld %ld",
      &state, &ppid, &pgrp, &session, &tty_nr, &tpgid, &flags, &minflt,
      &cminflt, &majflt, &cmajflt, &utime, &stime, &cutime, &cstime, priority,
      nice);
  return parsed == 17;
}

void pid_readlink(const std::filesystem::path &file, char *p,
                  unsigned int p_max_size) {
  std::string buf(p_max_size, '\0');
  auto len = readlink(file.c_str(), buf.data(), p_max_size);
  if (len >= 0) {
    buf.resize(len);
    snprintf(p, p_max_size, "%s", buf.c_str());
  } else {
    LOG_ERROR("failed to readlink proc symlink '{}'", file.c_str());
  }
}


enum class pid_link { root, cwd, exe, fd0, fd1, fd2 };

template <pid_link link>
void print_pid_readlink(struct text_object *obj, char *p,
                        unsigned int p_max_size) {
  constexpr auto *subpath =
      (const char *[]){"root", "cwd", "exe", "fd/0", "fd/1", "fd/2"}[static_cast<size_t>(link)];
  auto buf = eval_sub_arg(obj);
  pid_readlink(process_directory / buf.c_str() / subpath, p, p_max_size);
}

void print_pid_cmdline(struct text_object *obj, char *p,
                       unsigned int p_max_size) {
  auto objbuf = eval_sub_arg(obj);

  if (!objbuf.empty()) {
    auto path = process_directory / objbuf.c_str() / "cmdline";
    auto buf = readfile(path);
    if (buf) {
      for (std::size_t i = 0; i < buf->size() - 1; i++) {
        if ((*buf)[i] == 0) { (*buf)[i] = ' '; }
      }
      snprintf(p, p_max_size, "%s", buf->c_str());
    }
  } else {
    LOG_ERROR("$pid_cmdline requires a PID argument via sub-expression");
  }
}


void print_pid_environ(struct text_object *obj, char *p,
                       unsigned int p_max_size) {
  pid_t pid;
  auto objbuf = eval_sub_arg(obj);
  char *end = nullptr;
  long pid_value = strtol(objbuf.data(), &end, 10);
  if (end == objbuf.data() || pid_value <= 0) {
    *p = 0;
    return;
  }
  pid = static_cast<pid_t>(pid_value);
  while (*end == ' ' || *end == '\t') { ++end; }
  if (*end == 0) {
    *p = 0;
    return;
  }

  std::string var_name(end);
  size_t stop = var_name.find_first_of(" \t");
  if (stop != std::string::npos) { var_name.resize(stop); }
  if (var_name.empty()) {
    *p = 0;
    return;
  }
  for (char &ch : var_name) { ch = toupper(static_cast<unsigned char>(ch)); }
  auto path = process_directory / std::to_string(pid) / "environ";
  auto buf = readfile(path);
  if (buf) {
    for (std::size_t i = 0; i < buf->size();
         i += strlen(buf->c_str() + i) + 1) {
      if (strncmp(buf->c_str() + i, var_name.c_str(), var_name.size()) == 0 &&
          *(buf->c_str() + i + var_name.size()) == '=') {
        snprintf(p, p_max_size, "%s", buf->c_str() + i + var_name.size() + 1);
        return;
      }
    }
  }
  *p = 0;
}

// FIXME: this loop mutates buf while iterating it via strdup/sscanf which is
// fragile and hard to follow. Should be rewritten to iterate null-separated
// entries cleanly and build the output string without in-place mutation.
void print_pid_environ_list(struct text_object *obj, char *p,
                            unsigned int p_max_size) {
  auto objbuf = eval_sub_arg(obj);
  auto path = process_directory / objbuf.c_str() / "environ";

  auto buf = readfile(path);
  if (buf) {
    std::size_t bytes_read = 0;
    int i = 0;
    for (bytes_read = 0; bytes_read < buf->size(); (*buf)[i - 1] = ';') {
      char *buf2 = strdup(buf->c_str() + bytes_read);
      bytes_read += strlen(buf2) + 1;
      sscanf(buf2, "%[^=]", buf->data() + i);
      free(buf2);
      i = strlen(buf->c_str()) + 1;
    }
    (*buf)[i - 1] = 0;
    snprintf(p, p_max_size, "%s", buf->c_str());
  }
}


void print_pid_nice(struct text_object *obj, char *p, unsigned int p_max_size) {
  long int nice_value;
  auto objbuf = eval_sub_arg(obj);

  if (!obj->data.s) {
    auto path = process_directory / objbuf.c_str() / "stat";
    auto buf = readfile(path);
    if (buf) {
      const char *after = skip_proc_stat_comm(buf->c_str());
      long int priority_value = 0;
      if (parse_proc_stat_prio_nice(after, &priority_value, &nice_value)) {
        snprintf(p, p_max_size, "%ld", nice_value);
      }
    }
  } else {
    LOG_ERROR("$pid_nice requires a PID argument via sub-expression");
  }
}

void print_pid_openfiles(struct text_object *obj, char *p,
                         unsigned int p_max_size) {
  int totallength = 0;
  std::unordered_set<std::string> seen;
  auto objbuf = eval_sub_arg(obj);

  DIR *dir = opendir(objbuf.c_str());
  if (dir == nullptr) {
    p[0] = 0;
    return;
  }
  p[0] = 0;
  struct dirent *entry;
  while ((entry = readdir(dir)) != nullptr) {
    if (entry->d_name[0] == '.') { continue; }
    auto link_path = objbuf + "/" + entry->d_name;
    std::string target(p_max_size, '\0');
    auto len = readlink(link_path.c_str(), target.data(), p_max_size - 1);
    if (len < 0) { continue; }
    target.resize(len);
    if (seen.insert(target).second) {
      int remaining = static_cast<int>(p_max_size) - totallength;
      if (remaining <= 1) { break; }
      int written = snprintf(p + totallength, remaining, "%s; ", target.c_str());
      if (written < 0) { break; }
      if (written >= remaining) {
        totallength = p_max_size - 1;
        break;
      }
      totallength += written;
    }
  }
  closedir(dir);
  if (totallength >= 2 && p[totallength - 1] == ' ' &&
      p[totallength - 2] == ';') {
    p[totallength - 2] = 0;
  }
}


void print_pid_priority(struct text_object *obj, char *p,
                        unsigned int p_max_size) {
  long int priority;
  auto objbuf = eval_sub_arg(obj);

  if (!objbuf.empty()) {
    auto path = process_directory / objbuf.c_str() / "stat";
    auto buf = readfile(path);
    if (buf) {
      const char *after = skip_proc_stat_comm(buf->c_str());
      long int nice_value = 0;
      if (parse_proc_stat_prio_nice(after, &priority, &nice_value)) {
        snprintf(p, p_max_size, "%ld", priority);
      }
    }
  } else {
    LOG_ERROR("$pid_priority requires a PID argument via sub-expression");
  }
}

void print_pid_state(struct text_object *obj, char *p,
                     unsigned int p_max_size) {
  constexpr auto state_entry = "State:\t";
  auto objbuf = eval_sub_arg(obj);
  auto path = process_directory / objbuf.c_str() / "status";

  auto buf = readfile(path);
  if (!buf) { return; }

  const char *begin = strstr(buf->c_str(), state_entry);
  if (begin == nullptr) {
    LOG_ERROR("failed to find 'State:' field in proc status file '{}'", path.c_str());
    return;
  }

  begin += strlen(state_entry) + 3; // skip "State:\t" + short state char + " ("
  const char *end = strchr(begin, '\n');
  if (end != nullptr && end > begin) {
    snprintf(p, p_max_size, "%.*s", static_cast<int>(end - 1 - begin),
             begin);  // -1 strips the ')'
  } else {
    snprintf(p, p_max_size, "%s", begin);
  }
}

void print_pid_state_short(struct text_object *obj, char *p,
                           unsigned int p_max_size) {
  constexpr auto state_entry = "State:\t";
  auto objbuf = eval_sub_arg(obj);
  auto path = process_directory / objbuf.c_str() / "status";

  auto buf = readfile(path);
  if (!buf) { return; }

  const char *begin = strstr(buf->c_str(), state_entry);
  if (begin == nullptr) {
    LOG_ERROR("failed to find 'State:' field in proc status file '{}'", path.c_str());
    return;
  }
  begin += strlen(state_entry);
  if (*begin != 0) { snprintf(p, p_max_size, "%c", *begin); }
}

void scan_cmdline_to_pid_arg(struct text_object *obj, const char *arg,
                             void *free_at_crash) {
  unsigned int i;

  if (strlen(arg) > 0) {
    obj->data.s = strdup(arg);
    for (i = 0; obj->data.s[i] != 0; i++) {
      while (obj->data.s[i] == ' ' && obj->data.s[i + 1] == ' ') {
        memmove(obj->data.s + i, obj->data.s + i + 1,
                strlen(obj->data.s + i + 1) + 1);
      }
    }
    if (obj->data.s[i - 1] == ' ') { obj->data.s[i - 1] = 0; }
  } else {
    free(obj);
    free(free_at_crash);
    USER_ERR("$cmdline_to_pid requires a non-empty command line string as argument");
  }
}

void print_cmdline_to_pid(struct text_object *obj, char *p,
                          unsigned int p_max_size) {
  DIR *dir;
  struct dirent *entry;

  dir = opendir(process_directory.c_str());
  if (dir != nullptr) {
    while ((entry = readdir(dir)) != nullptr) {
      bool numeric = true;
      for (const char *ch = entry->d_name; *ch != '\0'; ++ch) {
        if (!std::isdigit(static_cast<unsigned char>(*ch))) {
          numeric = false;
          break;
        }
      }
      if (!numeric) { continue; }

      auto path = process_directory / entry->d_name / "cmdline";

      auto contents = readfile(path, false);
      if (contents) {
        for (std::size_t i = 0; i < contents->size() - 1; i++) {
          if ((*contents)[i] == 0) { (*contents)[i] = ' '; }
        }
        if (strstr(contents->c_str(), obj->data.s) != nullptr) {
          snprintf(p, p_max_size, "%s", entry->d_name);
          closedir(dir);
          return;
        }
      }
    }
    closedir(dir);
  } else {
    LOG_ERROR("failed to open process directory '{}' for cmdline_to_pid scan", process_directory.c_str());
  }
}


void print_pid_thread_list(struct text_object *obj, char *p,
                           unsigned int p_max_size) {
  DIR *dir;
  struct dirent *entry;
  unsigned int totallength = 0;
  auto objbuf = eval_sub_arg(obj);
  auto path = process_directory / objbuf.c_str() / "task";

  dir = opendir(path.c_str());
  if (dir != nullptr) {
    while ((entry = readdir(dir)) != nullptr) {
      if (entry->d_name[0] != '.') {
        if (totallength + 1 >= p_max_size) { break; }
        unsigned int remaining = p_max_size - totallength;
        int written =
            snprintf(p + totallength, remaining, "%s,", entry->d_name);
        if (written < 0) { break; }
        if (static_cast<unsigned int>(written) >= remaining) {
          totallength = p_max_size - 1;
          break;
        }
        totallength += static_cast<unsigned int>(written);
      }
    }
    closedir(dir);
    if (totallength > 0 && p[totallength - 1] == ',') {
      p[totallength - 1] = 0;
    }
  } else {
    p[0] = 0;
  }
}

void print_pid_time_kernelmode(struct text_object *obj, char *p,
                               unsigned int p_max_size) {
  unsigned long int utime = 0;
  unsigned long int stime = 0;
  auto objbuf = eval_sub_arg(obj);

  if (!objbuf.empty()) {
    auto path = process_directory / objbuf.c_str() / "stat";
    auto buf = readfile(path);
    if (buf) {
      if (parse_proc_stat_times(buf->c_str(), &utime, &stime)) {
        snprintf(p, p_max_size, "%.2f", static_cast<float>(stime) / 100);
      }
    }
  } else {
    LOG_ERROR("$pid_time_kernelmode requires a PID argument via sub-expression");
  }
}

void print_pid_time_usermode(struct text_object *obj, char *p,
                             unsigned int p_max_size) {
  unsigned long int utime = 0;
  unsigned long int stime = 0;
  auto objbuf = eval_sub_arg(obj);

  if (!objbuf.empty()) {
    auto path = process_directory / objbuf.c_str() / "stat";
    auto buf = readfile(path);
    if (buf) {
      if (parse_proc_stat_times(buf->c_str(), &utime, &stime)) {
        snprintf(p, p_max_size, "%.2f", static_cast<float>(utime) / 100);
      }
    }
  } else {
    LOG_ERROR("$pid_time_usermode requires a PID argument via sub-expression");
  }
}

void print_pid_time(struct text_object *obj, char *p, unsigned int p_max_size) {
  unsigned long int utime = 0;
  unsigned long int stime = 0;
  auto objbuf = eval_sub_arg(obj);

  if (!objbuf.empty()) {
    auto path = process_directory / objbuf.c_str() / "stat";
    auto buf = readfile(path);
    if (buf) {
      if (parse_proc_stat_times(buf->c_str(), &utime, &stime)) {
        snprintf(p, p_max_size, "%.2f",
                 static_cast<float>(utime + stime) / 100);
      }
    }
  } else {
    LOG_ERROR("$pid_time requires a PID argument via sub-expression");
  }
}

enum class xid_type { uid, euid, suid, fsuid, gid, egid, sgid, fsgid };

struct xid_info {
  const char *field;   // "Uid:\t" or "Gid:\t"
  int column;          // tab-separated column index (0-3)
  const char *label;   // human-readable name for error messages
};

// Indexed by xid_type — order must match the enum
static constexpr xid_info xid_table[] = {
    {"Uid:\t", 0, "real uid"},
    {"Uid:\t", 1, "effective uid"},
    {"Uid:\t", 2, "saved set uid"},
    {"Uid:\t", 3, "process file system uid"},
    {"Gid:\t", 0, "real gid"},
    {"Gid:\t", 1, "effective gid"},
    {"Gid:\t", 2, "saved set gid"},
    {"Gid:\t", 3, "process file system gid"},
};

template <xid_type type>
void print_pid_Xid(struct text_object *obj, char *p, unsigned int p_max_size) {
  constexpr auto &info = xid_table[static_cast<size_t>(type)];
  auto objbuf = eval_sub_arg(obj);
  auto path = process_directory / objbuf.c_str() / "status";

  auto buf = readfile(path);
  if (!buf) { return; }

  const char *begin = strstr(buf->c_str(), info.field);
  if (begin == nullptr) {
    LOG_ERROR("failed to find {} field in proc status file '{}'", info.label, path.c_str());
    return;
  }

  // Skip to the right tab-separated column
  begin += strlen(info.field);
  for (int i = 0; i < info.column; i++) {
    begin = strchr(begin, '\t');
    if (begin == nullptr) { return; }
    begin++;
  }

  // Last column ends at newline, others at tab
  const char *end = strchr(begin, info.column == 3 ? '\n' : '\t');
  if (end != nullptr) {
    snprintf(p, p_max_size, "%.*s", static_cast<int>(end - begin), begin);
  } else {
    snprintf(p, p_max_size, "%s", begin);
  }
}



enum class proc_field {
  vmpeak, vmsize, vmlck, vmhwm, vmrss, vmdata, vmstk, vmexe, vmlib, vmpte,
  read_bytes, write_bytes,
  ppid, threads
};

struct proc_field_info {
  const char *file;    // proc subpath ("status" or "io")
  const char *entry;   // field prefix to search for
  const char *label;   // human-readable name for errors
};

static constexpr proc_field_info proc_field_table[] = {
    {"status", "VmPeak:\t", "peak virtual memory size"},
    {"status", "VmSize:\t", "virtual memory size"},
    {"status", "VmLck:\t", "locked memory size"},
    {"status", "VmHWM:\t", "peak resident set size"},
    {"status", "VmRSS:\t", "resident set size"},
    {"status", "VmData:\t", "data segment size"},
    {"status", "VmStk:\t", "stack segment size"},
    {"status", "VmExe:\t", "text segment size"},
    {"status", "VmLib:\t", "shared library code size"},
    {"status", "VmPTE:\t", "page table entries size"},
    {"io", "read_bytes: ", "bytes read"},
    {"io", "write_bytes: ", "bytes written"},
    {"status", "PPid:\t", "parent pid"},
    {"status", "Threads:\t", "thread count"},
};

template <proc_field field>
void print_proc_field(struct text_object *obj, char *p,
                      unsigned int p_max_size) {
  constexpr auto &info = proc_field_table[static_cast<int>(field)];
  auto objbuf = eval_sub_arg(obj);
  auto path = process_directory / objbuf.c_str() / info.file;

  auto buf = readfile(path);
  if (!buf) { return; }

  const char *begin = strstr(buf->c_str(), info.entry);
  if (begin == nullptr) {
    LOG_ERROR("failed to find {} field in proc file '{}'", info.label, path.c_str());
    return;
  }

  begin += strlen(info.entry);
  while (*begin == '\t' || *begin == ' ') { begin++; }
  const char *end = strchr(begin, '\n');
  if (end != nullptr) {
    snprintf(p, p_max_size, "%.*s", static_cast<int>(end - begin), begin);
  } else {
    snprintf(p, p_max_size, "%s", begin);
  }
}

using namespace conky::text_object;

// clang-format off
CONKY_REGISTER_VARIABLES(
    {"cmdline_to_pid", [](text_object *obj, const construct_context &ctx) {
      scan_cmdline_to_pid_arg(obj, ctx.arg, ctx.free_at_crash);
      obj->callbacks.print = &print_cmdline_to_pid;
      obj->callbacks.free = &gen_free_opaque;
    }, nullptr, {}, obj_flags::arg},

    arg_object_variable<&print_pid_readlink<pid_link::root>>("pid_chroot"),
    arg_object_variable<&print_pid_readlink<pid_link::cwd>>("pid_cwd"),
    arg_object_variable<&print_pid_readlink<pid_link::exe>>("pid_exe"),
    arg_object_variable<&print_pid_readlink<pid_link::fd0>>("pid_stdin"),
    arg_object_variable<&print_pid_readlink<pid_link::fd1>>("pid_stdout"),
    arg_object_variable<&print_pid_readlink<pid_link::fd2>>("pid_stderr"),

    arg_object_variable<&print_pid_Xid<xid_type::uid>>("pid_uid"),
    arg_object_variable<&print_pid_Xid<xid_type::euid>>("pid_euid"),
    arg_object_variable<&print_pid_Xid<xid_type::suid>>("pid_suid"),
    arg_object_variable<&print_pid_Xid<xid_type::fsuid>>("pid_fsuid"),
    arg_object_variable<&print_pid_Xid<xid_type::gid>>("pid_gid"),
    arg_object_variable<&print_pid_Xid<xid_type::egid>>("pid_egid"),
    arg_object_variable<&print_pid_Xid<xid_type::sgid>>("pid_sgid"),
    arg_object_variable<&print_pid_Xid<xid_type::fsgid>>("pid_fsgid"),

    arg_object_variable<&print_proc_field<proc_field::ppid>>("pid_parent"),
    arg_object_variable<&print_proc_field<proc_field::threads>>("pid_threads"),
    arg_object_variable<&print_proc_field<proc_field::vmpeak>>("pid_vmpeak"),
    arg_object_variable<&print_proc_field<proc_field::vmsize>>("pid_vmsize"),
    arg_object_variable<&print_proc_field<proc_field::vmlck>>("pid_vmlck"),
    arg_object_variable<&print_proc_field<proc_field::vmhwm>>("pid_vmhwm"),
    arg_object_variable<&print_proc_field<proc_field::vmrss>>("pid_vmrss"),
    arg_object_variable<&print_proc_field<proc_field::vmdata>>("pid_vmdata"),
    arg_object_variable<&print_proc_field<proc_field::vmstk>>("pid_vmstk"),
    arg_object_variable<&print_proc_field<proc_field::vmexe>>("pid_vmexe"),
    arg_object_variable<&print_proc_field<proc_field::vmlib>>("pid_vmlib"),
    arg_object_variable<&print_proc_field<proc_field::vmpte>>("pid_vmpte"),
    arg_object_variable<&print_proc_field<proc_field::read_bytes>>("pid_read"),
    arg_object_variable<&print_proc_field<proc_field::write_bytes>>("pid_write"),

    arg_object_variable<&print_pid_cmdline>("pid_cmdline"),
    arg_object_variable<&print_pid_environ>("pid_environ"),
    arg_object_variable<&print_pid_environ_list>("pid_environ_list"),
    arg_object_variable<&print_pid_nice>("pid_nice"),
    arg_object_variable<&print_pid_openfiles>("pid_openfiles"),
    arg_object_variable<&print_pid_priority>("pid_priority"),
    arg_object_variable<&print_pid_state>("pid_state"),
    arg_object_variable<&print_pid_state_short>("pid_state_short"),
    arg_object_variable<&print_pid_thread_list>("pid_thread_list"),
    arg_object_variable<&print_pid_time>("pid_time"),
    arg_object_variable<&print_pid_time_kernelmode>("pid_time_kernelmode"),
    arg_object_variable<&print_pid_time_usermode>("pid_time_usermode"),
)
// clang-format on
