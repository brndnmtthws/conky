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

#include "timed_thread.h"

struct moc_s {
	char *state;
	char *file;
	char *title;
	char *artist;
	char *song;
	char *album;
	char *totaltime;
	char *timeleft;
	char *curtime;
	char *bitrate;
	char *rate;
};
extern struct moc_s moc;

int update_moc(void);
void free_moc(void);

#endif /* MOC_H_ */

