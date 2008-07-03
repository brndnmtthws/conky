# strdup.m4 serial 9

dnl Copyright (C) 2002, 2003, 2004, 2005, 2006, 2007 Free Software
dnl Foundation, Inc.

dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FUNC_STRDUP],
[
  AC_REQUIRE([gl_HEADER_STRING_H_DEFAULTS])
  AC_REPLACE_FUNCS(strdup)
  AC_CHECK_DECLS_ONCE(strdup)
  if test $ac_cv_have_decl_strdup = no; then
    HAVE_DECL_STRDUP=0
  fi
  gl_PREREQ_STRDUP
])

# Prerequisites of lib/strdup.c.
AC_DEFUN([gl_PREREQ_STRDUP], [:])
