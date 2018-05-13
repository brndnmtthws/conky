#!/bin/sh
#
# text2c.sh - convert a text file to C code
#
# Copyright (C) 2008  Phil Sutter <phil@nwl.cc>
# 
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the Free
# Software Foundation, either version 3 of the License, or (at your option)
# any later version.
# 
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
# for more details.
# 
# You should have received a copy of the GNU General Public License along
# with this program.  If not, see <http://www.gnu.org/licenses/>.


# Invocation is as follows:
# $1: text file
# $2: output file
# $3: name of variable
#
# The output will be a char **, with each field containing a single line of $1.
# Additionally, a macro with the name print_$3 will be defined, with acts as
# a parameter-less function, printing the text to stdout.

[ $# -eq 3 ] || {
	echo "Usage: `basename $0` <inputfile> <outputfile> <variablename>"
	exit 1
}

outupper="`basename "$2" | tr '[a-z-.]' '[A-Z__]'`"

(
	printf "const char %s[] = \n" "$3"
	sed -e 's/["\]/\\&/g' -e 's/^/    "/' -e 's/$/\\n"/' -e '$s/$/;/' "$1"
) > "$2"
