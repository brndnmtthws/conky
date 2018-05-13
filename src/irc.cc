/*
 *
 * Conky, a system monitor, based on torsmo
 *
 * Please see COPYING for details
 *
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

#include <libircclient.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include "conky.h"
#include "logging.h"
#include "text_object.h"

struct ll_text {
  char *text;
  struct ll_text *next;
};

struct obj_irc {
  pthread_t *thread;
  irc_session_t *session;
  char *arg;
};

struct ctx {
  char *chan;
  int max_msg_lines;
  struct ll_text *messages;
};

void ev_connected(irc_session_t *session, const char *, const char *,
                  const char **, unsigned int) {
  struct ctx *ctxptr = (struct ctx *)irc_get_ctx(session);
  if (irc_cmd_join(session, ctxptr->chan, nullptr) != 0) {
    NORM_ERR("irc: %s", irc_strerror(irc_errno(session)));
  }
}

void addmessage(struct ctx *ctxptr, char *nick, const char *text) {
  struct ll_text *lastmsg = ctxptr->messages;
  struct ll_text *newmsg = (struct ll_text *)malloc(sizeof(struct ll_text));
  newmsg->text = (char *)malloc(strlen(nick) + strlen(text) + 4);  // 4 = ": \n"
  sprintf(newmsg->text, "%s: %s\n", nick, text);
  newmsg->next = nullptr;
  int msgcnt = 1;
  if (!lastmsg) {
    ctxptr->messages = newmsg;
  } else {
    msgcnt++;
    while (lastmsg->next) {
      lastmsg = lastmsg->next;
      msgcnt++;
      if (msgcnt < 0) {
        NORM_ERR("irc: too many messages, discarding the last one.");
        free(newmsg->text);
        free(newmsg);
        return;
      }
    }
    lastmsg->next = newmsg;
  }
  if (ctxptr->max_msg_lines > 0) {
    newmsg = ctxptr->messages;
    msgcnt -= ctxptr->max_msg_lines;
    while ((msgcnt > 0) && (ctxptr->messages)) {
      msgcnt--;
      newmsg = ctxptr->messages->next;
      free(ctxptr->messages->text);
      free(ctxptr->messages);
      ctxptr->messages = newmsg;
    }
  }
}

void ev_talkinchan(irc_session_t *session, const char *, const char *origin,
                   const char **params, unsigned int) {
  char nickname[64];
  struct ctx *ctxptr = (struct ctx *)irc_get_ctx(session);

  irc_target_get_nick(origin, nickname, sizeof(nickname));
  addmessage(ctxptr, nickname, params[1]);
}

void ev_num(irc_session_t *session, unsigned int event, const char *,
            const char **params, unsigned int) {
  char attachment[4] = "_00";

  if (event == 433) {  // nick in use
    int len = strlen(params[1]) + 4;
    char *newnick = (char *)malloc(len);
    strcpy(newnick, params[1]);
    attachment[1] += rand() % 10;
    attachment[2] += rand() % 10;
    strncat(newnick, attachment, len - 1);
    irc_cmd_nick(session, newnick);
    free(newnick);
  }
}

#define IRCSYNTAX \
  "The correct syntax is ${irc server(:port) #channel (max_msg_lines)}"
#define IRCPORT 6667
#define IRCNICK "conky"
#define IRCSERVERPASS nullptr
#define IRCUSER nullptr
#define IRCREAL nullptr

void *ircclient(void *ptr) {
  struct obj_irc *ircobj = (struct obj_irc *)ptr;
  struct ctx *ctxptr = (struct ctx *)malloc(sizeof(struct ctx));
  irc_callbacks_t callbacks;
  char *server;
  char *strport;
  char *str_max_msg_lines;
  unsigned int port;

  memset(&callbacks, 0, sizeof(callbacks));
  callbacks.event_connect = ev_connected;
  callbacks.event_channel = ev_talkinchan;
  callbacks.event_numeric = ev_num;
  ircobj->session = irc_create_session(&callbacks);
  server = strtok(ircobj->arg, " ");
  ctxptr->chan = strtok(nullptr, " ");
  if (!ctxptr->chan) { NORM_ERR("irc: %s", IRCSYNTAX); }
  str_max_msg_lines = strtok(nullptr, " ");
  if (str_max_msg_lines) {
    ctxptr->max_msg_lines = strtol(str_max_msg_lines, nullptr, 10);
  }
  ctxptr->messages = nullptr;
  irc_set_ctx(ircobj->session, ctxptr);
  server = strtok(server, ":");
  strport = strtok(nullptr, ":");
  if (strport) {
    port = strtol(strport, nullptr, 10);
    if (port < 1 || port > 65535) port = IRCPORT;
  } else {
    port = IRCPORT;
  }
  int err = irc_connect(ircobj->session, server, port, IRCSERVERPASS, IRCNICK,
                        IRCUSER, IRCREAL);
  if (err != 0) { err = irc_errno(ircobj->session); }
#ifdef BUILD_IPV6
  if (err == LIBIRC_ERR_RESOLV) {
    err = irc_connect6(ircobj->session, server, port, IRCSERVERPASS, IRCNICK,
                       IRCUSER, IRCREAL);
    if (err != 0) { err = irc_errno(ircobj->session); }
  }
#endif /* BUILD_IPV6 */
  if (err != 0) { NORM_ERR("irc: %s", irc_strerror(err)); }
  if (irc_run(ircobj->session) != 0) {
    int ircerror = irc_errno(ircobj->session);
    if (irc_is_connected(ircobj->session)) {
      NORM_ERR("irc: %s", irc_strerror(ircerror));
    } else {
      NORM_ERR("irc: disconnected");
    }
  }
  free(ircobj->arg);
  free(ctxptr);
  return nullptr;
}

void parse_irc_args(struct text_object *obj, const char *arg) {
  struct obj_irc *opaque = (struct obj_irc *)malloc(sizeof(struct obj_irc));
  opaque->thread = (pthread_t *)malloc(sizeof(pthread_t));
  srand(time(nullptr));
  opaque->session = nullptr;
  opaque->arg = strdup(arg);
  pthread_create(opaque->thread, nullptr, ircclient, opaque);
  obj->data.opaque = opaque;
}

void print_irc(struct text_object *obj, char *p, int p_max_size) {
  struct obj_irc *ircobj = (struct obj_irc *)obj->data.opaque;
  struct ctx *ctxptr;
  struct ll_text *nextmsg, *curmsg;

  if (!ircobj->session) return;
  if (!irc_is_connected(ircobj->session)) return;
  ctxptr = (struct ctx *)irc_get_ctx(ircobj->session);
  curmsg = ctxptr->messages;
  while (curmsg) {
    nextmsg = curmsg->next;
    strncat(p, curmsg->text, p_max_size - strlen(p) - 1);
    if (!ctxptr->max_msg_lines) {
      free(curmsg->text);
      free(curmsg);
    }
    curmsg = nextmsg;
  }
  if (p[0] != 0) { p[strlen(p) - 1] = 0; }
  if (!ctxptr->max_msg_lines) { ctxptr->messages = nullptr; }
}

void free_irc(struct text_object *obj) {
  struct obj_irc *ircobj = (struct obj_irc *)obj->data.opaque;
  struct ctx *ctxptr;
  struct ll_text *nextmsg, *curmsg = nullptr;

  if (ircobj->session) {
    if (irc_is_connected(ircobj->session)) {
      ctxptr = (struct ctx *)irc_get_ctx(ircobj->session);
      curmsg = ctxptr->messages;
      irc_disconnect(ircobj->session);
    }
    pthread_join(*(ircobj->thread), nullptr);
    irc_destroy_session(ircobj->session);
  }
  free(ircobj->thread);
  free(obj->data.opaque);
  while (curmsg) {
    nextmsg = curmsg->next;
    free(curmsg->text);
    free(curmsg);
    curmsg = nextmsg;
  }
}
