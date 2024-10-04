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

#ifndef VC_AVX_TYPES_H_
#define VC_AVX_TYPES_H_

#include "../sse/types.h"
#include "../traits/type_traits.h"
#include "macros.h"

#ifdef Vc_DEFAULT_IMPL_AVX2
#define Vc_DOUBLE_V_SIZE 4
#define Vc_FLOAT_V_SIZE 8
#define Vc_INT_V_SIZE 8
#define Vc_UINT_V_SIZE 8
#define Vc_SHORT_V_SIZE 16
#define Vc_USHORT_V_SIZE 16
#elif defined Vc_DEFAULT_IMPL_AVX
#define Vc_DOUBLE_V_SIZE 4
#define Vc_FLOAT_V_SIZE 8
#define Vc_INT_V_SIZE 4
#define Vc_UINT_V_SIZE 4
#define Vc_SHORT_V_SIZE 8
#define Vc_USHORT_V_SIZE 8
#endif

namespace Vc_VERSIONED_NAMESPACE
{
namespace AVX
{
template <typename T> using Vector = Vc::Vector<T, VectorAbi::Avx1Abi<T>>;
typedef Vector<double>         double_v;
typedef Vector<float>           float_v;
typedef Vector<int>               int_v;
typedef Vector<unsigned int>     uint_v;
typedef Vector<short>           short_v;
typedef Vector<unsigned short> ushort_v;

template <typename T> using Mask = Vc::Mask<T, VectorAbi::Avx1Abi<T>>;
typedef Mask<double>         double_m;
typedef Mask<float>           float_m;
typedef Mask<int>               int_m;
typedef Mask<unsigned int>     uint_m;
typedef Mask<short>           short_m;
typedef Mask<unsigned short> ushort_m;

template <typename T> struct Const;

template <typename T> struct is_vector : public std::false_type {};
template <typename T> struct is_vector<Vector<T>> : public std::true_type {};
template <typename T> struct is_mask : public std::false_type {};
template <typename T> struct is_mask<Mask<T>> : public std::true_type {};
}  // namespace AVX

namespace AVX2
{
template <typename T> using Vector = Vc::Vector<T, VectorAbi::Avx>;
using double_v = Vector<double>;
using  float_v = Vector< float>;
using    int_v = Vector<   int>;
using   uint_v = Vector<  uint>;
using  short_v = Vector< short>;
using ushort_v = Vector<ushort>;

template <typename T> using Mask = Vc::Mask<T, VectorAbi::Avx>;
using double_m = Mask<double>;
using  float_m = Mask< float>;
using  llong_m = Mask< llong>;
using ullong_m = Mask<ullong>;
using   long_m = Mask<  long>;
using  ulong_m = Mask< ulong>;
using    int_m = Mask<   int>;
using   uint_m = Mask<  uint>;
using  short_m = Mask< short>;
using ushort_m = Mask<ushort>;
using  schar_m = Mask< schar>;
using  uchar_m = Mask< uchar>;

template <typename T> struct is_vector : public std::false_type {};
template <typename T> struct is_vector<Vector<T>> : public std::true_type {};
template <typename T> struct is_mask : public std::false_type {};
template <typename T> struct is_mask<Mask<T>> : public std::true_type {};
}  // namespace AVX2

namespace Traits
{
template <class T> struct
is_simd_vector_internal<Vector<T, VectorAbi::Avx>>
  : public is_valid_vector_argument<T> {};

template<typename T> struct is_simd_mask_internal<Mask<T, VectorAbi::Avx>>
  : public std::true_type {};
}  // namespace Traits
}  // namespace Vc

#endif // VC_AVX_TYPES_H_
