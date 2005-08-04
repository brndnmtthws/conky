#!/bin/sh

alocver=1.9
aheadver=2.59
amakever=1.9
aconfver=2.59

if [ -x aclocal-$alocver ]
then
	aclocal-$alocver
else
	aclocal
fi

if [ -x autoheader-$aheadver ]
then
	autoheader-$aheadver
else
	autoheader
fi

if [ -x automake-$amakever ]
then
	automake-$amakever -a
else
	automake
fi

if [ -x autoconf-$aconfver ]
then
	autoconf-$aconfver
else
	autoconf
fi
