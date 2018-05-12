/*
 *
 * Conky, a system monitor, based on torsmo
 *
 * Please see COPYING for details
 *
 * Copyright (c) 2005-2012 Brenden Matthews, Philip Kovacs, et. al.
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

#ifndef BMPX_H_
#define BMPX_H_

void update_bmpx(void);
struct bmpx_s {
  char *title;
  char *artist;
  char *album;
  char *uri;
  int bitrate;
  int track;
};

void print_bmpx_title(struct text_object *, char *, int);
void print_bmpx_artist(struct text_object *, char *, int);
void print_bmpx_album(struct text_object *, char *, int);
void print_bmpx_uri(struct text_object *, char *, int);
void print_bmpx_track(struct text_object *, char *, int);
void print_bmpx_bitrate(struct text_object *, char *, int);

#endif /*BMPX_H_*/
