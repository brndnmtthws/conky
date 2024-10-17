/*  This file is part of the Vc library. {{{
Copyright © 2009-2015 Matthias Kretz <kretz@kde.org>

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the names of contributing organizations nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

}}}*/

#ifndef VC_VECTOR_H_
#define VC_VECTOR_H_

// 1. define all of Vc::Scalar - this one is always present, so it makes sense to put it first
#include "scalar/vector.h"

#if defined(Vc_IMPL_SSE)
# include "sse/vector.h"
#endif
#ifdef Vc_IMPL_AVX
# include "avx/vector.h"
#endif

namespace Vc_VERSIONED_NAMESPACE
{
/**
 * \name Vector Type Aliases
 * \ingroup Vectors
 */
///@{
/**\addtogroup Vectors
 * @{
 */
/// vector of double precision
using double_v = Vector<double>;
/// vector of single precision
using float_v = Vector<float>;
/// vector of signed integers
using int_v = Vector<int>;
/// vector of unsigned integers
using uint_v = Vector<uint>;
/// vector of signed short integers
using short_v = Vector<short>;
/// vector of unsigned short integers
using ushort_v = Vector<ushort>;
///\internal vector of signed long long integers
using llong_v = Vector<llong>;
///\internal vector of unsigned long long integers
using ullong_v = Vector<ullong>;
///\internal vector of signed long integers
using long_v = Vector<long>;
///\internal vector of unsigned long integers
using ulong_v = Vector<ulong>;
///\internal vector of signed char-sized integers
using schar_v = Vector<schar>;
///\internal vector of unsigned char-sized integers
using uchar_v = Vector<uchar>;
///@}
///@}

/**
 * \name Mask Type Aliases
 * \ingroup Masks
 */
///@{
/**\addtogroup Masks
 * @{
 */
/// mask type for double_v vectors
using double_m = Mask<double>;
/// mask type for float_v vectors
using  float_m = Mask< float>;
///\internal mask type for llong_v vectors
using  llong_m = Mask< llong>;
///\internal mask type for ullong_v vectors
using ullong_m = Mask<ullong>;
///\internal mask type for long_v vectors
using   long_m = Mask<  long>;
///\internal mask type for ulong_v vectors
using  ulong_m = Mask< ulong>;
/// mask type for int_v vectors
using    int_m = Mask<   int>;
/// mask type for uint_v vectors
using   uint_m = Mask<  uint>;
/// mask type for short_v vectors
using  short_m = Mask< short>;
/// mask type for ushort_v vectors
using ushort_m = Mask<ushort>;
///\internal mask type for schar_v vectors
using  schar_m = Mask< schar>;
///\internal mask type for uchar_v vectors
using  uchar_m = Mask< uchar>;
///@}
///@}

  typedef Vector<std:: int_least64_t>  int_least64_v;
  typedef Vector<std::uint_least64_t> uint_least64_v;
  typedef Vector<std:: int_least32_t>  int_least32_v;
  typedef Vector<std::uint_least32_t> uint_least32_v;
  typedef Vector<std:: int_least16_t>  int_least16_v;
  typedef Vector<std::uint_least16_t> uint_least16_v;
  typedef Vector<std::  int_least8_t>   int_least8_v;
  typedef Vector<std:: uint_least8_t>  uint_least8_v;

  typedef Mask<std:: int_least64_t>  int_least64_m;
  typedef Mask<std::uint_least64_t> uint_least64_m;
  typedef Mask<std:: int_least32_t>  int_least32_m;
  typedef Mask<std::uint_least32_t> uint_least32_m;
  typedef Mask<std:: int_least16_t>  int_least16_m;
  typedef Mask<std::uint_least16_t> uint_least16_m;
  typedef Mask<std::  int_least8_t>   int_least8_m;
  typedef Mask<std:: uint_least8_t>  uint_least8_m;

  typedef Vector<std:: int_fast64_t>  int_fast64_v;
  typedef Vector<std::uint_fast64_t> uint_fast64_v;
  typedef Vector<std:: int_fast32_t>  int_fast32_v;
  typedef Vector<std::uint_fast32_t> uint_fast32_v;
  typedef Vector<std:: int_fast16_t>  int_fast16_v;
  typedef Vector<std::uint_fast16_t> uint_fast16_v;
  typedef Vector<std::  int_fast8_t>   int_fast8_v;
  typedef Vector<std:: uint_fast8_t>  uint_fast8_v;

  typedef Mask<std:: int_fast64_t>  int_fast64_m;
  typedef Mask<std::uint_fast64_t> uint_fast64_m;
  typedef Mask<std:: int_fast32_t>  int_fast32_m;
  typedef Mask<std::uint_fast32_t> uint_fast32_m;
  typedef Mask<std:: int_fast16_t>  int_fast16_m;
  typedef Mask<std::uint_fast16_t> uint_fast16_m;
  typedef Mask<std::  int_fast8_t>   int_fast8_m;
  typedef Mask<std:: uint_fast8_t>  uint_fast8_m;

#if defined INT64_MAX && defined UINT64_MAX
  typedef Vector<std:: int64_t>  int64_v;
  typedef Vector<std::uint64_t> uint64_v;
  typedef Mask<std:: int64_t>  int64_m;
  typedef Mask<std::uint64_t> uint64_m;
#endif
#if defined INT32_MAX && defined UINT32_MAX
  typedef Vector<std:: int32_t>  int32_v;
  typedef Vector<std::uint32_t> uint32_v;
  typedef Mask<std:: int32_t>  int32_m;
  typedef Mask<std::uint32_t> uint32_m;
#endif
#if defined INT16_MAX && defined UINT16_MAX
  typedef Vector<std:: int16_t>  int16_v;
  typedef Vector<std::uint16_t> uint16_v;
  typedef Mask<std:: int16_t>  int16_m;
  typedef Mask<std::uint16_t> uint16_m;
#endif
#if defined INT8_MAX && defined UINT8_MAX
  typedef Vector<std:: int8_t>  int8_v;
  typedef Vector<std::uint8_t> uint8_v;
  typedef Mask<std:: int8_t>  int8_m;
  typedef Mask<std::uint8_t> uint8_m;
#endif

  namespace {
    static_assert(double_v::Size == Vc_DOUBLE_V_SIZE, "Vc_DOUBLE_V_SIZE macro defined to an incorrect value");
    static_assert(float_v::Size  == Vc_FLOAT_V_SIZE , "Vc_FLOAT_V_SIZE macro defined to an incorrect value ");
    static_assert(int_v::Size    == Vc_INT_V_SIZE   , "Vc_INT_V_SIZE macro defined to an incorrect value   ");
    static_assert(uint_v::Size   == Vc_UINT_V_SIZE  , "Vc_UINT_V_SIZE macro defined to an incorrect value  ");
    static_assert(short_v::Size  == Vc_SHORT_V_SIZE , "Vc_SHORT_V_SIZE macro defined to an incorrect value ");
    static_assert(ushort_v::Size == Vc_USHORT_V_SIZE, "Vc_USHORT_V_SIZE macro defined to an incorrect value");
  }
}


// finally define the non-member operators
#include "common/operators.h"

#include "common/simdarray.h"
// XXX See bottom of common/simdmaskarray.h:
//#include "common/simd_cast_caller.tcc"

#include "common/alignedbase.h"
namespace Vc_VERSIONED_NAMESPACE {
/**
 * \ingroup Vectors
 *
 * Specifies the most conservative memory alignment necessary for Vector<T> objects with
 * default VectorAbi. Use this value e.g. with an \c alignas expression or when allocating
 * aligned memory dynamically (\ref Vc::malloc).
 *
 * \see Vc::MemoryAlignment
 * \see Vc::VectorAlignedBase
 */
constexpr std::size_t VectorAlignment = alignof(VectorAlignedBase);
/**
 * \ingroup Vectors
 *
 * Specifies the most conservative memory alignment necessary for aligned loads and stores
 * of Vector types. Use this value e.g. with an \c alignas expression or when allocating
 * aligned memory dynamically (\ref Vc::malloc).
 *
 * \see Vc::VectorAlignment
 * \see Vc::Vector<T, Abi>::MemoryAlignment
 */
constexpr std::size_t MemoryAlignment = alignof(MemoryAlignedBase);
} // namespace Vc_VERSIONED_NAMESPACE

#define Vc_VECTOR_DECLARED_ 1

#include "scalar/helperimpl.h"
#include "scalar/math.h"
#include "scalar/simd_cast_caller.tcc"
#if defined(Vc_IMPL_SSE)
# include "sse/helperimpl.h"
# include "sse/math.h"
# include "sse/simd_cast_caller.tcc"
#endif
#if defined(Vc_IMPL_AVX)
# include "avx/helperimpl.h"
# include "avx/math.h"
# include "avx/simd_cast_caller.tcc"
#endif

#include "common/math.h"

#ifdef isfinite
#undef isfinite
#endif
#ifdef isnan
#undef isnan
#endif

#include "common/vectortuple.h"
#include "common/where.h"
#include "common/iif.h"

#ifndef Vc_NO_STD_FUNCTIONS
namespace std
{
  using Vc::min;
  using Vc::max;

  using Vc::abs;
  using Vc::asin;
  using Vc::atan;
  using Vc::atan2;
  using Vc::ceil;
  using Vc::cos;
  using Vc::exp;
  using Vc::fma;
  using Vc::trunc;
  using Vc::floor;
  using Vc::frexp;
  using Vc::ldexp;
  using Vc::log;
  using Vc::log10;
  using Vc::log2;
  using Vc::round;
  using Vc::sin;
  using Vc::sqrt;

  using Vc::isfinite;
  using Vc::isnan;
} // namespace std
#endif

Vc_RESET_DIAGNOSTICS

#endif // VC_VECTOR_H_
