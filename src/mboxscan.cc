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
 * Copyright (c) 2006 Marco Candrian <mac@calmar.ws>
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

#include <sys/stat.h>
#include <sys/time.h>
#include <cerrno>
#include <memory>
#include "conky.h"
#include "logging.h"
#include "mail.h"
#include "text_object.h"

#define FROM_WIDTH 10
#define SUBJECT_WIDTH 22
#define PRINT_MAILS 5
#define TIME_DELAY 5

struct ring_list {
  char *from;
  char *subject;
  struct ring_list *previous;
  struct ring_list *next;
};

static time_t last_ctime; /* needed for mutt at least */
static time_t last_mtime; /* not sure what to test: testing both now */
static double last_update;

static int args_ok = 0;
static int from_width;
static int subject_width;
static int print_num_mails;
static int time_delay;

static char mbox_mail_spool[DEFAULT_TEXT_BUFFER_SIZE];

static void mbox_scan(char *args, char *output, size_t max_len) {
  int i, u, flag;
  int force_rescan = 0;
  std::unique_ptr<char[]> buf_(new char[text_buffer_size.get(*state)]);
  char *buf = buf_.get();
  struct stat statbuf {};
  struct ring_list *curr = nullptr, *prev = nullptr, *startlist = nullptr;
  FILE *fp;

  /* output was set to 1 after malloc'ing in conky.c */
  /* -> beeing able to test it here for catching SIGUSR1 */
  if (output[0] == 1) {
    force_rescan = 1;
    output[0] = '\0';
  }

  if ((args_ok == 0) || (force_rescan != 0)) {
    char *substr = strstr(args, "-n");

    if (substr != nullptr) {
      if (sscanf(substr, "-n %i", &print_num_mails) != 1) {
        print_num_mails = PRINT_MAILS;
      }
    } else {
      print_num_mails = PRINT_MAILS;
    }
    if (print_num_mails < 1) {
      print_num_mails = 1;
    }

    substr = strstr(args, "-t");
    if (substr != nullptr) {
      if (sscanf(substr, "-t %i", &time_delay) != 1) {
        time_delay = TIME_DELAY;
      }
    } else {
      time_delay = TIME_DELAY;
    }

    substr = strstr(args, "-fw");
    if (substr != nullptr) {
      if (sscanf(substr, "-fw %i", &from_width) != 1) {
        from_width = FROM_WIDTH;
      }
    } else {
      from_width = FROM_WIDTH;
    }

    substr = strstr(args, "-sw");
    if (substr != nullptr) {
      if (sscanf(substr, "-sw %i", &subject_width) != 1) {
        subject_width = SUBJECT_WIDTH;
      }
    } else {
      subject_width = SUBJECT_WIDTH;
    }
    /* encapsulated with "'s find first occurrence of " */
    if (args[strlen(args) - 1] == '"') {
      char *start;
      strncpy(mbox_mail_spool, args, DEFAULT_TEXT_BUFFER_SIZE);
      start = strchr(mbox_mail_spool, '"') + 1;

      start[(strrchr(mbox_mail_spool, '"') - start)] = '\0';
      strncpy(mbox_mail_spool, start, DEFAULT_TEXT_BUFFER_SIZE);
    } else {
      char *copy_args = strndup(args, text_buffer_size.get(*state));
      char *tmp = strtok(copy_args, " ");
      char *start = tmp;

      while (tmp != nullptr) {
        tmp = strtok(nullptr, " ");
        if (tmp != nullptr) {
          start = tmp;
        }
      }
      strncpy(mbox_mail_spool, start, DEFAULT_TEXT_BUFFER_SIZE);
      free(copy_args);
    }
    if (strlen(mbox_mail_spool) < 1) {
      CRIT_ERR(nullptr, nullptr,
               "Usage: ${mboxscan [-n <number of messages to print>] "
               "[-fw <from width>] [-sw <subject width>] "
               "[-t <delay in sec> mbox]}");
    }

    /* allowing $MAIL in the config */
    if (strcmp(mbox_mail_spool, "$MAIL") == 0) {
      strcpy(mbox_mail_spool, current_mail_spool.get(*state).c_str());
    }

    if (stat(mbox_mail_spool, &statbuf) != 0) {
      CRIT_ERR(nullptr, nullptr, "can't stat %s: %s", mbox_mail_spool,
               strerror(errno));
    }
    args_ok = 1; /* args-computing necessary only once */
  }

  /* if time_delay not yet reached, then return */
  if (current_update_time - last_update < time_delay && (force_rescan == 0)) {
    return;
  }

  last_update = current_update_time;

  /* mbox still exists? and get stat-infos */
  if (stat(mbox_mail_spool, &statbuf) != 0) {
    NORM_ERR("can't stat %s: %s", mbox_mail_spool, strerror(errno));
    output[0] = '\0'; /* delete any output */
    return;
  }

  /* modification time has not changed, so skip scanning the box */
  if (statbuf.st_ctime == last_ctime && statbuf.st_mtime == last_mtime &&
      (force_rescan == 0)) {
    return;
  }

  last_ctime = statbuf.st_ctime;
  last_mtime = statbuf.st_mtime;

  /* build up double-linked ring-list to hold data, while scanning down the
   * mbox */
  for (i = 0; i < print_num_mails; i++) {
    curr = static_cast<struct ring_list *>(malloc(sizeof(struct ring_list)));
    curr->from = static_cast<char *>(malloc(from_width + 1));
    curr->subject = static_cast<char *>(malloc(subject_width + 1));
    curr->from[0] = '\0';
    curr->subject[0] = '\0';

    if (i == 0) {
      startlist = curr;
    }
    if (i > 0) {
      curr->previous = prev;
      prev->next = curr;
    }
    prev = curr;
  }

  if (startlist == nullptr) {
    return;
  }
  /* connect end to start for an endless loop-ring */
  startlist->previous = curr;
  curr->next = startlist;

  /* mbox */
  fp = fopen(mbox_mail_spool, "re");
  if (fp == nullptr) {
    return;
  }

  /* first find a "From " to set it to 0 for header-sarchings */
  flag = 1;
  while (feof(fp) == 0) {
    if (fgets(buf, text_buffer_size.get(*state), fp) == nullptr) {
      break;
    }

    if (strncmp(buf, "From ", 5) == 0) {
      curr = curr->next;

      /* skip until \n */
      while (strchr(buf, '\n') == nullptr && (feof(fp) == 0)) {
        if (fgets(buf, text_buffer_size.get(*state), fp) == nullptr) {
          break;
        }
      }

      flag = 0; /* in the headers now */
      continue;
    }

    if (flag == 1) { /* in the body, so skip */
      continue;
    }

    if (buf[0] == '\n') {
      /* beyond the headers now (empty line), skip until \n */
      /* then search for new mail ("From ") */

      while (strchr(buf, '\n') == nullptr && (feof(fp) == 0)) {
        if (fgets(buf, text_buffer_size.get(*state), fp) == nullptr) {
          break;
        }
      }
      flag = 1; /* in the body now */
      continue;
    }

    if ((strncmp(buf, "X-Status: ", 10) == 0) ||
        (strncmp(buf, "Status: R", 9) == 0)) {
      /* Mail was read or something, so skip that message */
      flag = 1; /* search for next From */
      curr->subject[0] = '\0';
      curr->from[0] = '\0';
      /* (will get current again on new 'From ' finding) */
      curr = curr->previous;
      /* Skip until \n */
      while (strchr(buf, '\n') == nullptr && (feof(fp) == 0)) {
        if (fgets(buf, text_buffer_size.get(*state), fp) == nullptr) {
          break;
        }
      }
      continue;
    }

    /* that covers ^From: and ^from: ^From:<tab> */
    if (strncmp(buf + 1, "rom:", 4) == 0) {
      i = 0;
      u = 6; /* no "From: " string needed, so skip */
      while (1) {
        if (buf[u] == '"') { /* no quotes around names */
          u++;
          continue;
        }

        /* some are: From: <foo@bar.com> */
        if (buf[u] == '<' && i > 1) {
          curr->from[i] = '\0';
          /* skip until \n */
          while (strchr(buf, '\n') == nullptr && (feof(fp) == 0)) {
            if (fgets(buf, text_buffer_size.get(*state), fp) == nullptr) {
              break;
            }
          }
          break;
        }

        if (buf[u] == '\n') {
          curr->from[i] = '\0';
          break;
        }

        if (buf[u] == '\0') {
          curr->from[i] = '\0';
          break;
        }

        if (i >= from_width) {
          curr->from[i] = '\0';
          /* skip until \n */
          while (strchr(buf, '\n') == nullptr && (feof(fp) == 0)) {
            if (fgets(buf, text_buffer_size.get(*state), fp) == nullptr) {
              break;
            }
          }
          break;
        }

        /* nothing special so just set it */
        curr->from[i++] = buf[u++];
      }
    }

    /* that covers ^Subject: and ^subject: and ^Subjec:<tab> */
    if (strncmp(buf + 1, "ubject:", 7) == 0) {
      i = 0;
      u = 9; /* no "Subject: " string needed, so skip */
      while (1) {
        if (buf[u] == '\n') {
          curr->subject[i] = '\0';
          break;
        }
        if (buf[u] == '\0') {
          curr->subject[i] = '\0';
          break;
        }
        if (i >= subject_width) {
          curr->subject[i] = '\0';

          /* skip until \n */
          while (strchr(buf, '\n') == nullptr && (feof(fp) == 0)) {
            if (fgets(buf, text_buffer_size.get(*state), fp) == nullptr) {
              break;
            }
          }
          break;
        }

        /* nothing special so just set it */
        curr->subject[i++] = buf[u++];
      }
    }
  }

  fclose(fp);

  output[0] = '\0';

  i = print_num_mails;
  while (i != 0) {
    struct ring_list *tmp;
    if (curr->from[0] != '\0') {
      if (i != print_num_mails) {
        snprintf(buf, text_buffer_size.get(*state), "\nF: %-*s S: %-*s",
                 from_width, curr->from, subject_width, curr->subject);
      } else { /* first time - no \n in front */
        snprintf(buf, text_buffer_size.get(*state), "F: %-*s S: %-*s",
                 from_width, curr->from, subject_width, curr->subject);
      }
    } else {
      snprintf(buf, text_buffer_size.get(*state), "\n");
    }
    strncat(output, buf, max_len - strlen(output));

    tmp = curr;
    curr = curr->previous;
    free(tmp->from);
    free(tmp->subject);
    free(tmp);

    i--;
  }
}

struct mboxscan_data {
  char *args;
  char *output;
};

void parse_mboxscan_arg(struct text_object *obj, const char *arg) {
  struct mboxscan_data *msd;

  msd = static_cast<mboxscan_data *>(malloc(sizeof(struct mboxscan_data)));
  memset(msd, 0, sizeof(struct mboxscan_data));

  msd->args = strndup(arg, text_buffer_size.get(*state));
  msd->output = static_cast<char *>(malloc(text_buffer_size.get(*state)));
  /* if '1' (in mboxscan.c) then there was SIGUSR1, hmm */
  msd->output[0] = 1;

  obj->data.opaque = msd;
}

void print_mboxscan(struct text_object *obj, char *p, int p_max_size) {
  auto *msd = static_cast<mboxscan_data *>(obj->data.opaque);

  if (msd == nullptr) {
    return;
  }

  mbox_scan(msd->args, msd->output, text_buffer_size.get(*state));
  snprintf(p, p_max_size, "%s", msd->output);
}

void free_mboxscan(struct text_object *obj) {
  auto *msd = static_cast<mboxscan_data *>(obj->data.opaque);

  if (msd == nullptr) {
    return;
  }
  free_and_zero(msd->args);
  free_and_zero(msd->output);
  free_and_zero(obj->data.opaque);
}
