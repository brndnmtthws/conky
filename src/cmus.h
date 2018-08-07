/*
 *
 * CMUS Conky integration
 *
 * Please see COPYING for details
 *
 * Copyright (c) 2011, Christian Brabandt <cb@256bit.org>
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

#ifndef CMUS_H_
#define CMUS_H_

void print_cmus_state(struct text_object *, char *, unsigned int);
void print_cmus_file(struct text_object *, char *, unsigned int);
void print_cmus_title(struct text_object *, char *, unsigned int);
void print_cmus_artist(struct text_object *, char *, unsigned int);
void print_cmus_album(struct text_object *, char *, unsigned int);
void print_cmus_totaltime(struct text_object *, char *, unsigned int);
void print_cmus_timeleft(struct text_object *, char *, unsigned int);
void print_cmus_curtime(struct text_object *, char *, unsigned int);
void print_cmus_random(struct text_object *, char *, unsigned int);
void print_cmus_repeat(struct text_object *, char *, unsigned int);
void print_cmus_aaa(struct text_object *, char *, unsigned int);
void print_cmus_track(struct text_object *, char *, unsigned int);
void print_cmus_genre(struct text_object *, char *, unsigned int);
void print_cmus_date(struct text_object *, char *, unsigned int);
void print_cmus_bar(struct text_object *, char *, unsigned int);
double cmus_progress(struct text_object *obj);
uint8_t cmus_percent(struct text_object *obj);

#endif /* CMUS_H_ */
