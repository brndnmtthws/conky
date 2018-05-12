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

#ifndef XMMS2_H_
#define XMMS2_H_

#include <xmmsclient/xmmsclient.h>

struct xmms2_s {
  char *artist;
  char *album;
  char *title;
  char *genre;
  char *comment;
  char *url;
  char *date;
  char *playlist;
  int tracknr;
  int bitrate;
  unsigned int id;
  int duration;
  int elapsed;
  int timesplayed;
  float size;

  float progress;
  int percent;
  char *status;
  int conn_state;
};

int update_xmms2(void);

void print_xmms2_tracknr(struct text_object *, char *, int);
void print_xmms2_elapsed(struct text_object *, char *, int);
void print_xmms2_duration(struct text_object *, char *, int);
double xmms2_barval(struct text_object *);
void print_xmms2_smart(struct text_object *, char *, int);
void print_xmms2_artist(struct text_object *, char *, int);
void print_xmms2_album(struct text_object *, char *, int);
void print_xmms2_title(struct text_object *, char *, int);
void print_xmms2_genre(struct text_object *, char *, int);
void print_xmms2_comment(struct text_object *, char *, int);
void print_xmms2_url(struct text_object *, char *, int);
void print_xmms2_status(struct text_object *, char *, int);
void print_xmms2_date(struct text_object *, char *, int);
void print_xmms2_bitrate(struct text_object *, char *, int);
void print_xmms2_id(struct text_object *, char *, int);
void print_xmms2_size(struct text_object *, char *, int);
void print_xmms2_playlist(struct text_object *, char *, int);
void print_xmms2_timesplayed(struct text_object *, char *, int);
void print_xmms2_percent(struct text_object *, char *, int);
int if_xmms2_connected(struct text_object *);

void free_xmms2(struct text_object *);

#endif /*XMMS2_H_*/
