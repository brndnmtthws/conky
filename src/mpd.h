/* -*- mode: c++; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: t -*-
 * vim: ts=4 sw=4 noet ai cindent syntax=cpp
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

#ifndef MPD_H_
#define MPD_H_

/* text object functions */
void print_mpd_elapsed(struct text_object *, char *, int);
void print_mpd_length(struct text_object *, char *, int);
uint8_t mpd_percentage(struct text_object *);
double mpd_barval(struct text_object *);
void print_mpd_smart(struct text_object *, char *, int);
void print_mpd_title(struct text_object *, char *, int);
void print_mpd_artist(struct text_object *, char *, int);
void print_mpd_albumartist(struct text_object *, char *, int);
void print_mpd_album(struct text_object *, char *, int);
void print_mpd_date(struct text_object *, char *, int);
void print_mpd_random(struct text_object *, char *, int);
void print_mpd_repeat(struct text_object *, char *, int);
void print_mpd_track(struct text_object *, char *, int);
void print_mpd_name(struct text_object *, char *, int);
void print_mpd_file(struct text_object *, char *, int);
void print_mpd_vol(struct text_object *, char *, int);
void print_mpd_bitrate(struct text_object *, char *, int);
void print_mpd_status(struct text_object *, char *, int);
int check_mpd_playing(struct text_object *);

#endif /*MPD_H_*/
