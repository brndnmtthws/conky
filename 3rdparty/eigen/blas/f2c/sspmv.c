/* sspmv.f -- translated by f2c (version 20100827).
   You must link the resulting object file with libf2c:
        on Microsoft Windows system, link with libf2c.lib;
        on Linux or Unix systems, link with .../path/to/libf2c.a -lm
        or, if you install libf2c.a in a standard place, with -lf2c -lm
        -- in that order, at the end of the command line, as in
                cc *.o -lf2c -lm
        Source for libf2c is in /netlib/f2c/libf2c.zip, e.g.,

                http://www.netlib.org/f2c/libf2c.zip
*/

#include "datatypes.h"

/* Subroutine */ void sspmv_(char *uplo, integer *n, real *alpha, real *ap, real *x, integer *incx, real *beta, real *y,
                             integer *incy) {
  /* System generated locals */
  integer i__1, i__2;

  /* Local variables */
  integer i__, j, k, kk, ix, iy, jx, jy, kx, ky, info;
  real temp1, temp2;
  extern logical lsame_(char *, char *);
  extern /* Subroutine */ void xerbla_(const char *, integer *);

  /*     .. Scalar Arguments .. */
  /*     .. */
  /*     .. Array Arguments .. */
  /*     .. */

  /*  Purpose */
  /*  ======= */

  /*  SSPMV  performs the matrix-vector operation */

  /*     y := alpha*A*x + beta*y, */

  /*  where alpha and beta are scalars, x and y are n element vectors and */
  /*  A is an n by n symmetric matrix, supplied in packed form. */

  /*  Arguments */
  /*  ========== */

  /*  UPLO   - CHARACTER*1. */
  /*           On entry, UPLO specifies whether the upper or lower */
  /*           triangular part of the matrix A is supplied in the packed */
  /*           array AP as follows: */

  /*              UPLO = 'U' or 'u'   The upper triangular part of A is */
  /*                                  supplied in AP. */

  /*              UPLO = 'L' or 'l'   The lower triangular part of A is */
  /*                                  supplied in AP. */

  /*           Unchanged on exit. */

  /*  N      - INTEGER. */
  /*           On entry, N specifies the order of the matrix A. */
  /*           N must be at least zero. */
  /*           Unchanged on exit. */

  /*  ALPHA  - REAL            . */
  /*           On entry, ALPHA specifies the scalar alpha. */
  /*           Unchanged on exit. */

  /*  AP     - REAL             array of DIMENSION at least */
  /*           ( ( n*( n + 1 ) )/2 ). */
  /*           Before entry with UPLO = 'U' or 'u', the array AP must */
  /*           contain the upper triangular part of the symmetric matrix */
  /*           packed sequentially, column by column, so that AP( 1 ) */
  /*           contains a( 1, 1 ), AP( 2 ) and AP( 3 ) contain a( 1, 2 ) */
  /*           and a( 2, 2 ) respectively, and so on. */
  /*           Before entry with UPLO = 'L' or 'l', the array AP must */
  /*           contain the lower triangular part of the symmetric matrix */
  /*           packed sequentially, column by column, so that AP( 1 ) */
  /*           contains a( 1, 1 ), AP( 2 ) and AP( 3 ) contain a( 2, 1 ) */
  /*           and a( 3, 1 ) respectively, and so on. */
  /*           Unchanged on exit. */

  /*  X      - REAL             array of dimension at least */
  /*           ( 1 + ( n - 1 )*abs( INCX ) ). */
  /*           Before entry, the incremented array X must contain the n */
  /*           element vector x. */
  /*           Unchanged on exit. */

  /*  INCX   - INTEGER. */
  /*           On entry, INCX specifies the increment for the elements of */
  /*           X. INCX must not be zero. */
  /*           Unchanged on exit. */

  /*  BETA   - REAL            . */
  /*           On entry, BETA specifies the scalar beta. When BETA is */
  /*           supplied as zero then Y need not be set on input. */
  /*           Unchanged on exit. */

  /*  Y      - REAL             array of dimension at least */
  /*           ( 1 + ( n - 1 )*abs( INCY ) ). */
  /*           Before entry, the incremented array Y must contain the n */
  /*           element vector y. On exit, Y is overwritten by the updated */
  /*           vector y. */

  /*  INCY   - INTEGER. */
  /*           On entry, INCY specifies the increment for the elements of */
  /*           Y. INCY must not be zero. */
  /*           Unchanged on exit. */

  /*  Further Details */
  /*  =============== */

  /*  Level 2 Blas routine. */

  /*  -- Written on 22-October-1986. */
  /*     Jack Dongarra, Argonne National Lab. */
  /*     Jeremy Du Croz, Nag Central Office. */
  /*     Sven Hammarling, Nag Central Office. */
  /*     Richard Hanson, Sandia National Labs. */

  /*  ===================================================================== */

  /*     .. Parameters .. */
  /*     .. */
  /*     .. Local Scalars .. */
  /*     .. */
  /*     .. External Functions .. */
  /*     .. */
  /*     .. External Subroutines .. */
  /*     .. */

  /*     Test the input parameters. */

  /* Parameter adjustments */
  --y;
  --x;
  --ap;

  /* Function Body */
  info = 0;
  if (!lsame_(uplo, "U") && !lsame_(uplo, "L")) {
    info = 1;
  } else if (*n < 0) {
    info = 2;
  } else if (*incx == 0) {
    info = 6;
  } else if (*incy == 0) {
    info = 9;
  }
  if (info != 0) {
    xerbla_("SSPMV ", &info);
    return;
  }

  /*     Quick return if possible. */

  if (*n == 0 || (*alpha == 0.f && *beta == 1.f)) {
    return;
  }

  /*     Set up the start points in  X  and  Y. */

  if (*incx > 0) {
    kx = 1;
  } else {
    kx = 1 - (*n - 1) * *incx;
  }
  if (*incy > 0) {
    ky = 1;
  } else {
    ky = 1 - (*n - 1) * *incy;
  }

  /*     Start the operations. In this version the elements of the array AP */
  /*     are accessed sequentially with one pass through AP. */

  /*     First form  y := beta*y. */

  if (*beta != 1.f) {
    if (*incy == 1) {
      if (*beta == 0.f) {
        i__1 = *n;
        for (i__ = 1; i__ <= i__1; ++i__) {
          y[i__] = 0.f;
          /* L10: */
        }
      } else {
        i__1 = *n;
        for (i__ = 1; i__ <= i__1; ++i__) {
          y[i__] = *beta * y[i__];
          /* L20: */
        }
      }
    } else {
      iy = ky;
      if (*beta == 0.f) {
        i__1 = *n;
        for (i__ = 1; i__ <= i__1; ++i__) {
          y[iy] = 0.f;
          iy += *incy;
          /* L30: */
        }
      } else {
        i__1 = *n;
        for (i__ = 1; i__ <= i__1; ++i__) {
          y[iy] = *beta * y[iy];
          iy += *incy;
          /* L40: */
        }
      }
    }
  }
  if (*alpha == 0.f) {
    return;
  }
  kk = 1;
  if (lsame_(uplo, "U")) {
    /*        Form  y  when AP contains the upper triangle. */

    if (*incx == 1 && *incy == 1) {
      i__1 = *n;
      for (j = 1; j <= i__1; ++j) {
        temp1 = *alpha * x[j];
        temp2 = 0.f;
        k = kk;
        i__2 = j - 1;
        for (i__ = 1; i__ <= i__2; ++i__) {
          y[i__] += temp1 * ap[k];
          temp2 += ap[k] * x[i__];
          ++k;
          /* L50: */
        }
        y[j] = y[j] + temp1 * ap[kk + j - 1] + *alpha * temp2;
        kk += j;
        /* L60: */
      }
    } else {
      jx = kx;
      jy = ky;
      i__1 = *n;
      for (j = 1; j <= i__1; ++j) {
        temp1 = *alpha * x[jx];
        temp2 = 0.f;
        ix = kx;
        iy = ky;
        i__2 = kk + j - 2;
        for (k = kk; k <= i__2; ++k) {
          y[iy] += temp1 * ap[k];
          temp2 += ap[k] * x[ix];
          ix += *incx;
          iy += *incy;
          /* L70: */
        }
        y[jy] = y[jy] + temp1 * ap[kk + j - 1] + *alpha * temp2;
        jx += *incx;
        jy += *incy;
        kk += j;
        /* L80: */
      }
    }
  } else {
    /*        Form  y  when AP contains the lower triangle. */

    if (*incx == 1 && *incy == 1) {
      i__1 = *n;
      for (j = 1; j <= i__1; ++j) {
        temp1 = *alpha * x[j];
        temp2 = 0.f;
        y[j] += temp1 * ap[kk];
        k = kk + 1;
        i__2 = *n;
        for (i__ = j + 1; i__ <= i__2; ++i__) {
          y[i__] += temp1 * ap[k];
          temp2 += ap[k] * x[i__];
          ++k;
          /* L90: */
        }
        y[j] += *alpha * temp2;
        kk += *n - j + 1;
        /* L100: */
      }
    } else {
      jx = kx;
      jy = ky;
      i__1 = *n;
      for (j = 1; j <= i__1; ++j) {
        temp1 = *alpha * x[jx];
        temp2 = 0.f;
        y[jy] += temp1 * ap[kk];
        ix = jx;
        iy = jy;
        i__2 = kk + *n - j;
        for (k = kk + 1; k <= i__2; ++k) {
          ix += *incx;
          iy += *incy;
          y[iy] += temp1 * ap[k];
          temp2 += ap[k] * x[ix];
          /* L110: */
        }
        y[jy] += *alpha * temp2;
        jx += *incx;
        jy += *incy;
        kk += *n - j + 1;
        /* L120: */
      }
    }
  }

  /*     End of SSPMV . */

} /* sspmv_ */
