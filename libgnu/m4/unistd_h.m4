# unistd_h.m4 serial 9
dnl Copyright (C) 2006-2007 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

dnl Written by Simon Josefsson, Bruno Haible.

AC_DEFUN([gl_UNISTD_H],
[
  dnl Use AC_REQUIRE here, so that the default behavior below is expanded
  dnl once only, before all statements that occur in other macros.
  AC_REQUIRE([gl_UNISTD_H_DEFAULTS])

  gl_CHECK_NEXT_HEADERS([unistd.h])

  AC_CHECK_HEADERS_ONCE([unistd.h])
  if test $ac_cv_header_unistd_h = yes; then
    HAVE_UNISTD_H=1
  else
    HAVE_UNISTD_H=0
  fi
  AC_SUBST([HAVE_UNISTD_H])
])

AC_DEFUN([gl_UNISTD_MODULE_INDICATOR],
[
  dnl Use AC_REQUIRE here, so that the default settings are expanded once only.
  AC_REQUIRE([gl_UNISTD_H_DEFAULTS])
  GNULIB_[]m4_translit([$1],[abcdefghijklmnopqrstuvwxyz./-],[ABCDEFGHIJKLMNOPQRSTUVWXYZ___])=1
])

AC_DEFUN([gl_UNISTD_H_DEFAULTS],
[
  GNULIB_CHOWN=0;         AC_SUBST([GNULIB_CHOWN])
  GNULIB_DUP2=0;          AC_SUBST([GNULIB_DUP2])
  GNULIB_FCHDIR=0;        AC_SUBST([GNULIB_FCHDIR])
  GNULIB_FTRUNCATE=0;     AC_SUBST([GNULIB_FTRUNCATE])
  GNULIB_GETCWD=0;        AC_SUBST([GNULIB_GETCWD])
  GNULIB_GETLOGIN_R=0;    AC_SUBST([GNULIB_GETLOGIN_R])
  GNULIB_LCHOWN=0;        AC_SUBST([GNULIB_LCHOWN])
  GNULIB_LSEEK=0;         AC_SUBST([GNULIB_LSEEK])
  GNULIB_READLINK=0;      AC_SUBST([GNULIB_READLINK])
  GNULIB_SLEEP=0;         AC_SUBST([GNULIB_SLEEP])
  dnl Assume proper GNU behavior unless another module says otherwise.
  HAVE_DUP2=1;            AC_SUBST([HAVE_DUP2])
  HAVE_FTRUNCATE=1;       AC_SUBST([HAVE_FTRUNCATE])
  HAVE_READLINK=1;        AC_SUBST([HAVE_READLINK])
  HAVE_SLEEP=1;           AC_SUBST([HAVE_SLEEP])
  HAVE_DECL_GETLOGIN_R=1; AC_SUBST([HAVE_DECL_GETLOGIN_R])
  REPLACE_CHOWN=0;        AC_SUBST([REPLACE_CHOWN])
  REPLACE_FCHDIR=0;       AC_SUBST([REPLACE_FCHDIR])
  REPLACE_GETCWD=0;       AC_SUBST([REPLACE_GETCWD])
  REPLACE_LCHOWN=0;       AC_SUBST([REPLACE_LCHOWN])
  REPLACE_LSEEK=0;        AC_SUBST([REPLACE_LSEEK])
])
