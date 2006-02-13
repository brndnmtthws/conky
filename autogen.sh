#!/bin/sh
# $Id$

aclocal-1.9
libtoolize --force
autoheader-2.59
automake-1.9 -a
autoconf-2.59
