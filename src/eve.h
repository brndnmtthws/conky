/* -*- mode: c; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: t -*-
 *
 * Conky, a system monitor, based on torsmo
 *
 * Copyright (c) 2008 Asbjørn Zweidorff Kjær
 * Copyright (c) 2005-2009 Brenden Matthews, Philip Kovacs, et. al.
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

#define _GNU_SOURCE
#define MAXCHARS 4
#define EVE_UPDATE_DELAY 60

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <time.h>

typedef struct {
	char *charid;
	char *skillname;
	char *time;
	char *lastOutput;

	struct tm ends;
	struct tm cache;

	time_t delay;

	int level;
	int skill;
} Character;

struct xmlData {
	char *data;
	size_t size;
};

char *eve(char *, char *, char *);
char *getXmlFromAPI(const char *, const char *, const char *, const char *);
char *getSkillname(const char *, int);
char *formatTime(struct tm *);
int parseTrainingXml(char *, Character *);
int parseSkilltreeXml(char *, char *);
int isCacheValid(struct tm);
int file_exists(const char *);
void writeSkilltree(char *, const char *);
void init_eve(void);
