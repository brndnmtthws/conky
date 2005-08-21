#!/bin/sh
# $Id$

aclocal-1.9
libtoolize --force
autoheader
automake-1.9 -a
autoconf
