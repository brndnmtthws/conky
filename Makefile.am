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

SUBDIRS = 					\
	src 					\
	doc					\
	data

AUTOMAKE_OPTIONS = 				\
	1.8					\
	dist-bzip2

EXTRA_DIST = 					\
	autogen.sh				\
	configure.ac.in				\
	config.rpath				\
	extras/nano/README			\
	extras/nano/conky.nanorc		\
	extras/vim/README			\
	extras/vim/ftdetect/conkyrc.vim		\
	extras/vim/syntax/conkyrc.vim		\
	text2c.sh

ACLOCAL_AMFLAGS = -I m4

distclean-local:
	/bin/rm -rf *.cache *~

# vi:set ts=8 sw=8 noet ai nocindent syntax=automake:
