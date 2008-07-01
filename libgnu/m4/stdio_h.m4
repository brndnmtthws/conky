# stdio_h.m4 serial 7
dnl Copyright (C) 2007 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_STDIO_H],
[
  AC_REQUIRE([gl_STDIO_H_DEFAULTS])
  gl_CHECK_NEXT_HEADERS([stdio.h])
])

AC_DEFUN([gl_STDIO_MODULE_INDICATOR],
[
  dnl Use AC_REQUIRE here, so that the default settings are expanded once only.
  AC_REQUIRE([gl_STDIO_H_DEFAULTS])
  GNULIB_[]m4_translit([$1],[abcdefghijklmnopqrstuvwxyz./-],[ABCDEFGHIJKLMNOPQRSTUVWXYZ___])=1
])

AC_DEFUN([gl_STDIO_H_DEFAULTS],
[
  GNULIB_FPRINTF_POSIX=0;  AC_SUBST([GNULIB_FPRINTF_POSIX])
  GNULIB_PRINTF_POSIX=0;   AC_SUBST([GNULIB_PRINTF_POSIX])
  GNULIB_SNPRINTF=0;       AC_SUBST([GNULIB_SNPRINTF])
  GNULIB_SPRINTF_POSIX=0;  AC_SUBST([GNULIB_SPRINTF_POSIX])
  GNULIB_VFPRINTF_POSIX=0; AC_SUBST([GNULIB_VFPRINTF_POSIX])
  GNULIB_VPRINTF_POSIX=0;  AC_SUBST([GNULIB_VPRINTF_POSIX])
  GNULIB_VSNPRINTF=0;      AC_SUBST([GNULIB_VSNPRINTF])
  GNULIB_VSPRINTF_POSIX=0; AC_SUBST([GNULIB_VSPRINTF_POSIX])
  GNULIB_VASPRINTF=0;      AC_SUBST([GNULIB_VASPRINTF])
  GNULIB_FSEEK=0;          AC_SUBST([GNULIB_FSEEK])
  GNULIB_FSEEKO=0;         AC_SUBST([GNULIB_FSEEKO])
  GNULIB_FTELL=0;          AC_SUBST([GNULIB_FTELL])
  GNULIB_FTELLO=0;         AC_SUBST([GNULIB_FTELLO])
  GNULIB_FFLUSH=0;         AC_SUBST([GNULIB_FFLUSH])
  dnl Assume proper GNU behavior unless another module says otherwise.
  REPLACE_FPRINTF=0;       AC_SUBST([REPLACE_FPRINTF])
  REPLACE_VFPRINTF=0;      AC_SUBST([REPLACE_VFPRINTF])
  REPLACE_PRINTF=0;        AC_SUBST([REPLACE_PRINTF])
  REPLACE_VPRINTF=0;       AC_SUBST([REPLACE_VPRINTF])
  REPLACE_SNPRINTF=0;      AC_SUBST([REPLACE_SNPRINTF])
  HAVE_DECL_SNPRINTF=1;    AC_SUBST([HAVE_DECL_SNPRINTF])
  REPLACE_VSNPRINTF=0;     AC_SUBST([REPLACE_VSNPRINTF])
  HAVE_DECL_VSNPRINTF=1;   AC_SUBST([HAVE_DECL_VSNPRINTF])
  REPLACE_SPRINTF=0;       AC_SUBST([REPLACE_SPRINTF])
  REPLACE_VSPRINTF=0;      AC_SUBST([REPLACE_VSPRINTF])
  HAVE_VASPRINTF=1;        AC_SUBST([HAVE_VASPRINTF])
  REPLACE_VASPRINTF=0;     AC_SUBST([REPLACE_VASPRINTF])
  HAVE_FSEEKO=1;           AC_SUBST([HAVE_FSEEKO])
  REPLACE_FSEEKO=0;        AC_SUBST([REPLACE_FSEEKO])
  REPLACE_FSEEK=0;         AC_SUBST([REPLACE_FSEEK])
  HAVE_FTELLO=1;           AC_SUBST([HAVE_FTELLO])
  REPLACE_FTELLO=0;        AC_SUBST([REPLACE_FTELLO])
  REPLACE_FTELL=0;         AC_SUBST([REPLACE_FTELL])
  REPLACE_FFLUSH=0;        AC_SUBST([REPLACE_FFLUSH])
])

dnl Code shared by fseeko and ftello.  Determine if large files are supported,
dnl but stdin does not start as a large file by default.
AC_DEFUN([gl_STDIN_LARGE_OFFSET],
  [
    AC_CACHE_CHECK([whether stdin defaults to large file offsets],
      [gl_cv_var_stdin_large_offset],
      [AC_LINK_IFELSE([AC_LANG_PROGRAM([#include <stdio.h>],
[#if defined __SL64 && defined __SCLE /* cygwin */
  /* Cygwin 1.5.24 and earlier fail to put stdin in 64-bit mode, making
     fseeko/ftello needlessly fail.  This bug was fixed at the same time
     that cygwin started exporting asnprintf (cygwin 1.7.0), so we use
     that as a link-time test for cross-compiles rather than building
     a runtime test.  */
  size_t s;
  if (asnprintf (NULL, &s, ""))
    return 0;
#endif])],
	[gl_cv_var_stdin_large_offset=yes],
	[gl_cv_var_stdin_large_offset=no])])
])
