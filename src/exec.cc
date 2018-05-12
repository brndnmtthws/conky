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
 * Copyright (c) 2005-2018 Brenden Matthews, Philip Kovacs, et. al.
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

#include "exec.h"
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <cmath>
#include <cstdio>
#include <mutex>
#include "conky.h"
#include "core.h"
#include "logging.h"
#include "specials.h"
#include "text_object.h"
#include "update-cb.hh"

struct execi_data {
  float interval{0};
  char *cmd{nullptr};
  execi_data() = default;
};

// our own implementation of popen, the difference : the value of 'childpid'
// will be filled with the pid of the running 'command'. This is useful if want
// to kill it when it hangs while reading or writing to it. We have to kill it
// because pclose will wait until the process dies by itself
static FILE *pid_popen(const char *command, const char *mode, pid_t *child) {
  int ends[2];
  int parentend, childend;

  // by running pipe after the strcmp's we make sure that we don't have to
  // create a pipe and close the ends if mode is something illegal
  if (strcmp(mode, "r") == 0) {
    if (pipe(ends) != 0) {
      return nullptr;
    }
    parentend = ends[0];
    childend = ends[1];
  } else if (strcmp(mode, "w") == 0) {
    if (pipe(ends) != 0) {
      return nullptr;
    }
    parentend = ends[1];
    childend = ends[0];
  } else {
    return nullptr;
  }

  *child = fork();
  if (*child == -1) {
    close(parentend);
    close(childend);
    return nullptr;
  }
  if (*child > 0) {
    close(childend);
    waitpid(*child, nullptr, 0);
  } else {
    // don't read from both stdin and pipe or write to both stdout and pipe
    if (childend == ends[0]) {
      close(0);
    } else {
      close(1);
    }
    close(parentend);

    // by dupping childend, the returned fd will have close-on-exec turned off
    if (fcntl(childend, F_DUPFD_CLOEXEC) == -1) {
      perror("dup()");
    }
    close(childend);

    execl("/bin/sh", "sh", "-c", command, (char *)nullptr);
    _exit(EXIT_FAILURE);  // child should die here, (normally execl will take
                          // care of this but it can fail)
  }

  return fdopen(parentend, mode);
}

/**
 * Executes a command and stores the result
 *
 * This function is called automatically, either once every update
 * interval, or at specific intervals in the case of execi commands.
 * conky::run_all_callbacks() handles this. In order for this magic to
 * happen, we must register a callback with conky::register_cb<exec_cb>()
 * and store it somewhere, such as obj->exec_handle. To retrieve the
 * results, use the stored callback to call get_result_copy(), which
 * returns a std::string.
 */
void exec_cb::work() {
  pid_t childpid;
  std::string buf;
  std::shared_ptr<FILE> fp;
  char b[0x1000];

  if (FILE *t = pid_popen(std::get<0>(tuple).c_str(), "r", &childpid)) {
    fp.reset(t, fclose);
  } else {
    return;
  }

  while ((feof(fp.get()) == 0) && (ferror(fp.get()) == 0)) {
    int length = fread(b, 1, sizeof b, fp.get());
    buf.append(b, length);
  }

  if (*buf.rbegin() == '\n') {
    buf.resize(buf.size() - 1);
  }

  std::lock_guard<std::mutex> l(result_mutex);
  result = buf;
}

// remove backspaced chars, example: "dog^H^H^Hcat" becomes "cat"
// string has to end with \0 and it's length should fit in a int
#define BACKSPACE 8
static void remove_deleted_chars(char *string) {
  int i = 0;
  while (string[i] != 0) {
    if (string[i] == BACKSPACE) {
      if (i != 0) {
        strcpy(&(string[i - 1]), &(string[i + 1]));
        i--;
      } else {
        strcpy(
            &(string[i]),
            &(string[i + 1]));  // necessary for ^H's at the start of a string
      }
    } else {
      i++;
    }
  }
}

/**
 * Parses command output to find a number between 0.0 and 100.0.
 * Used by ${exec[i]{bar,gauge,graph}}.
 *
 * @param[in] buf output of a command executed by an exec_cb object
 * @return number between 0.0 and 100.0
 */
static inline double get_barnum(const char *buf) {
  double barnum;

  if (sscanf(buf, "%lf", &barnum) != 1) {
    NORM_ERR(
        "reading exec value failed (perhaps it's not the "
        "correct format?)");
    return 0.0;
  }
  if (barnum > 100.0 || barnum < 0.0) {
    NORM_ERR(
        "your exec value is not between 0 and 100, "
        "therefore it will be ignored");
    return 0.0;
  }

  return barnum;
}

/**
 * Store command output in p. For execp objects, we process the output
 * in case it contains special commands like ${color}
 *
 * @param[in] buffer the output of a command
 * @param[in] obj text_object that specifies whether or not to parse
 * @param[out] p the string in which we store command output
 * @param[in] p_max_size the maximum size of p...
 */
void fill_p(const char *buffer, struct text_object *obj, char *p,
            int p_max_size) {
  if (obj->parse) {
    evaluate(buffer, p, p_max_size);
  } else {
    snprintf(p, p_max_size, "%s", buffer);
  }

  remove_deleted_chars(p);
}

/**
 * Parses arg to find the command to be run, as well as special options
 * like height, width, color, and update interval
 *
 * @param[out] obj stores the command and an execi_data structure (if
 * applicable)
 * @param[in] arg the argument to an ${exec*} object
 * @param[in] execflag bitwise flag used to specify the exec variant we need to
 * process
 */
void scan_exec_arg(struct text_object *obj, const char *arg,
                   unsigned int execflag) {
  const char *cmd = arg;
  struct execi_data *ed;

  /* in case we have an execi object, we need to parse out the interval */
  if ((execflag & EF_EXECI) != 0u) {
    ed = new execi_data;
    int n;

    /* store the interval in ed->interval */
    if (sscanf(arg, "%f %n", &ed->interval, &n) <= 0) {
      NORM_ERR("missing execi interval: ${execi* <interval> command}");
      delete ed;
      ed = nullptr;
      return;
    }

    /* set cmd to everything after the interval */
    cmd = strndup(arg + n, text_buffer_size.get(*state));
  }

  /* parse any special options for the graphical exec types */
  if ((execflag & EF_BAR) != 0u) {
    cmd = scan_bar(obj, cmd, 100);
#ifdef BUILD_X11
  } else if ((execflag & EF_GAUGE) != 0u) {
    cmd = scan_gauge(obj, cmd, 100);
  } else if ((execflag & EF_GRAPH) != 0u) {
    cmd = scan_graph(obj, cmd, 100);
    if (cmd == nullptr) {
      NORM_ERR("error parsing arguments to execgraph object");
    }
#endif /* BUILD_X11 */
  }

  /* finally, store the resulting command, or an empty string if something went
   * wrong */
  if ((execflag & EF_EXEC) != 0u) {
    obj->data.s =
        strndup(cmd != nullptr ? cmd : "", text_buffer_size.get(*state));
  } else if ((execflag & EF_EXECI) != 0u) {
    ed->cmd = strndup(cmd != nullptr ? cmd : "", text_buffer_size.get(*state));
    obj->data.opaque = ed;
  }
}

/**
 * Register an exec_cb object using the command that we have parsed
 *
 * @param[out] obj stores the callback handle
 */
void register_exec(struct text_object *obj) {
  if ((obj->data.s != nullptr) && (obj->data.s[0] != 0)) {
    obj->exec_handle = new conky::callback_handle<exec_cb>(
        conky::register_cb<exec_cb>(1, true, obj->data.s));
  } else {
    DBGP("unable to register exec callback");
  }
}

/**
 * Register an exec_cb object using the command that we have parsed.
 *
 * This version takes care of execi intervals. Note that we depend on
 * obj->thread, so be sure to run this function *after* setting obj->thread.
 *
 * @param[out] obj stores the callback handle
 */
void register_execi(struct text_object *obj) {
  auto *ed = static_cast<struct execi_data *>(obj->data.opaque);

  if ((ed != nullptr) && (ed->cmd != nullptr) && (ed->cmd[0] != 0)) {
    uint32_t period =
        std::max(lround(ed->interval / active_update_interval()), 1l);
    obj->exec_handle = new conky::callback_handle<exec_cb>(
        conky::register_cb<exec_cb>(period, !obj->thread, ed->cmd));
  } else {
    DBGP("unable to register execi callback");
  }
}

/**
 * Get the results of an exec_cb object (command output)
 *
 * @param[in] obj holds an exec_handle, assuming one was registered
 * @param[out] p the string in which we store command output
 * @param[in] p_max_size the maximum size of p...
 */
void print_exec(struct text_object *obj, char *p, int p_max_size) {
  if (obj->exec_handle != nullptr) {
    fill_p((*obj->exec_handle)->get_result_copy().c_str(), obj, p, p_max_size);
  }
}

/**
 * Get the results of a graphical (bar, gauge, graph) exec_cb object
 *
 * @param[in] obj hold an exec_handle, assuming one was registered
 * @return a value between 0.0 and 100.0
 */
double execbarval(struct text_object *obj) {
  if (obj->exec_handle != nullptr) {
    return get_barnum((*obj->exec_handle)->get_result_copy().c_str());
  }
    return 0.0;
}

/**
 * Free up any dynamically allocated data
 *
 * @param[in] obj holds the data that we need to free up
 */
void free_exec(struct text_object *obj) {
  free_and_zero(obj->data.s);
  delete obj->exec_handle;
  obj->exec_handle = nullptr;
}

/**
 * Free up any dynamically allocated data, specifically for execi objects
 *
 * @param[in] obj holds the data that we need to free up
 */
void free_execi(struct text_object *obj) {
  auto *ed = static_cast<struct execi_data *>(obj->data.opaque);

  /* if ed is nullptr, there is nothing to do */
  if (ed == nullptr) {
    return;
  }

  delete obj->exec_handle;
  obj->exec_handle = nullptr;

  free_and_zero(ed->cmd);
  delete ed;
  ed = nullptr;
  obj->data.opaque = nullptr;
}
