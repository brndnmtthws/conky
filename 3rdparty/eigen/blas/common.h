// This file is part of Eigen, a lightweight C++ template library
// for linear algebra.
//
// Copyright (C) 2009-2015 Gael Guennebaud <gael.guennebaud@inria.fr>
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef EIGEN_BLAS_COMMON_H
#define EIGEN_BLAS_COMMON_H

#ifdef __GNUC__
#if __GNUC__ < 5
// GCC < 5.0 does not like the global Scalar typedef
// we just keep shadow-warnings disabled permanently
#define EIGEN_PERMANENTLY_DISABLE_STUPID_WARNINGS
#endif
#endif

#include "../Eigen/Core"
#include "../Eigen/Jacobi"

#include <complex>

#ifndef SCALAR
#error the token SCALAR must be defined to compile this file
#endif

#include "blas.h"

#include "BandTriangularSolver.h"
#include "GeneralRank1Update.h"
#include "PackedSelfadjointProduct.h"
#include "PackedTriangularMatrixVector.h"
#include "PackedTriangularSolverVector.h"
#include "Rank2Update.h"

#define NOTR 0
#define TR 1
#define ADJ 2

#define LEFT 0
#define RIGHT 1

#define UP 0
#define LO 1

#define NUNIT 0
#define UNIT 1

#define INVALID 0xff

#define OP(X) \
  (((X) == 'N' || (X) == 'n') ? NOTR : ((X) == 'T' || (X) == 't') ? TR : ((X) == 'C' || (X) == 'c') ? ADJ : INVALID)

#define SIDE(X) (((X) == 'L' || (X) == 'l') ? LEFT : ((X) == 'R' || (X) == 'r') ? RIGHT : INVALID)

#define UPLO(X) (((X) == 'U' || (X) == 'u') ? UP : ((X) == 'L' || (X) == 'l') ? LO : INVALID)

#define DIAG(X) (((X) == 'N' || (X) == 'n') ? NUNIT : ((X) == 'U' || (X) == 'u') ? UNIT : INVALID)

inline bool check_op(const char* op) { return OP(*op) != 0xff; }

inline bool check_side(const char* side) { return SIDE(*side) != 0xff; }

inline bool check_uplo(const char* uplo) { return UPLO(*uplo) != 0xff; }

typedef SCALAR Scalar;
typedef Eigen::NumTraits<Scalar>::Real RealScalar;
typedef std::complex<RealScalar> Complex;

enum { IsComplex = Eigen::NumTraits<SCALAR>::IsComplex, Conj = IsComplex };

typedef Eigen::Matrix<Scalar, Eigen::Dynamic, Eigen::Dynamic, Eigen::ColMajor> PlainMatrixType;
typedef Eigen::Map<Eigen::Matrix<Scalar, Eigen::Dynamic, Eigen::Dynamic, Eigen::ColMajor>, 0, Eigen::OuterStride<> >
    MatrixType;
typedef Eigen::Map<const Eigen::Matrix<Scalar, Eigen::Dynamic, Eigen::Dynamic, Eigen::ColMajor>, 0,
                   Eigen::OuterStride<> >
    ConstMatrixType;
typedef Eigen::Map<Eigen::Matrix<Scalar, Eigen::Dynamic, 1>, 0, Eigen::InnerStride<Eigen::Dynamic> > StridedVectorType;
typedef Eigen::Map<Eigen::Matrix<Scalar, Eigen::Dynamic, 1> > CompactVectorType;

template <typename T>
Eigen::Map<Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic, Eigen::ColMajor>, 0, Eigen::OuterStride<> > matrix(
    T* data, int rows, int cols, int stride) {
  return Eigen::Map<Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic, Eigen::ColMajor>, 0, Eigen::OuterStride<> >(
      data, rows, cols, Eigen::OuterStride<>(stride));
}

template <typename T>
Eigen::Map<const Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic, Eigen::ColMajor>, 0, Eigen::OuterStride<> > matrix(
    const T* data, int rows, int cols, int stride) {
  return Eigen::Map<const Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic, Eigen::ColMajor>, 0, Eigen::OuterStride<> >(
      data, rows, cols, Eigen::OuterStride<>(stride));
}

template <typename T>
Eigen::Map<Eigen::Matrix<T, Eigen::Dynamic, 1>, 0, Eigen::InnerStride<Eigen::Dynamic> > make_vector(T* data, int size,
                                                                                                    int incr) {
  return Eigen::Map<Eigen::Matrix<T, Eigen::Dynamic, 1>, 0, Eigen::InnerStride<Eigen::Dynamic> >(
      data, size, Eigen::InnerStride<Eigen::Dynamic>(incr));
}

template <typename T>
Eigen::Map<const Eigen::Matrix<T, Eigen::Dynamic, 1>, 0, Eigen::InnerStride<Eigen::Dynamic> > make_vector(const T* data,
                                                                                                          int size,
                                                                                                          int incr) {
  return Eigen::Map<const Eigen::Matrix<T, Eigen::Dynamic, 1>, 0, Eigen::InnerStride<Eigen::Dynamic> >(
      data, size, Eigen::InnerStride<Eigen::Dynamic>(incr));
}

template <typename T>
Eigen::Map<Eigen::Matrix<T, Eigen::Dynamic, 1> > make_vector(T* data, int size) {
  return Eigen::Map<Eigen::Matrix<T, Eigen::Dynamic, 1> >(data, size);
}

template <typename T>
Eigen::Map<const Eigen::Matrix<T, Eigen::Dynamic, 1> > make_vector(const T* data, int size) {
  return Eigen::Map<const Eigen::Matrix<T, Eigen::Dynamic, 1> >(data, size);
}

template <typename T>
T* get_compact_vector(T* x, int n, int incx) {
  if (incx == 1) return x;

  std::remove_const_t<T>* ret = new Scalar[n];
  if (incx < 0)
    make_vector(ret, n) = make_vector(x, n, -incx).reverse();
  else
    make_vector(ret, n) = make_vector(x, n, incx);
  return ret;
}

template <typename T>
T* copy_back(T* x_cpy, T* x, int n, int incx) {
  if (x_cpy == x) return 0;

  if (incx < 0)
    make_vector(x, n, -incx).reverse() = make_vector(x_cpy, n);
  else
    make_vector(x, n, incx) = make_vector(x_cpy, n);
  return x_cpy;
}

#ifndef EIGEN_BLAS_FUNC_SUFFIX
#define EIGEN_BLAS_FUNC_SUFFIX _
#endif

#define EIGEN_BLAS_FUNC_NAME(X) EIGEN_CAT(SCALAR_SUFFIX, EIGEN_CAT(X, EIGEN_BLAS_FUNC_SUFFIX))
#define EIGEN_BLAS_FUNC(X) extern "C" void EIGEN_BLAS_FUNC_NAME(X)

#endif  // EIGEN_BLAS_COMMON_H
