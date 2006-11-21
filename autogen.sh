#!/bin/sh
# $Id$

# autogen.sh
# optional $1 = full path to svn local working repository

AUTOCONF=${AUTOCONF:-autoconf}
AUTOMAKE=${AUTOMAKE:-automake}
ACLOCAL=${ACLOCAL:-aclocal}
AUTOHEADER=${AUTOHEADER:-autoheader}
LIBTOOLIZE=${LIBTOOLIZE:-libtoolize}
WORKINGREPO=${1:-.}

# substitute svn revision
revision=`LC_ALL=C svn info ${WORKINGREPO} | awk '/^Revision: / {printf "%05d\n", $2}'`
sed -e "s/@REVISION@/${revision}/g" \
    < "configure.ac.in" > "configure.ac"

echo Running $ACLOCAL ... && $ACLOCAL
echo Running $LIBTOOLIZE --force ... && $LIBTOOLIZE --force
echo Running $AUTOHEADER ... && $AUTOHEADER
echo Running $AUTOMAKE -a ... && $AUTOMAKE -a
echo Running $AUTOCONF ... && $AUTOCONF
