/*
 *
 * audacious.h:  conky support for audacious music player
 *
 * Copyright (C) 2005-2007 Philip Kovacs pkovacs@users.sourceforge.net
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301
 * USA.
 *
 */

#ifndef AUDACIOUS_H
#define AUDACIOUS_H

void print_audacious_status(struct text_object *, char *, unsigned int);
void print_audacious_title(struct text_object *, char *, unsigned int);
void print_audacious_length(struct text_object *, char *, unsigned int);
void print_audacious_length_seconds(struct text_object *, char *, unsigned int);
void print_audacious_position(struct text_object *, char *, unsigned int);
void print_audacious_position_seconds(struct text_object *, char *, unsigned int);
void print_audacious_bitrate(struct text_object *, char *, unsigned int);
void print_audacious_frequency(struct text_object *, char *, unsigned int);
void print_audacious_channels(struct text_object *, char *, unsigned int);
void print_audacious_filename(struct text_object *, char *, unsigned int);
void print_audacious_playlist_length(struct text_object *, char *, unsigned int);
void print_audacious_playlist_position(struct text_object *, char *, unsigned int);
void print_audacious_main_volume(struct text_object *, char *, unsigned int);
double audacious_barval(struct text_object *);

#endif /* AUDACIOUS_H */
