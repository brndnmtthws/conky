#!/bin/sh

if [ -x aclocal-1.9 ]
then
	aclocal-1.9
else
	aclocal
fi

if [ -x autoheader-2.59 ]
then
	autoheader-2.59
else
	autoheader
fi

if [ -x automake-1.9 ]
then
	automake-1.9 -a
else
	automake
fi

if [ -x autoconf-2.59 ]
then
	autoconf-2.59
else
	autoconf
fi
