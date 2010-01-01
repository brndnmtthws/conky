#!/bin/sh
#
# Conky, a system monitor, based on torsmo
#
# Any original torsmo code is licensed under the BSD license
#
# All code written since the fork of torsmo is licensed under the GPL
#
# Please see COPYING for details
#
# Copyright (c) 2004, Hannu Saransaari and Lauri Hakkarainen
# Copyright (c) 2005-2009 Brenden Matthews, Philip Kovacs, et. al. (see AUTHORS)
# All rights reserved.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
# optional $1 = optional directory containing build tree or git working copy

# Something exceptionally irritating to get people to move away from autotools.
echo
echo "\033[41m\033[34m The Conky autotools build system is now deprecated in favour of CMake.  Please see README.cmake for usage instructions."
echo

AUTOCONF=${AUTOCONF:-autoconf}
AUTOMAKE=${AUTOMAKE:-automake}
ACLOCAL=${ACLOCAL:-aclocal}
AUTOHEADER=${AUTOHEADER:-autoheader}
LIBTOOLIZE=${LIBTOOLIZE:-libtoolize}

# identify a git revision similar to svn based on number of commits, if a git
# working copy.  the last svn commit was rev 1274, so we'll pick up from there
if test "$1" != "" && test -d "$1/.git"; then
    revision=`git log --since=2008-12-06 --pretty=oneline | wc -l | awk '{print $1 + 1274}'`;
elif test -d ".git"; then
    revision=`git log --since=2008-12-06 --pretty=oneline | wc -l | awk '{print $1 + 1274}'`;
else
    revision="NONE"; fi

# generate configure.ac with substituted git revision
sed -e "s/@REVISION@/${revision}/g" < "configure.ac.in" > "configure.ac"

touch README # in case it doesn't exist
echo Running $ACLOCAL -I m4 ... && $ACLOCAL -I m4
echo Running $LIBTOOLIZE --force --copy ... && $LIBTOOLIZE --force --copy
echo Running $AUTOHEADER ... && $AUTOHEADER
echo Running $AUTOMAKE --add-missing --copy --gnu ... && $AUTOMAKE --add-missing --copy --gnu
echo Running $AUTOCONF ... && $AUTOCONF
