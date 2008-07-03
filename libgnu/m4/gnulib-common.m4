# gnulib-common.m4 serial 2
dnl Copyright (C) 2007 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

# gl_MODULE_INDICATOR([modulename])
# defines a C macro indicating the presence of the given module.
AC_DEFUN([gl_MODULE_INDICATOR],
[
  AC_DEFINE([GNULIB_]translit([$1],[abcdefghijklmnopqrstuvwxyz./-],[ABCDEFGHIJKLMNOPQRSTUVWXYZ___]), [1],
    [Define to 1 when using the gnulib module ]$1[.])
])

# AC_PROG_MKDIR_P
# is a backport of autoconf-2.60's AC_PROG_MKDIR_P.
# Remove this macro when we can assume autoconf >= 2.60.
m4_ifdef([AC_PROG_MKDIR_P], [], [
  AC_DEFUN([AC_PROG_MKDIR_P],
    [AC_REQUIRE([AM_PROG_MKDIR_P])dnl defined by automake
     MKDIR_P='$(mkdir_p)'
     AC_SUBST([MKDIR_P])])])
