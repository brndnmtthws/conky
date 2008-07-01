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
# Copyright (c) 2005-2007 Brenden Matthews, Philip Kovacs, et. al. (see AUTHORS)
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
# $Id$
#
# optional $1 = optional directory containing build tree or svn working copy

AUTOCONF=${AUTOCONF:-autoconf}
AUTOMAKE=${AUTOMAKE:-automake}
ACLOCAL=${ACLOCAL:-aclocal}
AUTOHEADER=${AUTOHEADER:-autoheader}
LIBTOOLIZE=${LIBTOOLIZE:-libtoolize}

# identify svn revision, if an svn working copy
if test "$1" != "" && test -d "$1/.svn"; then
    revision=`LC_ALL=C svn info $1 | awk '/^Revision: / {printf "%05d\n", $2}'`;
elif test -d ".svn"; then
    revision=`LC_ALL=C svn info | awk '/^Revision: / {printf "%05d\n", $2}'`;
else
    revision="NONE"; fi

# generate configure.ac with substituted svn revision
sed -e "s/@REVISION@/${revision}/g" < "configure.ac.in" > "configure.ac"

echo Running $ACLOCAL -I m4 -I libgnu/m4 ... && $ACLOCAL -I m4 -I libgnu/m4
echo Running $LIBTOOLIZE --force --copy ... && $LIBTOOLIZE --force --copy
echo Running $AUTOHEADER ... && $AUTOHEADER
echo Running $AUTOMAKE --add-missing --copy --gnu ... && $AUTOMAKE --add-missing --copy --gnu
echo Running $AUTOCONF ... && $AUTOCONF
