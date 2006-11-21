#!/bin/sh
# $Id$

# autogen.sh
# optional $1 = full path to svn working copy or "clean"

AUTOCONF=${AUTOCONF:-autoconf}
AUTOMAKE=${AUTOMAKE:-automake}
ACLOCAL=${ACLOCAL:-aclocal}
AUTOHEADER=${AUTOHEADER:-autoheader}
LIBTOOLIZE=${LIBTOOLIZE:-libtoolize}

if [ "$1" == "clean" ]; then
    /bin/rm -f configure.ac
    exit 0
fi

# substitute svn revision
revision=`LC_ALL=C svn info $1 | awk '/^Revision: / {printf "%05d\n", $2}'`
sed -e "s/@REVISION@/${revision}/g" \
    < "configure.ac.in" > "configure.ac"

echo Running $ACLOCAL ... && $ACLOCAL
echo Running $LIBTOOLIZE --force ... && $LIBTOOLIZE --force
echo Running $AUTOHEADER ... && $AUTOHEADER
echo Running $AUTOMAKE -a ... && $AUTOMAKE -a
echo Running $AUTOCONF ... && $AUTOCONF
