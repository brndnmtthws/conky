/* chbmv.f -- translated by f2c (version 20100827).
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

static inline void r_cnjg(complex *r, complex *z) {
  r->r = z->r;
  r->i = -(z->i);
}

/* Subroutine */ void chbmv_(char *uplo, integer *n, integer *k, complex *alpha, complex *a, integer *lda, complex *x,
                             integer *incx, complex *beta, complex *y, integer *incy) {
  /* System generated locals */
  integer a_dim1, a_offset, i__1, i__2, i__3, i__4, i__5;
  real r__1;
  complex q__1, q__2, q__3, q__4;

  /* Local variables */
  integer i__, j, l, ix, iy, jx, jy, kx, ky, info;
  complex temp1, temp2;
  extern logical lsame_(char *, char *);
  integer kplus1;
  extern /* Subroutine */ void xerbla_(const char *, integer *);

  /*     .. Scalar Arguments .. */
  /*     .. */
  /*     .. Array Arguments .. */
  /*     .. */

  /*  Purpose */
  /*  ======= */

  /*  CHBMV  performs the matrix-vector  operation */

  /*     y := alpha*A*x + beta*y, */

  /*  where alpha and beta are scalars, x and y are n element vectors and */
  /*  A is an n by n hermitian band matrix, with k super-diagonals. */

  /*  Arguments */
  /*  ========== */

  /*  UPLO   - CHARACTER*1. */
  /*           On entry, UPLO specifies whether the upper or lower */
  /*           triangular part of the band matrix A is being supplied as */
  /*           follows: */

  /*              UPLO = 'U' or 'u'   The upper triangular part of A is */
  /*                                  being supplied. */

  /*              UPLO = 'L' or 'l'   The lower triangular part of A is */
  /*                                  being supplied. */

  /*           Unchanged on exit. */

  /*  N      - INTEGER. */
  /*           On entry, N specifies the order of the matrix A. */
  /*           N must be at least zero. */
  /*           Unchanged on exit. */

  /*  K      - INTEGER. */
  /*           On entry, K specifies the number of super-diagonals of the */
  /*           matrix A. K must satisfy  0 .le. K. */
  /*           Unchanged on exit. */

  /*  ALPHA  - COMPLEX         . */
  /*           On entry, ALPHA specifies the scalar alpha. */
  /*           Unchanged on exit. */

  /*  A      - COMPLEX          array of DIMENSION ( LDA, n ). */
  /*           Before entry with UPLO = 'U' or 'u', the leading ( k + 1 ) */
  /*           by n part of the array A must contain the upper triangular */
  /*           band part of the hermitian matrix, supplied column by */
  /*           column, with the leading diagonal of the matrix in row */
  /*           ( k + 1 ) of the array, the first super-diagonal starting at */
  /*           position 2 in row k, and so on. The top left k by k triangle */
  /*           of the array A is not referenced. */
  /*           The following program segment will transfer the upper */
  /*           triangular part of a hermitian band matrix from conventional */
  /*           full matrix storage to band storage: */

  /*                 DO 20, J = 1, N */
  /*                    M = K + 1 - J */
  /*                    DO 10, I = MAX( 1, J - K ), J */
  /*                       A( M + I, J ) = matrix( I, J ) */
  /*              10    CONTINUE */
  /*              20 CONTINUE */

  /*           Before entry with UPLO = 'L' or 'l', the leading ( k + 1 ) */
  /*           by n part of the array A must contain the lower triangular */
  /*           band part of the hermitian matrix, supplied column by */
  /*           column, with the leading diagonal of the matrix in row 1 of */
  /*           the array, the first sub-diagonal starting at position 1 in */
  /*           row 2, and so on. The bottom right k by k triangle of the */
  /*           array A is not referenced. */
  /*           The following program segment will transfer the lower */
  /*           triangular part of a hermitian band matrix from conventional */
  /*           full matrix storage to band storage: */

  /*                 DO 20, J = 1, N */
  /*                    M = 1 - J */
  /*                    DO 10, I = J, MIN( N, J + K ) */
  /*                       A( M + I, J ) = matrix( I, J ) */
  /*              10    CONTINUE */
  /*              20 CONTINUE */

  /*           Note that the imaginary parts of the diagonal elements need */
  /*           not be set and are assumed to be zero. */
  /*           Unchanged on exit. */

  /*  LDA    - INTEGER. */
  /*           On entry, LDA specifies the first dimension of A as declared */
  /*           in the calling (sub) program. LDA must be at least */
  /*           ( k + 1 ). */
  /*           Unchanged on exit. */

  /*  X      - COMPLEX          array of DIMENSION at least */
  /*           ( 1 + ( n - 1 )*abs( INCX ) ). */
  /*           Before entry, the incremented array X must contain the */
  /*           vector x. */
  /*           Unchanged on exit. */

  /*  INCX   - INTEGER. */
  /*           On entry, INCX specifies the increment for the elements of */
  /*           X. INCX must not be zero. */
  /*           Unchanged on exit. */

  /*  BETA   - COMPLEX         . */
  /*           On entry, BETA specifies the scalar beta. */
  /*           Unchanged on exit. */

  /*  Y      - COMPLEX          array of DIMENSION at least */
  /*           ( 1 + ( n - 1 )*abs( INCY ) ). */
  /*           Before entry, the incremented array Y must contain the */
  /*           vector y. On exit, Y is overwritten by the updated vector y. */

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
  /*     .. Intrinsic Functions .. */
  /*     .. */

  /*     Test the input parameters. */

  /* Parameter adjustments */
  a_dim1 = *lda;
  a_offset = 1 + a_dim1;
  a -= a_offset;
  --x;
  --y;

  /* Function Body */
  info = 0;
  if (!lsame_(uplo, "U") && !lsame_(uplo, "L")) {
    info = 1;
  } else if (*n < 0) {
    info = 2;
  } else if (*k < 0) {
    info = 3;
  } else if (*lda < *k + 1) {
    info = 6;
  } else if (*incx == 0) {
    info = 8;
  } else if (*incy == 0) {
    info = 11;
  }
  if (info != 0) {
    xerbla_("CHBMV ", &info);
    return;
  }

  /*     Quick return if possible. */

  if (*n == 0 || (alpha->r == 0.f && alpha->i == 0.f && (beta->r == 1.f && beta->i == 0.f))) {
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

  /*     Start the operations. In this version the elements of the array A */
  /*     are accessed sequentially with one pass through A. */

  /*     First form  y := beta*y. */

  if (beta->r != 1.f || beta->i != 0.f) {
    if (*incy == 1) {
      if (beta->r == 0.f && beta->i == 0.f) {
        i__1 = *n;
        for (i__ = 1; i__ <= i__1; ++i__) {
          i__2 = i__;
          y[i__2].r = 0.f, y[i__2].i = 0.f;
          /* L10: */
        }
      } else {
        i__1 = *n;
        for (i__ = 1; i__ <= i__1; ++i__) {
          i__2 = i__;
          i__3 = i__;
          q__1.r = beta->r * y[i__3].r - beta->i * y[i__3].i, q__1.i = beta->r * y[i__3].i + beta->i * y[i__3].r;
          y[i__2].r = q__1.r, y[i__2].i = q__1.i;
          /* L20: */
        }
      }
    } else {
      iy = ky;
      if (beta->r == 0.f && beta->i == 0.f) {
        i__1 = *n;
        for (i__ = 1; i__ <= i__1; ++i__) {
          i__2 = iy;
          y[i__2].r = 0.f, y[i__2].i = 0.f;
          iy += *incy;
          /* L30: */
        }
      } else {
        i__1 = *n;
        for (i__ = 1; i__ <= i__1; ++i__) {
          i__2 = iy;
          i__3 = iy;
          q__1.r = beta->r * y[i__3].r - beta->i * y[i__3].i, q__1.i = beta->r * y[i__3].i + beta->i * y[i__3].r;
          y[i__2].r = q__1.r, y[i__2].i = q__1.i;
          iy += *incy;
          /* L40: */
        }
      }
    }
  }
  if (alpha->r == 0.f && alpha->i == 0.f) {
    return;
  }
  if (lsame_(uplo, "U")) {
    /*        Form  y  when upper triangle of A is stored. */

    kplus1 = *k + 1;
    if (*incx == 1 && *incy == 1) {
      i__1 = *n;
      for (j = 1; j <= i__1; ++j) {
        i__2 = j;
        q__1.r = alpha->r * x[i__2].r - alpha->i * x[i__2].i, q__1.i = alpha->r * x[i__2].i + alpha->i * x[i__2].r;
        temp1.r = q__1.r, temp1.i = q__1.i;
        temp2.r = 0.f, temp2.i = 0.f;
        l = kplus1 - j;
        /* Computing MAX */
        i__2 = 1, i__3 = j - *k;
        i__4 = j - 1;
        for (i__ = max(i__2, i__3); i__ <= i__4; ++i__) {
          i__2 = i__;
          i__3 = i__;
          i__5 = l + i__ + j * a_dim1;
          q__2.r = temp1.r * a[i__5].r - temp1.i * a[i__5].i, q__2.i = temp1.r * a[i__5].i + temp1.i * a[i__5].r;
          q__1.r = y[i__3].r + q__2.r, q__1.i = y[i__3].i + q__2.i;
          y[i__2].r = q__1.r, y[i__2].i = q__1.i;
          r_cnjg(&q__3, &a[l + i__ + j * a_dim1]);
          i__2 = i__;
          q__2.r = q__3.r * x[i__2].r - q__3.i * x[i__2].i, q__2.i = q__3.r * x[i__2].i + q__3.i * x[i__2].r;
          q__1.r = temp2.r + q__2.r, q__1.i = temp2.i + q__2.i;
          temp2.r = q__1.r, temp2.i = q__1.i;
          /* L50: */
        }
        i__4 = j;
        i__2 = j;
        i__3 = kplus1 + j * a_dim1;
        r__1 = a[i__3].r;
        q__3.r = r__1 * temp1.r, q__3.i = r__1 * temp1.i;
        q__2.r = y[i__2].r + q__3.r, q__2.i = y[i__2].i + q__3.i;
        q__4.r = alpha->r * temp2.r - alpha->i * temp2.i, q__4.i = alpha->r * temp2.i + alpha->i * temp2.r;
        q__1.r = q__2.r + q__4.r, q__1.i = q__2.i + q__4.i;
        y[i__4].r = q__1.r, y[i__4].i = q__1.i;
        /* L60: */
      }
    } else {
      jx = kx;
      jy = ky;
      i__1 = *n;
      for (j = 1; j <= i__1; ++j) {
        i__4 = jx;
        q__1.r = alpha->r * x[i__4].r - alpha->i * x[i__4].i, q__1.i = alpha->r * x[i__4].i + alpha->i * x[i__4].r;
        temp1.r = q__1.r, temp1.i = q__1.i;
        temp2.r = 0.f, temp2.i = 0.f;
        ix = kx;
        iy = ky;
        l = kplus1 - j;
        /* Computing MAX */
        i__4 = 1, i__2 = j - *k;
        i__3 = j - 1;
        for (i__ = max(i__4, i__2); i__ <= i__3; ++i__) {
          i__4 = iy;
          i__2 = iy;
          i__5 = l + i__ + j * a_dim1;
          q__2.r = temp1.r * a[i__5].r - temp1.i * a[i__5].i, q__2.i = temp1.r * a[i__5].i + temp1.i * a[i__5].r;
          q__1.r = y[i__2].r + q__2.r, q__1.i = y[i__2].i + q__2.i;
          y[i__4].r = q__1.r, y[i__4].i = q__1.i;
          r_cnjg(&q__3, &a[l + i__ + j * a_dim1]);
          i__4 = ix;
          q__2.r = q__3.r * x[i__4].r - q__3.i * x[i__4].i, q__2.i = q__3.r * x[i__4].i + q__3.i * x[i__4].r;
          q__1.r = temp2.r + q__2.r, q__1.i = temp2.i + q__2.i;
          temp2.r = q__1.r, temp2.i = q__1.i;
          ix += *incx;
          iy += *incy;
          /* L70: */
        }
        i__3 = jy;
        i__4 = jy;
        i__2 = kplus1 + j * a_dim1;
        r__1 = a[i__2].r;
        q__3.r = r__1 * temp1.r, q__3.i = r__1 * temp1.i;
        q__2.r = y[i__4].r + q__3.r, q__2.i = y[i__4].i + q__3.i;
        q__4.r = alpha->r * temp2.r - alpha->i * temp2.i, q__4.i = alpha->r * temp2.i + alpha->i * temp2.r;
        q__1.r = q__2.r + q__4.r, q__1.i = q__2.i + q__4.i;
        y[i__3].r = q__1.r, y[i__3].i = q__1.i;
        jx += *incx;
        jy += *incy;
        if (j > *k) {
          kx += *incx;
          ky += *incy;
        }
        /* L80: */
      }
    }
  } else {
    /*        Form  y  when lower triangle of A is stored. */

    if (*incx == 1 && *incy == 1) {
      i__1 = *n;
      for (j = 1; j <= i__1; ++j) {
        i__3 = j;
        q__1.r = alpha->r * x[i__3].r - alpha->i * x[i__3].i, q__1.i = alpha->r * x[i__3].i + alpha->i * x[i__3].r;
        temp1.r = q__1.r, temp1.i = q__1.i;
        temp2.r = 0.f, temp2.i = 0.f;
        i__3 = j;
        i__4 = j;
        i__2 = j * a_dim1 + 1;
        r__1 = a[i__2].r;
        q__2.r = r__1 * temp1.r, q__2.i = r__1 * temp1.i;
        q__1.r = y[i__4].r + q__2.r, q__1.i = y[i__4].i + q__2.i;
        y[i__3].r = q__1.r, y[i__3].i = q__1.i;
        l = 1 - j;
        /* Computing MIN */
        i__4 = *n, i__2 = j + *k;
        i__3 = min(i__4, i__2);
        for (i__ = j + 1; i__ <= i__3; ++i__) {
          i__4 = i__;
          i__2 = i__;
          i__5 = l + i__ + j * a_dim1;
          q__2.r = temp1.r * a[i__5].r - temp1.i * a[i__5].i, q__2.i = temp1.r * a[i__5].i + temp1.i * a[i__5].r;
          q__1.r = y[i__2].r + q__2.r, q__1.i = y[i__2].i + q__2.i;
          y[i__4].r = q__1.r, y[i__4].i = q__1.i;
          r_cnjg(&q__3, &a[l + i__ + j * a_dim1]);
          i__4 = i__;
          q__2.r = q__3.r * x[i__4].r - q__3.i * x[i__4].i, q__2.i = q__3.r * x[i__4].i + q__3.i * x[i__4].r;
          q__1.r = temp2.r + q__2.r, q__1.i = temp2.i + q__2.i;
          temp2.r = q__1.r, temp2.i = q__1.i;
          /* L90: */
        }
        i__3 = j;
        i__4 = j;
        q__2.r = alpha->r * temp2.r - alpha->i * temp2.i, q__2.i = alpha->r * temp2.i + alpha->i * temp2.r;
        q__1.r = y[i__4].r + q__2.r, q__1.i = y[i__4].i + q__2.i;
        y[i__3].r = q__1.r, y[i__3].i = q__1.i;
        /* L100: */
      }
    } else {
      jx = kx;
      jy = ky;
      i__1 = *n;
      for (j = 1; j <= i__1; ++j) {
        i__3 = jx;
        q__1.r = alpha->r * x[i__3].r - alpha->i * x[i__3].i, q__1.i = alpha->r * x[i__3].i + alpha->i * x[i__3].r;
        temp1.r = q__1.r, temp1.i = q__1.i;
        temp2.r = 0.f, temp2.i = 0.f;
        i__3 = jy;
        i__4 = jy;
        i__2 = j * a_dim1 + 1;
        r__1 = a[i__2].r;
        q__2.r = r__1 * temp1.r, q__2.i = r__1 * temp1.i;
        q__1.r = y[i__4].r + q__2.r, q__1.i = y[i__4].i + q__2.i;
        y[i__3].r = q__1.r, y[i__3].i = q__1.i;
        l = 1 - j;
        ix = jx;
        iy = jy;
        /* Computing MIN */
        i__4 = *n, i__2 = j + *k;
        i__3 = min(i__4, i__2);
        for (i__ = j + 1; i__ <= i__3; ++i__) {
          ix += *incx;
          iy += *incy;
          i__4 = iy;
          i__2 = iy;
          i__5 = l + i__ + j * a_dim1;
          q__2.r = temp1.r * a[i__5].r - temp1.i * a[i__5].i, q__2.i = temp1.r * a[i__5].i + temp1.i * a[i__5].r;
          q__1.r = y[i__2].r + q__2.r, q__1.i = y[i__2].i + q__2.i;
          y[i__4].r = q__1.r, y[i__4].i = q__1.i;
          r_cnjg(&q__3, &a[l + i__ + j * a_dim1]);
          i__4 = ix;
          q__2.r = q__3.r * x[i__4].r - q__3.i * x[i__4].i, q__2.i = q__3.r * x[i__4].i + q__3.i * x[i__4].r;
          q__1.r = temp2.r + q__2.r, q__1.i = temp2.i + q__2.i;
          temp2.r = q__1.r, temp2.i = q__1.i;
          /* L110: */
        }
        i__3 = jy;
        i__4 = jy;
        q__2.r = alpha->r * temp2.r - alpha->i * temp2.i, q__2.i = alpha->r * temp2.i + alpha->i * temp2.r;
        q__1.r = y[i__4].r + q__2.r, q__1.i = y[i__4].i + q__2.i;
        y[i__3].r = q__1.r, y[i__3].i = q__1.i;
        jx += *incx;
        jy += *incy;
        /* L120: */
      }
    }
  }

  /*     End of CHBMV . */

} /* chbmv_ */
