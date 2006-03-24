#!/bin/sh
# $Id$

if [ `uname -s` = FreeBSD ]; then
	aclocal19
	libtoolize15 --force
	autoheader259
	automake19 -a
	autoconf259
else
	aclocal-1.9
	libtoolize --force
	autoheader-2.59
	automake-1.9 -a
	autoconf-2.59
fi
