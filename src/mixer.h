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
 * Copyright (c) 2007 Toni Spets
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

#ifndef MIXER_H_
#define MIXER_H_

void parse_mixer_arg(struct text_object *, const char *);
uint8_t mixer_percentage(struct text_object *obj);
uint8_t mixerl_percentage(struct text_object *obj);
uint8_t mixerr_percentage(struct text_object *obj);
int check_mixer_muted(struct text_object *);

void scan_mixer_bar(struct text_object *, const char *);
double mixer_barval(struct text_object *);
double mixerl_barval(struct text_object *);
double mixerr_barval(struct text_object *);

#endif /*MIXER_H_*/
