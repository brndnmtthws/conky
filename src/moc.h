/* -*- mode: c; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: t -*-
 *
 * MOC Conky integration
 *
 * Please see COPYING for details
 *
 * Copyright (c) 2008, Henri Häkkinen
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

#ifndef MOC_H_
#define MOC_H_

void update_moc(void);
void free_moc(struct text_object *);

void print_moc_state(struct text_object *, char *, int);
void print_moc_file(struct text_object *, char *, int);
void print_moc_title(struct text_object *, char *, int);
void print_moc_artist(struct text_object *, char *, int);
void print_moc_song(struct text_object *, char *, int);
void print_moc_album(struct text_object *, char *, int);
void print_moc_totaltime(struct text_object *, char *, int);
void print_moc_timeleft(struct text_object *, char *, int);
void print_moc_curtime(struct text_object *, char *, int);
void print_moc_bitrate(struct text_object *, char *, int);
void print_moc_rate(struct text_object *, char *, int);

#endif /* MOC_H_ */

