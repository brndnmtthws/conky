#!/bin/sh
# $Id$

# autogen.sh
#
# optional $1 = optional directory containing build tree or svn working copy

export AUTOCONF=${AUTOCONF:-autoconf}
export AUTOMAKE=${AUTOMAKE:-automake}
export ACLOCAL=${ACLOCAL:-aclocal}
export AUTOHEADER=${AUTOHEADER:-autoheader}
export LIBTOOLIZE=${LIBTOOLIZE:-libtoolize}

# identify svn revision, if an svn working copy
if test "$1" != "" && test -d "$1/.svn"; then
    revision=`LC_ALL=C svn info $1 | awk '/^Revision: / {printf "%05d\n", $2}'`;
elif test -d ".svn"; then
    revision=`LC_ALL=C svn info | awk '/^Revision: / {printf "%05d\n", $2}'`; 
else
    revision="NONE"; fi

# generate configure.ac with substituted svn revision
sed -e "s/@REVISION@/${revision}/g" < "configure.ac.in" > "configure.ac"

echo Running $ACLOCAL -I m4 ... && $ACLOCAL -I m4
echo Running $LIBTOOLIZE --force --copy ... && $LIBTOOLIZE --force --copy
echo Running $AUTOHEADER ... && $AUTOHEADER
echo Running $AUTOMAKE --add-missing --copy --gnu ... && $AUTOMAKE --add-missing --copy --gnu
echo Running $AUTOCONF ... && $AUTOCONF
