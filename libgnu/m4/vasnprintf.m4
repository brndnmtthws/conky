# vasnprintf.m4 serial 20
dnl Copyright (C) 2002-2004, 2006-2007 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FUNC_VASNPRINTF],
[
  AC_REQUIRE([gl_EOVERFLOW])
  AC_CHECK_FUNCS_ONCE([vasnprintf])
  if test $ac_cv_func_vasnprintf = no; then
    gl_REPLACE_VASNPRINTF
  fi
])

AC_DEFUN([gl_REPLACE_VASNPRINTF],
[
  AC_CHECK_FUNCS_ONCE([vasnprintf])
  AC_LIBOBJ([vasnprintf])
  AC_LIBOBJ([printf-args])
  AC_LIBOBJ([printf-parse])
  AC_LIBOBJ([asnprintf])
  if test $ac_cv_func_vasnprintf = yes; then
    AC_DEFINE([REPLACE_VASNPRINTF], 1,
      [Define if vasnprintf exists but is overridden by gnulib.])
  fi
  gl_PREREQ_PRINTF_ARGS
  gl_PREREQ_PRINTF_PARSE
  gl_PREREQ_VASNPRINTF
  gl_PREREQ_ASNPRINTF
])

# Prequisites of lib/printf-args.h, lib/printf-args.c.
AC_DEFUN([gl_PREREQ_PRINTF_ARGS],
[
  AC_REQUIRE([AC_TYPE_LONG_LONG_INT])
  AC_REQUIRE([gt_TYPE_WCHAR_T])
  AC_REQUIRE([gt_TYPE_WINT_T])
])

# Prequisites of lib/printf-parse.h, lib/printf-parse.c.
AC_DEFUN([gl_PREREQ_PRINTF_PARSE],
[
  AC_REQUIRE([AC_TYPE_LONG_LONG_INT])
  AC_REQUIRE([gt_TYPE_WCHAR_T])
  AC_REQUIRE([gt_TYPE_WINT_T])
  AC_REQUIRE([AC_TYPE_SIZE_T])
  AC_CHECK_TYPES(ptrdiff_t)
  AC_REQUIRE([gt_AC_TYPE_INTMAX_T])
])

# Prerequisites of lib/vasnprintf.c.
AC_DEFUN([gl_PREREQ_VASNPRINTF],
[
  AC_REQUIRE([AC_FUNC_ALLOCA])
  AC_REQUIRE([AC_TYPE_LONG_LONG_INT])
  AC_REQUIRE([gt_TYPE_WCHAR_T])
  AC_REQUIRE([gt_TYPE_WINT_T])
  AC_CHECK_FUNCS(snprintf wcslen)
  dnl Use the _snprintf function only if it is declared (because on NetBSD it
  dnl is defined as a weak alias of snprintf; we prefer to use the latter).
  AC_CHECK_DECLS([_snprintf], , , [#include <stdio.h>])
])

# Extra prerequisites of lib/vasnprintf.c for supporting 'long double'
# arguments.
AC_DEFUN([gl_PREREQ_VASNPRINTF_LONG_DOUBLE],
[
  AC_REQUIRE([gl_PRINTF_LONG_DOUBLE])
  case "$gl_cv_func_printf_long_double" in
    *yes)
      ;;
    *)
      AC_DEFINE([NEED_PRINTF_LONG_DOUBLE], 1,
        [Define if the vasnprintf implementation needs special code for
         'long double' arguments.])
      ;;
  esac
])

# Extra prerequisites of lib/vasnprintf.c for supporting infinite 'double'
# arguments.
AC_DEFUN([gl_PREREQ_VASNPRINTF_INFINITE_DOUBLE],
[
  AC_REQUIRE([gl_PRINTF_INFINITE])
  case "$gl_cv_func_printf_infinite" in
    *yes)
      ;;
    *)
      AC_DEFINE([NEED_PRINTF_INFINITE_DOUBLE], 1,
        [Define if the vasnprintf implementation needs special code for
         infinite 'double' arguments.])
      ;;
  esac
])

# Extra prerequisites of lib/vasnprintf.c for supporting infinite 'long double'
# arguments.
AC_DEFUN([gl_PREREQ_VASNPRINTF_INFINITE_LONG_DOUBLE],
[
  AC_REQUIRE([gl_PRINTF_INFINITE_LONG_DOUBLE])
  dnl There is no need to set NEED_PRINTF_INFINITE_LONG_DOUBLE if
  dnl NEED_PRINTF_LONG_DOUBLE is already set.
  AC_REQUIRE([gl_PREREQ_VASNPRINTF_LONG_DOUBLE])
  case "$gl_cv_func_printf_long_double" in
    *yes)
      case "$gl_cv_func_printf_infinite_long_double" in
        *yes)
          ;;
        *)
          AC_DEFINE([NEED_PRINTF_INFINITE_LONG_DOUBLE], 1,
            [Define if the vasnprintf implementation needs special code for
             infinite 'long double' arguments.])
          ;;
      esac
      ;;
  esac
])

# Extra prerequisites of lib/vasnprintf.c for supporting the 'a' directive.
AC_DEFUN([gl_PREREQ_VASNPRINTF_DIRECTIVE_A],
[
  AC_REQUIRE([gl_PRINTF_DIRECTIVE_A])
  case "$gl_cv_func_printf_directive_a" in
    *yes)
      ;;
    *)
      AC_DEFINE([NEED_PRINTF_DIRECTIVE_A], 1,
        [Define if the vasnprintf implementation needs special code for
         the 'a' and 'A' directives.])
      AC_CHECK_FUNCS([nl_langinfo])
      ;;
  esac
])

# Extra prerequisites of lib/vasnprintf.c for supporting the 'F' directive.
AC_DEFUN([gl_PREREQ_VASNPRINTF_DIRECTIVE_F],
[
  AC_REQUIRE([gl_PRINTF_DIRECTIVE_F])
  case "$gl_cv_func_printf_directive_f" in
    *yes)
      ;;
    *)
      AC_DEFINE([NEED_PRINTF_DIRECTIVE_F], 1,
        [Define if the vasnprintf implementation needs special code for
         the 'F' directive.])
      ;;
  esac
])

# Extra prerequisites of lib/vasnprintf.c for supporting the ' flag.
AC_DEFUN([gl_PREREQ_VASNPRINTF_FLAG_GROUPING],
[
  AC_REQUIRE([gl_PRINTF_FLAG_GROUPING])
  case "$gl_cv_func_printf_flag_grouping" in
    *yes)
      ;;
    *)
      AC_DEFINE([NEED_PRINTF_FLAG_GROUPING], 1,
        [Define if the vasnprintf implementation needs special code for the
         ' flag.])
      ;;
  esac
])

# Extra prerequisites of lib/vasnprintf.c for supporting the 0 flag.
AC_DEFUN([gl_PREREQ_VASNPRINTF_FLAG_ZERO],
[
  AC_REQUIRE([gl_PRINTF_FLAG_ZERO])
  case "$gl_cv_func_printf_flag_zero" in
    *yes)
      ;;
    *)
      AC_DEFINE([NEED_PRINTF_FLAG_ZERO], 1,
        [Define if the vasnprintf implementation needs special code for the
         0 flag.])
      ;;
  esac
])

# Prerequisites of lib/vasnprintf.c including all extras for POSIX compliance.
AC_DEFUN([gl_PREREQ_VASNPRINTF_WITH_EXTRAS],
[
  AC_REQUIRE([gl_PREREQ_VASNPRINTF])
  gl_PREREQ_VASNPRINTF_LONG_DOUBLE
  gl_PREREQ_VASNPRINTF_INFINITE_DOUBLE
  gl_PREREQ_VASNPRINTF_INFINITE_LONG_DOUBLE
  gl_PREREQ_VASNPRINTF_DIRECTIVE_A
  gl_PREREQ_VASNPRINTF_DIRECTIVE_F
  gl_PREREQ_VASNPRINTF_FLAG_GROUPING
  gl_PREREQ_VASNPRINTF_FLAG_ZERO
])

# Prerequisites of lib/asnprintf.c.
AC_DEFUN([gl_PREREQ_ASNPRINTF],
[
])
