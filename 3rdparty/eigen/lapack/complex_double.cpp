// This file is part of Eigen, a lightweight C++ template library
// for linear algebra.
//
// Copyright (C) 2009-2014 Gael Guennebaud <gael.guennebaud@inria.fr>
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#define SCALAR std::complex<double>
#define SCALAR_SUFFIX z
#define SCALAR_SUFFIX_UP "Z"
#define REAL_SCALAR_SUFFIX d
#define ISCOMPLEX 1

#include "cholesky.inc"
#include "lu.inc"
#include "svd.inc"
