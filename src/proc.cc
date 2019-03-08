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

#include "proc.h"
#include <dirent.h>
#include <unistd.h>
#include <cctype>
#include <memory>
#include <sstream>
#include "conky.h"
#include "core.h"
#include "logging.h"

char *readfile(const char *filename, int *total_read, char showerror) {
  FILE *file;
  char *buf = nullptr;
  int bytes_read;

  *total_read = 0;
  file = fopen(filename, "re");
  if (file != nullptr) {
    do {
      buf = static_cast<char *>(realloc(buf, *total_read + READSIZE + 1));
      bytes_read = fread(buf + *total_read, 1, READSIZE, file);
      *total_read += bytes_read;
      buf[*total_read] = 0;
    } while (bytes_read != 0);
    fclose(file);
  } else if (showerror != 0) {
    NORM_ERR(READERR, filename);
  }
  return buf;
}

void pid_readlink(const char *file, char *p, unsigned int p_max_size) {
  std::unique_ptr<char[]> buf(new char[p_max_size]);

  memset(buf.get(), 0, p_max_size);
  if (readlink(file, buf.get(), p_max_size) >= 0) {
    snprintf(p, p_max_size, "%s", buf.get());
  } else {
    NORM_ERR(READERR, file);
  }
}

struct ll_string {
  char *string;
  struct ll_string *next;
};

struct ll_string *addnode(struct ll_string *end, char *string) {
  auto *current =
      static_cast<struct ll_string *>(malloc(sizeof(struct ll_string)));
  current->string = strdup(string);
  current->next = nullptr;
  if (end != nullptr) { end->next = current; }
  return current;
}

void freelist(struct ll_string *front) {
  if (front != nullptr) {
    free(front->string);
    if (front->next != nullptr) { freelist(front->next); }
    free(front);
  }
}

int inlist(struct ll_string *front, char *string) {
  struct ll_string *current;

  for (current = front; current != nullptr; current = current->next) {
    if (strcmp(current->string, string) == 0) { return 1; }
  }
  return 0;
}

void print_pid_chroot(struct text_object *obj, char *p,
                      unsigned int p_max_size) {
  std::ostringstream pathstream;
  std::unique_ptr<char[]> buf(new char[max_user_text.get(*state)]);

  generate_text_internal(buf.get(), max_user_text.get(*state), *obj->sub);
  pathstream << PROCDIR "/" << buf.get() << "/root";
  pid_readlink(pathstream.str().c_str(), p, p_max_size);
}

void print_pid_cmdline(struct text_object *obj, char *p,
                       unsigned int p_max_size) {
  char *buf;
  int i, bytes_read;
  std::ostringstream pathstream;
  std::unique_ptr<char[]> objbuf(new char[max_user_text.get(*state)]);

  generate_text_internal(objbuf.get(), max_user_text.get(*state), *obj->sub);

  if (*(objbuf.get()) != 0) {
    pathstream << PROCDIR "/" << objbuf.get() << "/cmdline";
    buf = readfile(pathstream.str().c_str(), &bytes_read, 1);
    if (buf != nullptr) {
      for (i = 0; i < bytes_read - 1; i++) {
        if (buf[i] == 0) { buf[i] = ' '; }
      }
      snprintf(p, p_max_size, "%s", buf);
      free(buf);
    }
  } else {
    NORM_ERR("$pid_cmdline didn't receive a argument");
  }
}

void print_pid_cwd(struct text_object *obj, char *p, unsigned int p_max_size) {
  std::unique_ptr<char[]> buf(new char[p_max_size]);
  int bytes_read;
  std::ostringstream pathstream;
  std::unique_ptr<char[]> objbuf(new char[max_user_text.get(*state)]);

  generate_text_internal(objbuf.get(), max_user_text.get(*state), *obj->sub);
  pathstream << PROCDIR "/" << objbuf.get() << "/cwd";
  bytes_read = readlink(pathstream.str().c_str(), buf.get(), p_max_size);
  if (bytes_read != -1) {
    buf[bytes_read] = 0;
    snprintf(p, p_max_size, "%s", buf.get());
  } else {
    NORM_ERR(READERR, pathstream.str().c_str());
  }
}

void print_pid_environ(struct text_object *obj, char *p,
                       unsigned int p_max_size) {
  int i, total_read;
  pid_t pid;
  std::ostringstream pathstream;
  std::unique_ptr<char[]> objbuf(new char[max_user_text.get(*state)]);
  char *buf, *var = strdup(obj->data.s);
  ;

  generate_text_internal(objbuf.get(), max_user_text.get(*state), *obj->sub);
  if (sscanf(objbuf.get(), "%d %s", &pid, var) == 2) {
    for (i = 0; var[i] != 0; i++) {
      var[i] = toupper(static_cast<unsigned char>(var[i]));
    }
    pathstream << PROCDIR "/" << pid << "/cwd";
    buf = readfile(pathstream.str().c_str(), &total_read, 1);
    if (buf != nullptr) {
      for (i = 0; i < total_read; i += strlen(buf + i) + 1) {
        if (strncmp(buf + i, var, strlen(var)) == 0 &&
            *(buf + i + strlen(var)) == '=') {
          snprintf(p, p_max_size, "%s", buf + i + strlen(var) + 1);
          free(buf);
          free(var);
          return;
        }
      }
      free(buf);
    }
    *p = 0;
  }
  free(var);
}

void print_pid_environ_list(struct text_object *obj, char *p,
                            unsigned int p_max_size) {
  char *buf = nullptr;
  char *buf2;
  int bytes_read, total_read;
  int i = 0;
  std::ostringstream pathstream;
  std::unique_ptr<char[]> objbuf(new char[max_user_text.get(*state)]);

  generate_text_internal(objbuf.get(), max_user_text.get(*state), *obj->sub);
  pathstream << PROCDIR "/" << objbuf.get() << "/environ";

  buf = readfile(pathstream.str().c_str(), &total_read, 1);
  if (buf != nullptr) {
    for (bytes_read = 0; bytes_read < total_read; buf[i - 1] = ';') {
      buf2 = strdup(buf + bytes_read);
      bytes_read += strlen(buf2) + 1;
      sscanf(buf2, "%[^=]", buf + i);
      free(buf2);
      i = strlen(buf) + 1;
    }
    buf[i - 1] = 0;
    snprintf(p, p_max_size, "%s", buf);
    free(buf);
  }
}

void print_pid_exe(struct text_object *obj, char *p, unsigned int p_max_size) {
  std::ostringstream pathstream;
  std::unique_ptr<char[]> objbuf(new char[max_user_text.get(*state)]);

  generate_text_internal(objbuf.get(), max_user_text.get(*state), *obj->sub);
  pathstream << PROCDIR "/" << objbuf.get() << "/exe";
  pid_readlink(pathstream.str().c_str(), p, p_max_size);
}

void print_pid_nice(struct text_object *obj, char *p, unsigned int p_max_size) {
  char *buf = nullptr;
  int bytes_read;
  long int nice_value;
  std::ostringstream pathstream;
  std::unique_ptr<char[]> objbuf(new char[max_user_text.get(*state)]);

  generate_text_internal(objbuf.get(), max_user_text.get(*state), *obj->sub);

  if (!obj->data.s) {
    pathstream << PROCDIR "/" << objbuf.get() << "/stat";
    buf = readfile(pathstream.str().c_str(), &bytes_read, 1);
    if (buf != nullptr) {
      sscanf(buf,
             "%*d %*s %*c %*d %*d %*d %*d %*d %*u %*u %*u %*u %*u %*u %*u %*d "
             "%*d %*d %ld",
             &nice_value);
      snprintf(p, p_max_size, "%ld", nice_value);
      free(buf);
    }
  } else {
    NORM_ERR("$pid_nice didn't receive a argument");
  }
}

void print_pid_openfiles(struct text_object *obj, char *p,
                         unsigned int p_max_size) {
  DIR *dir;
  struct dirent *entry;
  std::unique_ptr<char[]> buf(new char[p_max_size]);
  int length, totallength = 0;
  struct ll_string *files_front = nullptr;
  struct ll_string *files_back = nullptr;
  std::unique_ptr<char[]> objbuf(new char[max_user_text.get(*state)]);

  generate_text_internal(objbuf.get(), max_user_text.get(*state), *obj->sub);

  dir = opendir(objbuf.get());
  if (dir != nullptr) {
    while ((entry = readdir(dir)) != nullptr) {
      if (entry->d_name[0] != '.') {
        snprintf(buf.get(), p_max_size, "%s/%s", objbuf.get(), entry->d_name);
        length = readlink(buf.get(), buf.get(), p_max_size);
        buf[length] = 0;
        if (inlist(files_front, buf.get()) == 0) {
          files_back = addnode(files_back, buf.get());
          snprintf(p + totallength, p_max_size - totallength, "%s; ",
                   buf.get());
          totallength += length + strlen("; ");
        }
        if (files_front == nullptr) { files_front = files_back; }
      }
    }
    closedir(dir);
    freelist(files_front);
    p[totallength - strlen("; ")] = 0;
  } else {
    p[0] = 0;
  }
}

void print_pid_parent(struct text_object *obj, char *p,
                      unsigned int p_max_size) {
#define PARENT_ENTRY "PPid:\t"
#define PARENTNOTFOUND "Can't find the process parent in '%s'"
  char *begin, *end, *buf = nullptr;
  int bytes_read;
  std::ostringstream pathstream;
  std::unique_ptr<char[]> objbuf(new char[max_user_text.get(*state)]);

  generate_text_internal(objbuf.get(), max_user_text.get(*state), *obj->sub);
  pathstream << PROCDIR "/" << objbuf.get() << "/status";

  buf = readfile(pathstream.str().c_str(), &bytes_read, 1);
  if (buf != nullptr) {
    begin = strstr(buf, PARENT_ENTRY);
    if (begin != nullptr) {
      begin += strlen(PARENT_ENTRY);
      end = strchr(begin, '\n');
      if (end != nullptr) { *(end) = 0; }
      snprintf(p, p_max_size, "%s", begin);
    } else {
      NORM_ERR(PARENTNOTFOUND, pathstream.str().c_str());
    }
    free(buf);
  }
}

void print_pid_priority(struct text_object *obj, char *p,
                        unsigned int p_max_size) {
  char *buf = nullptr;
  int bytes_read;
  long int priority;
  std::ostringstream pathstream;
  std::unique_ptr<char[]> objbuf(new char[max_user_text.get(*state)]);

  generate_text_internal(objbuf.get(), max_user_text.get(*state), *obj->sub);

  if (*(objbuf.get()) != 0) {
    pathstream << PROCDIR "/" << objbuf.get() << "/stat";
    buf = readfile(pathstream.str().c_str(), &bytes_read, 1);
    if (buf != nullptr) {
      sscanf(buf,
             "%*d %*s %*c %*d %*d %*d %*d %*d %*u %*u %*u %*u %*u %*u %*u %*d "
             "%*d %ld",
             &priority);
      snprintf(p, p_max_size, "%ld", priority);
      free(buf);
    }
  } else {
    NORM_ERR("$pid_priority didn't receive a argument");
  }
}

void print_pid_state(struct text_object *obj, char *p,
                     unsigned int p_max_size) {
#define STATE_ENTRY "State:\t"
#define STATENOTFOUND "Can't find the process state in '%s'"
  char *begin, *end, *buf = nullptr;
  int bytes_read;
  std::ostringstream pathstream;
  std::unique_ptr<char[]> objbuf(new char[max_user_text.get(*state)]);

  generate_text_internal(objbuf.get(), max_user_text.get(*state), *obj->sub);
  pathstream << PROCDIR "/" << objbuf.get() << "/status";

  buf = readfile(pathstream.str().c_str(), &bytes_read, 1);
  if (buf != nullptr) {
    begin = strstr(buf, STATE_ENTRY);
    if (begin != nullptr) {
      begin += strlen(STATE_ENTRY) +
               3;  // +3 will strip the char representing the short state and
                   // the space and '(' that follow
      end = strchr(begin, '\n');
      if (end != nullptr) {
        *(end - 1) = 0;  // -1 strips the ')'
      }
      snprintf(p, p_max_size, "%s", begin);
    } else {
      NORM_ERR(STATENOTFOUND, pathstream.str().c_str());
    }
    free(buf);
  }
}

void print_pid_state_short(struct text_object *obj, char *p,
                           unsigned int p_max_size) {
  char *begin, *buf = nullptr;
  int bytes_read;
  std::ostringstream pathstream;
  std::unique_ptr<char[]> objbuf(new char[max_user_text.get(*state)]);

  generate_text_internal(objbuf.get(), max_user_text.get(*state), *obj->sub);

  pathstream << PROCDIR "/" << objbuf.get() << "/status";

  buf = readfile(pathstream.str().c_str(), &bytes_read, 1);
  if (buf != nullptr) {
    begin = strstr(buf, STATE_ENTRY);
    if (begin != nullptr) {
      snprintf(p, p_max_size, "%c", *begin);
    } else {
      NORM_ERR(STATENOTFOUND, pathstream.str().c_str());
    }
    free(buf);
  }
}

void print_pid_stderr(struct text_object *obj, char *p,
                      unsigned int p_max_size) {
  std::ostringstream pathstream;
  std::unique_ptr<char[]> objbuf(new char[max_user_text.get(*state)]);

  generate_text_internal(objbuf.get(), max_user_text.get(*state), *obj->sub);

  pathstream << PROCDIR "/" << objbuf.get() << "/fd/2";
  pid_readlink(pathstream.str().c_str(), p, p_max_size);
}

void print_pid_stdin(struct text_object *obj, char *p,
                     unsigned int p_max_size) {
  std::unique_ptr<char[]> objbuf(new char[max_user_text.get(*state)]);
  std::ostringstream pathstream;

  generate_text_internal(objbuf.get(), max_user_text.get(*state), *obj->sub);

  pathstream << PROCDIR "/" << objbuf.get() << "/fd/0";
  pid_readlink(pathstream.str().c_str(), p, p_max_size);
}

void print_pid_stdout(struct text_object *obj, char *p,
                      unsigned int p_max_size) {
  std::ostringstream pathstream;
  std::unique_ptr<char[]> objbuf(new char[max_user_text.get(*state)]);

  generate_text_internal(objbuf.get(), max_user_text.get(*state), *obj->sub);

  pathstream << PROCDIR "/" << objbuf.get() << "/fd/1";
  pid_readlink(pathstream.str().c_str(), p, p_max_size);
}

void scan_cmdline_to_pid_arg(struct text_object *obj, const char *arg,
                             void *free_at_crash) {
  unsigned int i;
  std::unique_ptr<char[]> objbuf(new char[max_user_text.get(*state)]);

  /* FIXME */
  generate_text_internal(objbuf.get(), max_user_text.get(*state), *obj->sub);

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
    CRIT_ERR(obj, free_at_crash, "${cmdline_to_pid commandline}");
  }
}

void print_cmdline_to_pid(struct text_object *obj, char *p,
                          unsigned int p_max_size) {
  DIR *dir;
  struct dirent *entry;
  char *buf;
  int bytes_read, i;
  std::ostringstream pathstream;

  dir = opendir(PROCDIR);
  if (dir != nullptr) {
    while ((entry = readdir(dir)) != nullptr) {
      pathstream << PROCDIR "/" << entry->d_name << "/cmdline";

      buf = readfile(pathstream.str().c_str(), &bytes_read, 0);
      if (buf != nullptr) {
        for (i = 0; i < bytes_read - 1; i++) {
          if (buf[i] == 0) { buf[i] = ' '; }
        }
        if (strstr(buf, obj->data.s) != nullptr) {
          snprintf(p, p_max_size, "%s", entry->d_name);
          free(buf);
          closedir(dir);
          return;
        }
        free(buf);
      }
    }
    closedir(dir);
  } else {
    NORM_ERR(READERR, PROCDIR);
  }
}

void print_pid_threads(struct text_object *obj, char *p,
                       unsigned int p_max_size) {
#define THREADS_ENTRY "Threads:\t"
#define THREADSNOTFOUND \
  "Can't find the number of the threads of the process in '%s'"
  char *begin, *end, *buf = nullptr;
  int bytes_read;
  std::ostringstream pathstream;
  std::unique_ptr<char[]> objbuf(new char[max_user_text.get(*state)]);

  generate_text_internal(objbuf.get(), max_user_text.get(*state), *obj->sub);
  pathstream << PROCDIR "/" << objbuf.get() << "/status";

  buf = readfile(pathstream.str().c_str(), &bytes_read, 1);
  if (buf != nullptr) {
    begin = strstr(buf, THREADS_ENTRY);
    if (begin != nullptr) {
      begin += strlen(THREADS_ENTRY);
      end = strchr(begin, '\n');
      if (end != nullptr) { *(end) = 0; }
      snprintf(p, p_max_size, "%s", begin);
    } else {
      NORM_ERR(THREADSNOTFOUND, pathstream.str().c_str());
    }
    free(buf);
  }
}

void print_pid_thread_list(struct text_object *obj, char *p,
                           unsigned int p_max_size) {
  DIR *dir;
  struct dirent *entry;
  int totallength = 0;
  std::ostringstream pathstream;
  std::unique_ptr<char[]> objbuf(new char[max_user_text.get(*state)]);

  generate_text_internal(objbuf.get(), max_user_text.get(*state), *obj->sub);
  pathstream << PROCDIR "/" << objbuf.get() << "/task";

  dir = opendir(pathstream.str().c_str());
  if (dir != nullptr) {
    while ((entry = readdir(dir)) != nullptr) {
      if (entry->d_name[0] != '.') {
        snprintf(p + totallength, p_max_size - totallength, "%s,",
                 entry->d_name);
        totallength += strlen(entry->d_name) + 1;
      }
    }
    closedir(dir);
    if (p[totallength - 1] == ',') { p[totallength - 1] = 0; }
  } else {
    p[0] = 0;
  }
}

void print_pid_time_kernelmode(struct text_object *obj, char *p,
                               unsigned int p_max_size) {
  char *buf = nullptr;
  int bytes_read;
  unsigned long int umtime;
  std::ostringstream pathstream;
  std::unique_ptr<char[]> objbuf(new char[max_user_text.get(*state)]);

  generate_text_internal(objbuf.get(), max_user_text.get(*state), *obj->sub);

  if (*(objbuf.get()) != 0) {
    pathstream << PROCDIR "/" << objbuf.get() << "/stat";
    buf = readfile(pathstream.str().c_str(), &bytes_read, 1);
    if (buf != nullptr) {
      sscanf(buf, "%*d %*s %*c %*d %*d %*d %*d %*d %*u %*u %*u %*u %*u %lu",
             &umtime);
      snprintf(p, p_max_size, "%.2f", static_cast<float>(umtime) / 100);
      free(buf);
    }
  } else {
    NORM_ERR("$pid_time_kernelmode didn't receive a argument");
  }
}

void print_pid_time_usermode(struct text_object *obj, char *p,
                             unsigned int p_max_size) {
  char *buf = nullptr;
  int bytes_read;
  unsigned long int kmtime;
  std::ostringstream pathstream;
  std::unique_ptr<char[]> objbuf(new char[max_user_text.get(*state)]);

  generate_text_internal(objbuf.get(), max_user_text.get(*state), *obj->sub);

  if (*(objbuf.get()) != 0) {
    pathstream << PROCDIR "/" << objbuf.get() << "/stat";
    buf = readfile(pathstream.str().c_str(), &bytes_read, 1);
    if (buf != nullptr) {
      sscanf(buf, "%*d %*s %*c %*d %*d %*d %*d %*d %*u %*u %*u %*u %*u %*u %lu",
             &kmtime);
      snprintf(p, p_max_size, "%.2f", static_cast<float>(kmtime) / 100);
      free(buf);
    }
  } else {
    NORM_ERR("$pid_time_usermode didn't receive a argument");
  }
}

void print_pid_time(struct text_object *obj, char *p, unsigned int p_max_size) {
  char *buf = nullptr;
  int bytes_read;
  unsigned long int umtime, kmtime;
  std::ostringstream pathstream;
  std::unique_ptr<char[]> objbuf(new char[max_user_text.get(*state)]);

  generate_text_internal(objbuf.get(), max_user_text.get(*state), *obj->sub);

  if (*(objbuf.get()) != 0) {
    pathstream << PROCDIR "/" << objbuf.get() << "/stat";
    buf = readfile(pathstream.str().c_str(), &bytes_read, 1);
    if (buf != nullptr) {
      sscanf(buf, "%*d %*s %*c %*d %*d %*d %*d %*d %*u %*u %*u %*u %*u %lu %lu",
             &umtime, &kmtime);
      snprintf(p, p_max_size, "%.2f",
               static_cast<float>(umtime + kmtime) / 100);
      free(buf);
    }
  } else {
    NORM_ERR("$pid_time didn't receive a argument");
  }
}

enum xid_type { egid, euid, fsgid, fsuid, gid, sgid, suid, uid };

void print_pid_Xid(struct text_object *obj, char *p, int p_max_size,
                   xid_type type) {
  char *begin, *end, *buf = nullptr;
  int bytes_read;
  std::ostringstream pathstream;
  std::string errorstring;
  std::unique_ptr<char[]> objbuf(new char[max_user_text.get(*state)]);

  generate_text_internal(objbuf.get(), max_user_text.get(*state), *obj->sub);
  pathstream << PROCDIR "/" << objbuf.get() << "/status";

  buf = readfile(pathstream.str().c_str(), &bytes_read, 1);
  if (buf != nullptr) {
    switch (type) {
      case egid:
        begin = strstr(buf, "Gid:\t");
        break;
      case euid:
        begin = strstr(buf, "Uid:\t");
        break;
      case fsgid:
        begin = strstr(buf, "Gid:\t");
        break;
      case fsuid:
        begin = strstr(buf, "Uid:\t");
        break;
      case gid:
        begin = strstr(buf, "Gid:\t");
        break;
      case sgid:
        begin = strstr(buf, "Gid:\t");
        break;
      case suid:
        begin = strstr(buf, "Uid:\t");
        break;
      case uid:
        begin = strstr(buf, "Uid:\t");
        break;
      default:
        break;
    }
    if (begin != nullptr) {
      switch (type) {
        case gid:
          begin += strlen("Gid:\t");
          break;
        case uid:
          begin += strlen("Uid:\t");
          break;
        case egid:
        case euid:
          begin = strchr(begin, '\t');
          begin++;
          begin = strchr(begin, '\t');
          begin++;
          break;
        case sgid:
        case suid:
          begin = strchr(begin, '\t');
          begin++;
          begin = strchr(begin, '\t');
          begin++;
          begin = strchr(begin, '\t');
          begin++;
          break;
        case fsgid:
        case fsuid:
          begin = strchr(begin, '\t');
          begin++;
          begin = strchr(begin, '\t');
          begin++;
          begin = strchr(begin, '\t');
          begin++;
          begin = strchr(begin, '\t');
          begin++;
          break;
        default:
          break;
      }
      if (type == fsgid || type == fsuid) {
        end = strchr(begin, '\n');
      } else {
        end = strchr(begin, '\t');
      }
      if (end != nullptr) { *(end) = 0; }
      snprintf(p, p_max_size, "%s", begin);
    } else {
      errorstring = "Can't find the process ";
      switch (type) {
        case egid:
          errorstring.append("effective gid");
          break;
        case euid:
          errorstring.append("effective uid");
          break;
        case fsgid:
          errorstring.append("process file system gid");
          break;
        case fsuid:
          errorstring.append("process file system uid");
          break;
        case gid:
          errorstring.append("real gid");
          break;
        case sgid:
          errorstring.append("saved set gid");
          break;
        case suid:
          errorstring.append("saved set uid");
          break;
        case uid:
          errorstring.append("real uid");
          break;
        default:
          break;
      }
      errorstring.append(" in '%s'");
      NORM_ERR(errorstring.c_str(), pathstream.str().c_str());
    }
    free(buf);
  }
}

void print_pid_egid(struct text_object *obj, char *p, unsigned int p_max_size) {
  print_pid_Xid(obj, p, p_max_size, egid);
}

void print_pid_euid(struct text_object *obj, char *p, unsigned int p_max_size) {
  print_pid_Xid(obj, p, p_max_size, euid);
}

void print_pid_fsgid(struct text_object *obj, char *p,
                     unsigned int p_max_size) {
  print_pid_Xid(obj, p, p_max_size, fsgid);
}

void print_pid_fsuid(struct text_object *obj, char *p,
                     unsigned int p_max_size) {
  print_pid_Xid(obj, p, p_max_size, fsuid);
}

void print_pid_gid(struct text_object *obj, char *p, unsigned int p_max_size) {
  print_pid_Xid(obj, p, p_max_size, gid);
}

void print_pid_sgid(struct text_object *obj, char *p, unsigned int p_max_size) {
  print_pid_Xid(obj, p, p_max_size, sgid);
}

void print_pid_suid(struct text_object *obj, char *p, unsigned int p_max_size) {
  print_pid_Xid(obj, p, p_max_size, suid);
}

void print_pid_uid(struct text_object *obj, char *p, unsigned int p_max_size) {
  print_pid_Xid(obj, p, p_max_size, uid);
}

void internal_print_pid_vm(struct text_object *obj, char *p, int p_max_size,
                           const char *entry, const char *errorstring) {
  char *begin, *end, *buf = nullptr;
  int bytes_read;
  std::ostringstream pathstream;
  std::unique_ptr<char[]> objbuf(new char[max_user_text.get(*state)]);

  generate_text_internal(objbuf.get(), max_user_text.get(*state), *obj->sub);
  pathstream << PROCDIR "/" << objbuf.get() << "/status";

  buf = readfile(pathstream.str().c_str(), &bytes_read, 1);
  if (buf != nullptr) {
    begin = strstr(buf, entry);
    if (begin != nullptr) {
      begin += strlen(entry);
      while (*begin == '\t' || *begin == ' ') { begin++; }
      end = strchr(begin, '\n');
      if (end != nullptr) { *(end) = 0; }
      snprintf(p, p_max_size, "%s", begin);
    } else {
      NORM_ERR(errorstring, pathstream.str().c_str());
    }
    free(buf);
  }
}

void print_pid_vmpeak(struct text_object *obj, char *p,
                      unsigned int p_max_size) {
  internal_print_pid_vm(
      obj, p, p_max_size, "VmPeak:\t",
      "Can't find the process peak virtual memory size in '%s'");
}

void print_pid_vmsize(struct text_object *obj, char *p,
                      unsigned int p_max_size) {
  internal_print_pid_vm(obj, p, p_max_size, "VmSize:\t",
                        "Can't find the process virtual memory size in '%s'");
}

void print_pid_vmlck(struct text_object *obj, char *p,
                     unsigned int p_max_size) {
  internal_print_pid_vm(obj, p, p_max_size, "VmLck:\t",
                        "Can't find the process locked memory size in '%s'");
}

void print_pid_vmhwm(struct text_object *obj, char *p,
                     unsigned int p_max_size) {
  internal_print_pid_vm(
      obj, p, p_max_size, "VmHWM:\t",
      "Can't find the process peak resident set size in '%s'");
}

void print_pid_vmrss(struct text_object *obj, char *p,
                     unsigned int p_max_size) {
  internal_print_pid_vm(obj, p, p_max_size, "VmHWM:\t",
                        "Can't find the process resident set size in '%s'");
}

void print_pid_vmdata(struct text_object *obj, char *p,
                      unsigned int p_max_size) {
  internal_print_pid_vm(obj, p, p_max_size, "VmData:\t",
                        "Can't find the process data segment size in '%s'");
}

void print_pid_vmstk(struct text_object *obj, char *p,
                     unsigned int p_max_size) {
  internal_print_pid_vm(obj, p, p_max_size, "VmData:\t",
                        "Can't find the process stack segment size in '%s'");
}

void print_pid_vmexe(struct text_object *obj, char *p,
                     unsigned int p_max_size) {
  internal_print_pid_vm(obj, p, p_max_size, "VmData:\t",
                        "Can't find the process text segment size in '%s'");
}

void print_pid_vmlib(struct text_object *obj, char *p,
                     unsigned int p_max_size) {
  internal_print_pid_vm(
      obj, p, p_max_size, "VmLib:\t",
      "Can't find the process shared library code size in '%s'");
}

void print_pid_vmpte(struct text_object *obj, char *p,
                     unsigned int p_max_size) {
  internal_print_pid_vm(
      obj, p, p_max_size, "VmPTE:\t",
      "Can't find the process page table entries size in '%s'");
}

#define READ_ENTRY "read_bytes: "
#define READNOTFOUND "Can't find the amount of bytes read in '%s'"
void print_pid_read(struct text_object *obj, char *p, unsigned int p_max_size) {
  char *begin, *end, *buf = nullptr;
  int bytes_read;
  std::ostringstream pathstream;
  std::unique_ptr<char[]> objbuf(new char[max_user_text.get(*state)]);

  generate_text_internal(objbuf.get(), max_user_text.get(*state), *obj->sub);
  pathstream << PROCDIR "/" << objbuf.get() << "/io";

  buf = readfile(pathstream.str().c_str(), &bytes_read, 1);
  if (buf != nullptr) {
    begin = strstr(buf, READ_ENTRY);
    if (begin != nullptr) {
      end = strchr(begin, '\n');
      if (end != nullptr) { *(end) = 0; }
      snprintf(p, p_max_size, "%s", begin);
    } else {
      NORM_ERR(READNOTFOUND, pathstream.str().c_str());
    }
    free(buf);
  }
}

#define WRITE_ENTRY "write_bytes: "
#define WRITENOTFOUND "Can't find the amount of bytes written in '%s'"
void print_pid_write(struct text_object *obj, char *p,
                     unsigned int p_max_size) {
  char *begin, *end, *buf = nullptr;
  int bytes_read;
  std::ostringstream pathstream;
  std::unique_ptr<char[]> objbuf(new char[max_user_text.get(*state)]);

  generate_text_internal(objbuf.get(), max_user_text.get(*state), *obj->sub);
  pathstream << PROCDIR "/" << objbuf.get() << "/io";

  buf = readfile(pathstream.str().c_str(), &bytes_read, 1);
  if (buf != nullptr) {
    begin = strstr(buf, WRITE_ENTRY);
    if (begin != nullptr) {
      end = strchr(begin, '\n');
      if (end != nullptr) { *(end) = 0; }
      snprintf(p, p_max_size, "%s", begin);
    } else {
      NORM_ERR(WRITENOTFOUND, pathstream.str().c_str());
    }
    free(buf);
  }
}
