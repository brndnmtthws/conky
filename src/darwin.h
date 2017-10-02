/*
 *  This file is part of conky.
 *
 *  conky is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  conky is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with conky.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

//  darwin.h
//  Nickolas Pylarinos

#ifndef DARWIN_H
#define DARWIN_H

#include <sys/param.h>
#include <sys/mount.h>
#include <strings.h>
#include <stdio.h>

int get_entropy_avail(unsigned int *);
int get_entropy_poolsize(unsigned int *);

int update_running_threads(void);

#endif /*DARWIN_H*/
