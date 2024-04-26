// This file is part of Eigen, a lightweight C++ template library
// for linear algebra.
//
// Copyright (C) 2009-2010 Gael Guennebaud <gael.guennebaud@inria.fr>
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "common.h"

// computes the sum of magnitudes of all vector elements or, for a complex vector x, the sum
// res = |Rex1| + |Imx1| + |Rex2| + |Imx2| + ... + |Rexn| + |Imxn|, where x is a vector of order n
extern "C" RealScalar EIGEN_BLAS_FUNC_NAME(asum)(int *n, Scalar *px, int *incx) {
  //   std::cerr << "_asum " << *n << " " << *incx << "\n";

  Scalar *x = reinterpret_cast<Scalar *>(px);

  if (*n <= 0) return 0;

  if (*incx == 1)
    return make_vector(x, *n).cwiseAbs().sum();
  else
    return make_vector(x, *n, std::abs(*incx)).cwiseAbs().sum();
}

extern "C" int EIGEN_CAT(i, EIGEN_BLAS_FUNC_NAME(amax))(int *n, Scalar *px, int *incx) {
  if (*n <= 0) return 0;
  Scalar *x = reinterpret_cast<Scalar *>(px);

  Eigen::DenseIndex ret;
  if (*incx == 1)
    make_vector(x, *n).cwiseAbs().maxCoeff(&ret);
  else
    make_vector(x, *n, std::abs(*incx)).cwiseAbs().maxCoeff(&ret);
  return int(ret) + 1;
}

extern "C" int EIGEN_CAT(i, EIGEN_BLAS_FUNC_NAME(amin))(int *n, Scalar *px, int *incx) {
  if (*n <= 0) return 0;
  Scalar *x = reinterpret_cast<Scalar *>(px);

  Eigen::DenseIndex ret;
  if (*incx == 1)
    make_vector(x, *n).cwiseAbs().minCoeff(&ret);
  else
    make_vector(x, *n, std::abs(*incx)).cwiseAbs().minCoeff(&ret);
  return int(ret) + 1;
}

// computes a vector-vector dot product.
extern "C" Scalar EIGEN_BLAS_FUNC_NAME(dot)(int *n, Scalar *px, int *incx, Scalar *py, int *incy) {
  //   std::cerr << "_dot " << *n << " " << *incx << " " << *incy << "\n";

  if (*n <= 0) return 0;

  Scalar *x = reinterpret_cast<Scalar *>(px);
  Scalar *y = reinterpret_cast<Scalar *>(py);

  if (*incx == 1 && *incy == 1)
    return (make_vector(x, *n).cwiseProduct(make_vector(y, *n))).sum();
  else if (*incx > 0 && *incy > 0)
    return (make_vector(x, *n, *incx).cwiseProduct(make_vector(y, *n, *incy))).sum();
  else if (*incx < 0 && *incy > 0)
    return (make_vector(x, *n, -*incx).reverse().cwiseProduct(make_vector(y, *n, *incy))).sum();
  else if (*incx > 0 && *incy < 0)
    return (make_vector(x, *n, *incx).cwiseProduct(make_vector(y, *n, -*incy).reverse())).sum();
  else if (*incx < 0 && *incy < 0)
    return (make_vector(x, *n, -*incx).reverse().cwiseProduct(make_vector(y, *n, -*incy).reverse())).sum();
  else
    return 0;
}

// computes the Euclidean norm of a vector.
// FIXME
extern "C" Scalar EIGEN_BLAS_FUNC_NAME(nrm2)(int *n, Scalar *px, int *incx) {
  //   std::cerr << "_nrm2 " << *n << " " << *incx << "\n";
  if (*n <= 0) return 0;

  Scalar *x = reinterpret_cast<Scalar *>(px);

  if (*incx == 1)
    return make_vector(x, *n).stableNorm();
  else
    return make_vector(x, *n, std::abs(*incx)).stableNorm();
}

EIGEN_BLAS_FUNC(rot)(int *n, Scalar *px, int *incx, Scalar *py, int *incy, Scalar *pc, Scalar *ps) {
  //   std::cerr << "_rot " << *n << " " << *incx << " " << *incy << "\n";
  if (*n <= 0) return;

  Scalar *x = reinterpret_cast<Scalar *>(px);
  Scalar *y = reinterpret_cast<Scalar *>(py);
  Scalar c = *reinterpret_cast<Scalar *>(pc);
  Scalar s = *reinterpret_cast<Scalar *>(ps);

  StridedVectorType vx(make_vector(x, *n, std::abs(*incx)));
  StridedVectorType vy(make_vector(y, *n, std::abs(*incy)));

  Eigen::Reverse<StridedVectorType> rvx(vx);
  Eigen::Reverse<StridedVectorType> rvy(vy);

  if (*incx < 0 && *incy > 0)
    Eigen::internal::apply_rotation_in_the_plane(rvx, vy, Eigen::JacobiRotation<Scalar>(c, s));
  else if (*incx > 0 && *incy < 0)
    Eigen::internal::apply_rotation_in_the_plane(vx, rvy, Eigen::JacobiRotation<Scalar>(c, s));
  else
    Eigen::internal::apply_rotation_in_the_plane(vx, vy, Eigen::JacobiRotation<Scalar>(c, s));
}

/*
// performs rotation of points in the modified plane.
EIGEN_BLAS_FUNC(rotm)(int *n, Scalar *px, int *incx, Scalar *py, int *incy, Scalar *param)
{
  Scalar* x = reinterpret_cast<Scalar*>(px);
  Scalar* y = reinterpret_cast<Scalar*>(py);

  // TODO

  return 0;
}

// computes the modified parameters for a Givens rotation.
EIGEN_BLAS_FUNC(rotmg)(Scalar *d1, Scalar *d2, Scalar *x1, Scalar *x2, Scalar *param)
{
  // TODO

  return 0;
}
*/
