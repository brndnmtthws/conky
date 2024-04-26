// This file is part of Eigen, a lightweight C++ template library
// for linear algebra.
//
// Copyright (C) 2010-2014 Gael Guennebaud <gael.guennebaud@inria.fr>
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef EIGEN_LAPACK_COMMON_H
#define EIGEN_LAPACK_COMMON_H

#include "../blas/common.h"
#include "lapack.h"

#define EIGEN_LAPACK_FUNC(FUNC) EIGEN_BLAS_FUNC(FUNC)

typedef Eigen::Map<Eigen::Transpositions<Eigen::Dynamic, Eigen::Dynamic, int> > PivotsType;

#endif  // EIGEN_LAPACK_COMMON_H
