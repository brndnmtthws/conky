/*  This file is part of the Vc library. {{{
Copyright © 2014-2015 Matthias Kretz <kretz@kde.org>

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

#ifndef VC_SSE_SIMD_CAST_H_
#define VC_SSE_SIMD_CAST_H_

#include "../common/utility.h"
#ifdef Vc_IMPL_AVX
#include "../avx/casts.h"
#endif

#ifndef VC_SSE_VECTOR_H_
#error "Vc/sse/vector.h needs to be included before Vc/sse/simd_cast.h"
#endif
#include "macros.h"

namespace Vc_VERSIONED_NAMESPACE
{
namespace SSE
{

// Declarations: helper macros Vc_SIMD_CAST_[1248] {{{1
#define Vc_SIMD_CAST_1(from_, to_)                                                       \
    template <typename To>                                                               \
    Vc_INTRINSIC Vc_CONST To simd_cast(                                                  \
        from_ x, enable_if<std::is_same<To, to_>::value> = nullarg)

#define Vc_SIMD_CAST_2(from_, to_)                                                       \
    template <typename To>                                                               \
    Vc_INTRINSIC Vc_CONST To simd_cast(                                                  \
        from_ x0, from_ x1, enable_if<std::is_same<To, to_>::value> = nullarg)

#define Vc_SIMD_CAST_4(from_, to_)                                                       \
    template <typename To>                                                               \
    Vc_INTRINSIC Vc_CONST To simd_cast(                                                  \
        from_ x0, from_ x1, from_ x2, from_ x3,                                          \
        enable_if<std::is_same<To, to_>::value> = nullarg)

#define Vc_SIMD_CAST_8(from_, to_)                                                       \
    template <typename To>                                                               \
    Vc_INTRINSIC Vc_CONST To simd_cast(                                                  \
        from_ x0, from_ x1, from_ x2, from_ x3, from_ x4, from_ x5, from_ x6, from_ x7,  \
        enable_if<std::is_same<To, to_>::value> = nullarg)

// Declarations: Vector casts without offset {{{1
// 1 SSE::Vector to 1 SSE::Vector {{{2
Vc_SIMD_CAST_1( float_v,    int_v);
Vc_SIMD_CAST_1(double_v,    int_v);
Vc_SIMD_CAST_1(  uint_v,    int_v);
Vc_SIMD_CAST_1( short_v,    int_v);
Vc_SIMD_CAST_1(ushort_v,    int_v);
Vc_SIMD_CAST_1( float_v,   uint_v);
Vc_SIMD_CAST_1(double_v,   uint_v);
Vc_SIMD_CAST_1(   int_v,   uint_v);
Vc_SIMD_CAST_1( short_v,   uint_v);
Vc_SIMD_CAST_1(ushort_v,   uint_v);
Vc_SIMD_CAST_1(double_v,  float_v);
Vc_SIMD_CAST_1(   int_v,  float_v);
Vc_SIMD_CAST_1(  uint_v,  float_v);
Vc_SIMD_CAST_1( short_v,  float_v);
Vc_SIMD_CAST_1(ushort_v,  float_v);
Vc_SIMD_CAST_1( float_v, double_v);
Vc_SIMD_CAST_1(   int_v, double_v);
Vc_SIMD_CAST_1(  uint_v, double_v);
Vc_SIMD_CAST_1( short_v, double_v);
Vc_SIMD_CAST_1(ushort_v, double_v);
Vc_SIMD_CAST_1(   int_v,  short_v);
Vc_SIMD_CAST_1(  uint_v,  short_v);
Vc_SIMD_CAST_1( float_v,  short_v);
Vc_SIMD_CAST_1(double_v,  short_v);
Vc_SIMD_CAST_1(ushort_v,  short_v);
Vc_SIMD_CAST_1(   int_v, ushort_v);
Vc_SIMD_CAST_1(  uint_v, ushort_v);
Vc_SIMD_CAST_1( float_v, ushort_v);
Vc_SIMD_CAST_1(double_v, ushort_v);
Vc_SIMD_CAST_1( short_v, ushort_v);

// 2 SSE::Vector to 1 SSE::Vector {{{2
Vc_SIMD_CAST_2(double_v,    int_v);
Vc_SIMD_CAST_2(double_v,   uint_v);
Vc_SIMD_CAST_2(double_v,  float_v);
Vc_SIMD_CAST_2(   int_v,  short_v);
Vc_SIMD_CAST_2(  uint_v,  short_v);
Vc_SIMD_CAST_2( float_v,  short_v);
Vc_SIMD_CAST_2(double_v,  short_v);
Vc_SIMD_CAST_2(   int_v, ushort_v);
Vc_SIMD_CAST_2(  uint_v, ushort_v);
Vc_SIMD_CAST_2( float_v, ushort_v);
Vc_SIMD_CAST_2(double_v, ushort_v);

// 3 SSE::Vector to 1 SSE::Vector {{{2
#define Vc_CAST_(To_)                                                                    \
    template <typename Return>                                                           \
    Vc_INTRINSIC Vc_CONST enable_if<std::is_same<Return, To_>::value, Return>
Vc_CAST_(short_v) simd_cast(double_v a, double_v b, double_v c);
Vc_CAST_(ushort_v) simd_cast(double_v a, double_v b, double_v c);

// 4 SSE::Vector to 1 SSE::Vector {{{2
Vc_SIMD_CAST_4(double_v,  short_v);
Vc_SIMD_CAST_4(double_v, ushort_v);
//}}}2
}  // namespace SSE
using SSE::simd_cast;

// 1 Scalar::Vector to 1 SSE::Vector {{{2
template <typename Return, typename T>
Vc_INTRINSIC Vc_CONST Return
simd_cast(Scalar::Vector<T> x,
          enable_if<std::is_same<Return, SSE::double_v>::value> = nullarg);
template <typename Return, typename T>
Vc_INTRINSIC Vc_CONST Return
simd_cast(Scalar::Vector<T> x,
          enable_if<std::is_same<Return, SSE::float_v>::value> = nullarg);
template <typename Return, typename T>
Vc_INTRINSIC Vc_CONST Return
simd_cast(Scalar::Vector<T> x,
          enable_if<std::is_same<Return, SSE::int_v>::value> = nullarg);
template <typename Return, typename T>
Vc_INTRINSIC Vc_CONST Return
simd_cast(Scalar::Vector<T> x,
          enable_if<std::is_same<Return, SSE::uint_v>::value> = nullarg);
template <typename Return, typename T>
Vc_INTRINSIC Vc_CONST Return
simd_cast(Scalar::Vector<T> x,
          enable_if<std::is_same<Return, SSE::short_v>::value> = nullarg);
template <typename Return, typename T>
Vc_INTRINSIC Vc_CONST Return
simd_cast(Scalar::Vector<T> x,
          enable_if<std::is_same<Return, SSE::ushort_v>::value> = nullarg);

// 2 Scalar::Vector to 1 SSE::Vector {{{2
template <typename Return, typename T>
Vc_INTRINSIC Vc_CONST Return
simd_cast(Scalar::Vector<T> x0, Scalar::Vector<T> x1,
          enable_if<std::is_same<Return, SSE::double_v>::value> = nullarg);
template <typename Return, typename T>
Vc_INTRINSIC Vc_CONST Return
simd_cast(Scalar::Vector<T> x0, Scalar::Vector<T> x1,
          enable_if<std::is_same<Return, SSE::float_v>::value> = nullarg);
template <typename Return, typename T>
Vc_INTRINSIC Vc_CONST Return
simd_cast(Scalar::Vector<T> x0, Scalar::Vector<T> x1,
          enable_if<std::is_same<Return, SSE::int_v>::value> = nullarg);
template <typename Return, typename T>
Vc_INTRINSIC Vc_CONST Return
simd_cast(Scalar::Vector<T> x0, Scalar::Vector<T> x1,
          enable_if<std::is_same<Return, SSE::uint_v>::value> = nullarg);
template <typename Return, typename T>
Vc_INTRINSIC Vc_CONST Return
simd_cast(Scalar::Vector<T> x0, Scalar::Vector<T> x1,
          enable_if<std::is_same<Return, SSE::short_v>::value> = nullarg);
template <typename Return, typename T>
Vc_INTRINSIC Vc_CONST Return
simd_cast(Scalar::Vector<T> x0, Scalar::Vector<T> x1,
          enable_if<std::is_same<Return, SSE::ushort_v>::value> = nullarg);

// 3 Scalar::Vector to 1 SSE::Vector {{{2
template <typename Return, typename T>
Vc_INTRINSIC Vc_CONST Return
simd_cast(Scalar::Vector<T> x0, Scalar::Vector<T> x1, Scalar::Vector<T> x3,
          enable_if<std::is_same<Return, SSE::float_v>::value> = nullarg);
template <typename Return, typename T>
Vc_INTRINSIC Vc_CONST Return
simd_cast(Scalar::Vector<T> x0, Scalar::Vector<T> x1, Scalar::Vector<T> x3,
          enable_if<std::is_same<Return, SSE::int_v>::value> = nullarg);
template <typename Return, typename T>
Vc_INTRINSIC Vc_CONST Return
simd_cast(Scalar::Vector<T> x0, Scalar::Vector<T> x1, Scalar::Vector<T> x3,
          enable_if<std::is_same<Return, SSE::uint_v>::value> = nullarg);
template <typename Return, typename T>
Vc_INTRINSIC Vc_CONST Return
simd_cast(Scalar::Vector<T> x0, Scalar::Vector<T> x1, Scalar::Vector<T> x3,
          enable_if<std::is_same<Return, SSE::short_v>::value> = nullarg);
template <typename Return, typename T>
Vc_INTRINSIC Vc_CONST Return
simd_cast(Scalar::Vector<T> x0, Scalar::Vector<T> x1, Scalar::Vector<T> x3,
          enable_if<std::is_same<Return, SSE::ushort_v>::value> = nullarg);

// 4 Scalar::Vector to 1 SSE::Vector {{{2
template <typename Return, typename T>
Vc_INTRINSIC Vc_CONST Return
simd_cast(Scalar::Vector<T> x0, Scalar::Vector<T> x1, Scalar::Vector<T> x2,
          Scalar::Vector<T> x3,
          enable_if<std::is_same<Return, SSE::float_v>::value> = nullarg);
template <typename Return, typename T>
Vc_INTRINSIC Vc_CONST Return
simd_cast(Scalar::Vector<T> x0, Scalar::Vector<T> x1, Scalar::Vector<T> x2,
          Scalar::Vector<T> x3,
          enable_if<std::is_same<Return, SSE::int_v>::value> = nullarg);
template <typename Return, typename T>
Vc_INTRINSIC Vc_CONST Return
simd_cast(Scalar::Vector<T> x0, Scalar::Vector<T> x1, Scalar::Vector<T> x2,
          Scalar::Vector<T> x3,
          enable_if<std::is_same<Return, SSE::uint_v>::value> = nullarg);
template <typename Return, typename T>
Vc_INTRINSIC Vc_CONST Return
simd_cast(Scalar::Vector<T> x0, Scalar::Vector<T> x1, Scalar::Vector<T> x2,
          Scalar::Vector<T> x3,
          enable_if<std::is_same<Return, SSE::short_v>::value> = nullarg);
template <typename Return, typename T>
Vc_INTRINSIC Vc_CONST Return
simd_cast(Scalar::Vector<T> x0, Scalar::Vector<T> x1, Scalar::Vector<T> x2,
          Scalar::Vector<T> x3,
          enable_if<std::is_same<Return, SSE::ushort_v>::value> = nullarg);

// 5 Scalar::Vector to 1 SSE::Vector {{{2
template <typename Return, typename T>
Vc_INTRINSIC Vc_CONST Return
simd_cast(Scalar::Vector<T> x0, Scalar::Vector<T> x1, Scalar::Vector<T> x2,
          Scalar::Vector<T> x3, Scalar::Vector<T> x4,
          enable_if<std::is_same<Return, SSE::short_v>::value> = nullarg);
template <typename Return, typename T>
Vc_INTRINSIC Vc_CONST Return
simd_cast(Scalar::Vector<T> x0, Scalar::Vector<T> x1, Scalar::Vector<T> x2,
          Scalar::Vector<T> x3, Scalar::Vector<T> x4,
          enable_if<std::is_same<Return, SSE::ushort_v>::value> = nullarg);

// 6 Scalar::Vector to 1 SSE::Vector {{{2
template <typename Return, typename T>
Vc_INTRINSIC Vc_CONST Return
simd_cast(Scalar::Vector<T> x0, Scalar::Vector<T> x1, Scalar::Vector<T> x2,
          Scalar::Vector<T> x3, Scalar::Vector<T> x4, Scalar::Vector<T> x5,
          enable_if<std::is_same<Return, SSE::short_v>::value> = nullarg);
template <typename Return, typename T>
Vc_INTRINSIC Vc_CONST Return
simd_cast(Scalar::Vector<T> x0, Scalar::Vector<T> x1, Scalar::Vector<T> x2,
          Scalar::Vector<T> x3, Scalar::Vector<T> x4, Scalar::Vector<T> x5,
          enable_if<std::is_same<Return, SSE::ushort_v>::value> = nullarg);

// 7 Scalar::Vector to 1 SSE::Vector {{{2
template <typename Return, typename T>
Vc_INTRINSIC Vc_CONST Return
simd_cast(Scalar::Vector<T> x0, Scalar::Vector<T> x1, Scalar::Vector<T> x2,
          Scalar::Vector<T> x3, Scalar::Vector<T> x4, Scalar::Vector<T> x5,
          Scalar::Vector<T> x6,
          enable_if<std::is_same<Return, SSE::short_v>::value> = nullarg);
template <typename Return, typename T>
Vc_INTRINSIC Vc_CONST Return
simd_cast(Scalar::Vector<T> x0, Scalar::Vector<T> x1, Scalar::Vector<T> x2,
          Scalar::Vector<T> x3, Scalar::Vector<T> x4, Scalar::Vector<T> x5,
          Scalar::Vector<T> x6,
          enable_if<std::is_same<Return, SSE::ushort_v>::value> = nullarg);

// 8 Scalar::Vector to 1 SSE::Vector {{{2
template <typename Return, typename T>
Vc_INTRINSIC Vc_CONST Return
simd_cast(Scalar::Vector<T> x0, Scalar::Vector<T> x1, Scalar::Vector<T> x2,
          Scalar::Vector<T> x3, Scalar::Vector<T> x4, Scalar::Vector<T> x5,
          Scalar::Vector<T> x6, Scalar::Vector<T> x7,
          enable_if<std::is_same<Return, SSE::short_v>::value> = nullarg);
template <typename Return, typename T>
Vc_INTRINSIC Vc_CONST Return
simd_cast(Scalar::Vector<T> x0, Scalar::Vector<T> x1, Scalar::Vector<T> x2,
          Scalar::Vector<T> x3, Scalar::Vector<T> x4, Scalar::Vector<T> x5,
          Scalar::Vector<T> x6, Scalar::Vector<T> x7,
          enable_if<std::is_same<Return, SSE::ushort_v>::value> = nullarg);

// SSE::Vector to Scalar::Vector {{{2
template <typename To, typename FromT>
Vc_INTRINSIC Vc_CONST To
simd_cast(SSE::Vector<FromT> x, enable_if<Scalar::is_vector<To>::value> = nullarg);

// helper macros Vc_SIMD_CAST_[1248] {{{1
#undef Vc_SIMD_CAST_1
#undef Vc_SIMD_CAST_2
#undef Vc_SIMD_CAST_4
#undef Vc_SIMD_CAST_8
#define Vc_SIMD_CAST_1(from_, to_)                                                       \
    template <typename To>                                                               \
    Vc_INTRINSIC Vc_CONST To simd_cast(from_ x, enable_if<std::is_same<To, to_>::value>)

#define Vc_SIMD_CAST_2(from_, to_)                                                       \
    template <typename To>                                                               \
    Vc_INTRINSIC Vc_CONST To simd_cast(from_ x0, from_ x1,                               \
                                       enable_if<std::is_same<To, to_>::value>)

#define Vc_SIMD_CAST_4(from_, to_)                                                       \
    template <typename To>                                                               \
    Vc_INTRINSIC Vc_CONST To simd_cast(from_ x0, from_ x1, from_ x2, from_ x3,           \
                                       enable_if<std::is_same<To, to_>::value>)

#define Vc_SIMD_CAST_8(from_, to_)                                                       \
    template <typename To>                                                               \
    Vc_INTRINSIC Vc_CONST To simd_cast(from_ x0, from_ x1, from_ x2, from_ x3, from_ x4, \
                                       from_ x5, from_ x6, from_ x7,                     \
                                       enable_if<std::is_same<To, to_>::value>)

// Vector casts without offset {{{1
namespace SSE
{
// helper functions {{{2
Vc_INTRINSIC __m128i convert_int32_to_int16(__m128i a, __m128i b)
{
    auto tmp0 = _mm_unpacklo_epi16(a, b);        // 0 4 X X 1 5 X X
    auto tmp1 = _mm_unpackhi_epi16(a, b);        // 2 6 X X 3 7 X X
    auto tmp2 = _mm_unpacklo_epi16(tmp0, tmp1);  // 0 2 4 6 X X X X
    auto tmp3 = _mm_unpackhi_epi16(tmp0, tmp1);  // 1 3 5 7 X X X X
    return _mm_unpacklo_epi16(tmp2, tmp3);       // 0 1 2 3 4 5 6 7
}

// 1 SSE::Vector to 1 SSE::Vector {{{2
// to int_v {{{3
Vc_SIMD_CAST_1( float_v,    int_v) { return convert< float, int>(x.data()); }
Vc_SIMD_CAST_1(double_v,    int_v) { return convert<double, int>(x.data()); }
Vc_SIMD_CAST_1(  uint_v,    int_v) { return convert<  uint, int>(x.data()); }
Vc_SIMD_CAST_1( short_v,    int_v) { return convert< short, int>(x.data()); }
Vc_SIMD_CAST_1(ushort_v,    int_v) { return convert<ushort, int>(x.data()); }
// to uint_v {{{3
Vc_SIMD_CAST_1( float_v,   uint_v) { return convert< float, uint>(x.data()); }
Vc_SIMD_CAST_1(double_v,   uint_v) { return convert<double, uint>(x.data()); }
Vc_SIMD_CAST_1(   int_v,   uint_v) { return convert<   int, uint>(x.data()); }
Vc_SIMD_CAST_1( short_v,   uint_v) { return convert< short, uint>(x.data()); }
Vc_SIMD_CAST_1(ushort_v,   uint_v) { return convert<ushort, uint>(x.data()); }
// to float_v {{{3
Vc_SIMD_CAST_1(double_v,  float_v) { return convert<double, float>(x.data()); }
Vc_SIMD_CAST_1(   int_v,  float_v) { return convert<   int, float>(x.data()); }
Vc_SIMD_CAST_1(  uint_v,  float_v) { return convert<  uint, float>(x.data()); }
Vc_SIMD_CAST_1( short_v,  float_v) { return convert< short, float>(x.data()); }
Vc_SIMD_CAST_1(ushort_v,  float_v) { return convert<ushort, float>(x.data()); }
// to double_v {{{3
Vc_SIMD_CAST_1( float_v, double_v) { return convert< float, double>(x.data()); }
Vc_SIMD_CAST_1(   int_v, double_v) { return convert<   int, double>(x.data()); }
Vc_SIMD_CAST_1(  uint_v, double_v) { return convert<  uint, double>(x.data()); }
Vc_SIMD_CAST_1( short_v, double_v) { return convert< short, double>(x.data()); }
Vc_SIMD_CAST_1(ushort_v, double_v) { return convert<ushort, double>(x.data()); }
// to short_v {{{3
/*
 * §4.7 p3 (integral conversions)
 *  If the destination type is signed, the value is unchanged if it can be represented in the
 *  destination type (and bit-field width); otherwise, the value is implementation-defined.
 *
 * See also below for the Vc_SIMD_CAST_2
 *
 * the alternative, which is probably incorrect for all compilers out there:
    Vc_SIMD_CAST_1(   int_v,  short_v) { return _mm_packs_epi32(x.data(), _mm_setzero_si128()); }
    Vc_SIMD_CAST_1(  uint_v,  short_v) { return _mm_packs_epi32(x.data(), _mm_setzero_si128()); }
    Vc_SIMD_CAST_2(   int_v,  short_v) { return _mm_packs_epi32(x0.data(), x1.data()); }
    Vc_SIMD_CAST_2(  uint_v,  short_v) { return _mm_packs_epi32(x0.data(), x1.data()); }
 */
Vc_SIMD_CAST_1(   int_v,  short_v) { return SSE::convert_int32_to_int16(x.data(), _mm_setzero_si128()); }
Vc_SIMD_CAST_1(  uint_v,  short_v) { return SSE::convert_int32_to_int16(x.data(), _mm_setzero_si128()); }
Vc_SIMD_CAST_1( float_v,  short_v) { return _mm_packs_epi32(simd_cast<SSE::int_v>(x).data(), _mm_setzero_si128()); }
Vc_SIMD_CAST_1(double_v,  short_v) { return _mm_packs_epi32(simd_cast<SSE::int_v>(x).data(), _mm_setzero_si128()); }
Vc_SIMD_CAST_1(ushort_v,  short_v) { return x.data(); }
// to ushort_v {{{3
Vc_SIMD_CAST_1(   int_v, ushort_v) { return SSE::convert_int32_to_int16(x.data(), _mm_setzero_si128()); }
Vc_SIMD_CAST_1(  uint_v, ushort_v) { return SSE::convert_int32_to_int16(x.data(), _mm_setzero_si128()); }
Vc_SIMD_CAST_1( float_v, ushort_v) { return simd_cast<SSE::ushort_v>(simd_cast<SSE::int_v>(x)); }
Vc_SIMD_CAST_1(double_v, ushort_v) { return simd_cast<SSE::ushort_v>(simd_cast<SSE::int_v>(x)); }
Vc_SIMD_CAST_1( short_v, ushort_v) { return x.data(); }
// 2 SSE::Vector to 1 SSE::Vector {{{2
Vc_SIMD_CAST_2(double_v,    int_v) {
#ifdef Vc_IMPL_AVX
    return AVX::convert<double, int>(AVX::concat(x0.data(), x1.data()));
#else
    return _mm_unpacklo_epi64(convert<double, int>(x0.data()), convert<double, int>(x1.data()));
#endif
}
Vc_SIMD_CAST_2(double_v,   uint_v) {
#ifdef Vc_IMPL_AVX
    return AVX::convert<double, uint>(AVX::concat(x0.data(), x1.data()));
#else
    return _mm_unpacklo_epi64(convert<double, uint>(x0.data()), convert<double, uint>(x1.data()));
#endif
}
Vc_SIMD_CAST_2(double_v,  float_v) {
#ifdef Vc_IMPL_AVX
    return _mm256_cvtpd_ps(AVX::concat(x0.data(), x1.data()));
#else
    return _mm_movelh_ps(_mm_cvtpd_ps(x0.data()), _mm_cvtpd_ps(x1.data()));
#endif
}

Vc_SIMD_CAST_2(   int_v,  short_v) { return SSE::convert_int32_to_int16(x0.data(), x1.data()); }
Vc_SIMD_CAST_2(  uint_v,  short_v) { return SSE::convert_int32_to_int16(x0.data(), x1.data()); }
Vc_SIMD_CAST_2( float_v,  short_v) { return _mm_packs_epi32(simd_cast<SSE::int_v>(x0).data(), simd_cast<SSE::int_v>(x1).data()); }
Vc_SIMD_CAST_2(double_v,  short_v) { return _mm_packs_epi32(simd_cast<SSE::int_v>(x0, x1).data(), _mm_setzero_si128()); }

Vc_SIMD_CAST_2(   int_v, ushort_v) { return SSE::convert_int32_to_int16(x0.data(), x1.data()); }
Vc_SIMD_CAST_2(  uint_v, ushort_v) { return SSE::convert_int32_to_int16(x0.data(), x1.data()); }
Vc_SIMD_CAST_2( float_v, ushort_v) { return simd_cast<SSE::ushort_v>(simd_cast<SSE::int_v>(x0), simd_cast<SSE::int_v>(x1)); }
Vc_SIMD_CAST_2(double_v, ushort_v) { return simd_cast<SSE::ushort_v>(simd_cast<SSE::int_v>(x0, x1)); }

// 3 SSE::Vector to 1 SSE::Vector {{{2
Vc_CAST_(short_v) simd_cast(double_v a, double_v b, double_v c)
{
    return simd_cast<short_v>(simd_cast<int_v>(a, b), simd_cast<int_v>(c));
}
Vc_CAST_(ushort_v) simd_cast(double_v a, double_v b, double_v c)
{
    return simd_cast<ushort_v>(simd_cast<int_v>(a, b), simd_cast<int_v>(c));
}
#undef Vc_CAST_

// 4 SSE::Vector to 1 SSE::Vector {{{2
Vc_SIMD_CAST_4(double_v,  short_v) { return _mm_packs_epi32(simd_cast<SSE::int_v>(x0, x1).data(), simd_cast<SSE::int_v>(x2, x3).data()); }
Vc_SIMD_CAST_4(double_v, ushort_v) { return simd_cast<SSE::ushort_v>(simd_cast<SSE::int_v>(x0, x1), simd_cast<SSE::int_v>(x2, x3)); }
}  // namespace SSE

// 1 Scalar::Vector to 1 SSE::Vector {{{2
template <typename Return, typename T>
Vc_INTRINSIC Vc_CONST Return
    simd_cast(Scalar::Vector<T> x,
              enable_if<std::is_same<Return, SSE::double_v>::value> )
{
    return _mm_setr_pd(x.data(), 0.);  // FIXME: use register-register mov
}
template <typename Return, typename T>
Vc_INTRINSIC Vc_CONST Return
    simd_cast(Scalar::Vector<T> x,
              enable_if<std::is_same<Return, SSE::float_v>::value> )
{
    return _mm_setr_ps(x.data(), 0.f, 0.f, 0.f);  // FIXME: use register-register mov
}
template <typename Return, typename T>
Vc_INTRINSIC Vc_CONST Return
    simd_cast(Scalar::Vector<T> x,
              enable_if<std::is_same<Return, SSE::int_v>::value> )
{
    return _mm_setr_epi32(x.data(), 0, 0, 0);  // FIXME: use register-register mov
}
template <typename Return, typename T>
Vc_INTRINSIC Vc_CONST Return
    simd_cast(Scalar::Vector<T> x,
              enable_if<std::is_same<Return, SSE::uint_v>::value> )
{
    return _mm_setr_epi32(uint(x.data()), 0, 0, 0);  // FIXME: use register-register mov
}
template <typename Return, typename T>
Vc_INTRINSIC Vc_CONST Return
    simd_cast(Scalar::Vector<T> x,
              enable_if<std::is_same<Return, SSE::short_v>::value> )
{
    return _mm_setr_epi16(
        x.data(), 0, 0, 0, 0, 0, 0, 0);  // FIXME: use register-register mov
}
template <typename Return, typename T>
Vc_INTRINSIC Vc_CONST Return
    simd_cast(Scalar::Vector<T> x,
              enable_if<std::is_same<Return, SSE::ushort_v>::value> )
{
    return _mm_setr_epi16(
        x.data(), 0, 0, 0, 0, 0, 0, 0);  // FIXME: use register-register mov
}

// 2 Scalar::Vector to 1 SSE::Vector {{{2
template <typename Return, typename T>
Vc_INTRINSIC Vc_CONST Return
    simd_cast(Scalar::Vector<T> x0,
              Scalar::Vector<T> x1,
              enable_if<std::is_same<Return, SSE::double_v>::value> )
{
    return _mm_setr_pd(x0.data(), x1.data());  // FIXME: use register-register mov
}
template <typename Return, typename T>
Vc_INTRINSIC Vc_CONST Return
    simd_cast(Scalar::Vector<T> x0,
              Scalar::Vector<T> x1,
              enable_if<std::is_same<Return, SSE::float_v>::value> )
{
    return _mm_setr_ps(x0.data(), x1.data(), 0.f, 0.f);  // FIXME: use register-register mov
}
template <typename Return, typename T>
Vc_INTRINSIC Vc_CONST Return
    simd_cast(Scalar::Vector<T> x0,
              Scalar::Vector<T> x1,
              enable_if<std::is_same<Return, SSE::int_v>::value> )
{
    return _mm_setr_epi32(x0.data(), x1.data(), 0, 0);  // FIXME: use register-register mov
}
template <typename Return, typename T>
Vc_INTRINSIC Vc_CONST Return
    simd_cast(Scalar::Vector<T> x0,
              Scalar::Vector<T> x1,
              enable_if<std::is_same<Return, SSE::uint_v>::value> )
{
    return _mm_setr_epi32(uint(x0.data()), uint(x1.data()), 0,
                          0);  // FIXME: use register-register mov
}
template <typename Return, typename T>
Vc_INTRINSIC Vc_CONST Return
    simd_cast(Scalar::Vector<T> x0,
              Scalar::Vector<T> x1,
              enable_if<std::is_same<Return, SSE::short_v>::value> )
{
    return _mm_setr_epi16(
        x0.data(), x1.data(), 0, 0, 0, 0, 0, 0);  // FIXME: use register-register mov
}
template <typename Return, typename T>
Vc_INTRINSIC Vc_CONST Return
    simd_cast(Scalar::Vector<T> x0,
              Scalar::Vector<T> x1,
              enable_if<std::is_same<Return, SSE::ushort_v>::value> )
{
    return _mm_setr_epi16(
        x0.data(), x1.data(), 0, 0, 0, 0, 0, 0);  // FIXME: use register-register mov
}

// 3 Scalar::Vector to 1 SSE::Vector {{{2
template <typename Return, typename T>
Vc_INTRINSIC Vc_CONST Return
simd_cast(Scalar::Vector<T> x0, Scalar::Vector<T> x1, Scalar::Vector<T> x2,
          enable_if<std::is_same<Return, SSE::float_v>::value>)
{
    return _mm_setr_ps(x0.data(), x1.data(), x2.data(), 0.f);
}
template <typename Return, typename T>
Vc_INTRINSIC Vc_CONST Return
simd_cast(Scalar::Vector<T> x0, Scalar::Vector<T> x1, Scalar::Vector<T> x2,
          enable_if<std::is_same<Return, SSE::int_v>::value>)
{
    return _mm_setr_epi32(x0.data(), x1.data(), x2.data(), 0);
}
template <typename Return, typename T>
Vc_INTRINSIC Vc_CONST Return
simd_cast(Scalar::Vector<T> x0, Scalar::Vector<T> x1, Scalar::Vector<T> x2,
          enable_if<std::is_same<Return, SSE::uint_v>::value>)
{
    return _mm_setr_epi32(uint(x0.data()), uint(x1.data()), uint(x2.data()),
                          0);  // FIXME: use register-register mov
}
template <typename Return, typename T>
Vc_INTRINSIC Vc_CONST Return
simd_cast(Scalar::Vector<T> x0, Scalar::Vector<T> x1, Scalar::Vector<T> x2,
          enable_if<std::is_same<Return, SSE::short_v>::value>)
{
    return _mm_setr_epi16(x0.data(), x1.data(), x2.data(), 0, 0, 0, 0, 0);
}
template <typename Return, typename T>
Vc_INTRINSIC Vc_CONST Return
simd_cast(Scalar::Vector<T> x0, Scalar::Vector<T> x1, Scalar::Vector<T> x2,
          enable_if<std::is_same<Return, SSE::ushort_v>::value>)
{
    return _mm_setr_epi16(x0.data(), x1.data(), x2.data(), 0, 0, 0, 0, 0);
}

// 4 Scalar::Vector to 1 SSE::Vector {{{2
template <typename Return, typename T>
Vc_INTRINSIC Vc_CONST Return
    simd_cast(Scalar::Vector<T> x0,
              Scalar::Vector<T> x1,
              Scalar::Vector<T> x2,
              Scalar::Vector<T> x3,
              enable_if<std::is_same<Return, SSE::float_v>::value> )
{
    return _mm_setr_ps(
        x0.data(), x1.data(), x2.data(), x3.data());  // FIXME: use register-register mov
}
template <typename Return, typename T>
Vc_INTRINSIC Vc_CONST Return
    simd_cast(Scalar::Vector<T> x0,
              Scalar::Vector<T> x1,
              Scalar::Vector<T> x2,
              Scalar::Vector<T> x3,
              enable_if<std::is_same<Return, SSE::int_v>::value> )
{
    return _mm_setr_epi32(
        x0.data(), x1.data(), x2.data(), x3.data());  // FIXME: use register-register mov
}
template <typename Return, typename T>
Vc_INTRINSIC Vc_CONST Return
    simd_cast(Scalar::Vector<T> x0,
              Scalar::Vector<T> x1,
              Scalar::Vector<T> x2,
              Scalar::Vector<T> x3,
              enable_if<std::is_same<Return, SSE::uint_v>::value> )
{
    return _mm_setr_epi32(uint(x0.data()), uint(x1.data()), uint(x2.data()),
                          uint(x3.data()));  // FIXME: use register-register mov
}
template <typename Return, typename T>
Vc_INTRINSIC Vc_CONST Return
    simd_cast(Scalar::Vector<T> x0,
              Scalar::Vector<T> x1,
              Scalar::Vector<T> x2,
              Scalar::Vector<T> x3,
              enable_if<std::is_same<Return, SSE::short_v>::value> )
{
    return _mm_setr_epi16(
        x0.data(), x1.data(), x2.data(), x3.data(), 0, 0, 0, 0);  // FIXME: use register-register mov
}
template <typename Return, typename T>
Vc_INTRINSIC Vc_CONST Return
    simd_cast(Scalar::Vector<T> x0,
              Scalar::Vector<T> x1,
              Scalar::Vector<T> x2,
              Scalar::Vector<T> x3,
              enable_if<std::is_same<Return, SSE::ushort_v>::value> )
{
    return _mm_setr_epi16(
        x0.data(), x1.data(), x2.data(), x3.data(), 0, 0, 0, 0);  // FIXME: use register-register mov
}

// 5 Scalar::Vector to 1 SSE::Vector {{{2
template <typename Return, typename T>
Vc_INTRINSIC Vc_CONST Return
simd_cast(Scalar::Vector<T> x0, Scalar::Vector<T> x1, Scalar::Vector<T> x2,
          Scalar::Vector<T> x3, Scalar::Vector<T> x4,
          enable_if<std::is_same<Return, SSE::short_v>::value>)
{
    return _mm_setr_epi16(x0.data(), x1.data(), x2.data(), x3.data(), x4.data(), 0, 0, 0);
}
template <typename Return, typename T>
Vc_INTRINSIC Vc_CONST Return
simd_cast(Scalar::Vector<T> x0, Scalar::Vector<T> x1, Scalar::Vector<T> x2,
          Scalar::Vector<T> x3, Scalar::Vector<T> x4,
          enable_if<std::is_same<Return, SSE::ushort_v>::value>)
{
    return _mm_setr_epi16(x0.data(), x1.data(), x2.data(), x3.data(), x4.data(), 0, 0, 0);
}

// 6 Scalar::Vector to 1 SSE::Vector {{{2
template <typename Return, typename T>
Vc_INTRINSIC Vc_CONST Return
simd_cast(Scalar::Vector<T> x0, Scalar::Vector<T> x1, Scalar::Vector<T> x2,
          Scalar::Vector<T> x3, Scalar::Vector<T> x4, Scalar::Vector<T> x5,
          enable_if<std::is_same<Return, SSE::short_v>::value>)
{
    return _mm_setr_epi16(x0.data(), x1.data(), x2.data(), x3.data(), x4.data(),
                          x5.data(), 0, 0);
}
template <typename Return, typename T>
Vc_INTRINSIC Vc_CONST Return
simd_cast(Scalar::Vector<T> x0, Scalar::Vector<T> x1, Scalar::Vector<T> x2,
          Scalar::Vector<T> x3, Scalar::Vector<T> x4, Scalar::Vector<T> x5,
          enable_if<std::is_same<Return, SSE::ushort_v>::value>)
{
    return _mm_setr_epi16(x0.data(), x1.data(), x2.data(), x3.data(), x4.data(),
                          x5.data(), 0, 0);
}

// 7 Scalar::Vector to 1 SSE::Vector {{{2
template <typename Return, typename T>
Vc_INTRINSIC Vc_CONST Return
simd_cast(Scalar::Vector<T> x0, Scalar::Vector<T> x1, Scalar::Vector<T> x2,
          Scalar::Vector<T> x3, Scalar::Vector<T> x4, Scalar::Vector<T> x5,
          Scalar::Vector<T> x6, enable_if<std::is_same<Return, SSE::short_v>::value>)
{
    return _mm_setr_epi16(x0.data(), x1.data(), x2.data(), x3.data(), x4.data(),
                          x5.data(), x6.data(), 0);
}
template <typename Return, typename T>
Vc_INTRINSIC Vc_CONST Return
simd_cast(Scalar::Vector<T> x0, Scalar::Vector<T> x1, Scalar::Vector<T> x2,
          Scalar::Vector<T> x3, Scalar::Vector<T> x4, Scalar::Vector<T> x5,
          Scalar::Vector<T> x6, enable_if<std::is_same<Return, SSE::ushort_v>::value>)
{
    return _mm_setr_epi16(x0.data(), x1.data(), x2.data(), x3.data(), x4.data(),
                          x5.data(), x6.data(), 0);
}

// 8 Scalar::Vector to 1 SSE::Vector {{{2
template <typename Return, typename T>
Vc_INTRINSIC Vc_CONST Return
    simd_cast(Scalar::Vector<T> x0,
              Scalar::Vector<T> x1,
              Scalar::Vector<T> x2,
              Scalar::Vector<T> x3,
              Scalar::Vector<T> x4,
              Scalar::Vector<T> x5,
              Scalar::Vector<T> x6,
              Scalar::Vector<T> x7,
              enable_if<std::is_same<Return, SSE::short_v>::value> )
{
    return _mm_setr_epi16(x0.data(),
                          x1.data(),
                          x2.data(),
                          x3.data(),
                          x4.data(),
                          x5.data(),
                          x6.data(),
                          x7.data());  // FIXME: use register-register mov
}
template <typename Return, typename T>
Vc_INTRINSIC Vc_CONST Return
    simd_cast(Scalar::Vector<T> x0,
              Scalar::Vector<T> x1,
              Scalar::Vector<T> x2,
              Scalar::Vector<T> x3,
              Scalar::Vector<T> x4,
              Scalar::Vector<T> x5,
              Scalar::Vector<T> x6,
              Scalar::Vector<T> x7,
              enable_if<std::is_same<Return, SSE::ushort_v>::value> )
{
    return _mm_setr_epi16(x0.data(),
                          x1.data(),
                          x2.data(),
                          x3.data(),
                          x4.data(),
                          x5.data(),
                          x6.data(),
                          x7.data());  // FIXME: use register-register mov
}

// SSE::Vector to Scalar::Vector {{{2
template <typename To, typename FromT>
Vc_INTRINSIC Vc_CONST To
    simd_cast(SSE::Vector<FromT> x, enable_if<Scalar::is_vector<To>::value> )
{
    return static_cast<To>(x[0]);
}

// Mask casts without offset {{{1
// 1 SSE Mask to 1 SSE Mask {{{2
template <typename Return, typename T>
Vc_INTRINSIC Vc_CONST Return
    simd_cast(SSE::Mask<T> x, enable_if<SSE::is_mask<Return>::value> = nullarg)
{
    using M = SSE::Mask<T>;
    return {Detail::mask_cast<M::Size, Return::Size, __m128>(x.dataI())};
}
// 2 SSE Masks to 1 SSE Mask {{{2
template <typename Return, typename T>
Vc_INTRINSIC Vc_CONST Return simd_cast(
    SSE::Mask<T> x0,
    SSE::Mask<T> x1,
    enable_if<SSE::is_mask<Return>::value && Mask<T, VectorAbi::Sse>::Size * 2 == Return::Size> = nullarg)
{
    return SSE::sse_cast<__m128>(_mm_packs_epi16(x0.dataI(), x1.dataI()));
}
template <typename Return, typename T>
Vc_INTRINSIC Vc_CONST Return simd_cast(
    SSE::Mask<T> x0,
    SSE::Mask<T> x1,
    enable_if<SSE::is_mask<Return>::value && Mask<T, VectorAbi::Sse>::Size * 4 == Return::Size> = nullarg)
{
    return SSE::sse_cast<__m128>(
        _mm_packs_epi16(_mm_packs_epi16(x0.dataI(), x1.dataI()), _mm_setzero_si128()));
}
// 4 SSE Masks to 1 SSE Mask {{{2
template <typename Return, typename T>
Vc_INTRINSIC Vc_CONST Return simd_cast(
    SSE::Mask<T> x0,
    SSE::Mask<T> x1,
    SSE::Mask<T> x2,
    SSE::Mask<T> x3,
    enable_if<SSE::is_mask<Return>::value && Mask<T, VectorAbi::Sse>::Size * 4 == Return::Size> = nullarg)
{
    return SSE::sse_cast<__m128>(_mm_packs_epi16(_mm_packs_epi16(x0.dataI(), x1.dataI()),
                                                 _mm_packs_epi16(x2.dataI(), x3.dataI())));
}

// 1 Scalar Mask to 1 SSE Mask {{{2
template <typename Return, typename T>
Vc_INTRINSIC Vc_CONST Return
    simd_cast(Scalar::Mask<T> x, enable_if<SSE::is_mask<Return>::value> = nullarg)
{
    Return m(false);
    m[0] = x[0];
    return m;
}
// 2 Scalar Masks to 1 SSE Mask {{{2
template <typename Return, typename T>
Vc_INTRINSIC Vc_CONST Return
    simd_cast(Scalar::Mask<T> x0, Scalar::Mask<T> x1, enable_if<SSE::is_mask<Return>::value> = nullarg)
{
    Return m(false);
    m[0] = x0[0];
    m[1] = x1[0];
    return m;
}
// 4 Scalar Masks to 1 SSE Mask {{{2
template <typename Return, typename T>
Vc_INTRINSIC Vc_CONST Return simd_cast(Scalar::Mask<T> x0,
                                       Scalar::Mask<T> x1,
                                       Scalar::Mask<T> x2,
                                       Scalar::Mask<T> x3,
                                       enable_if<SSE::is_mask<Return>::value> = nullarg)
{
    Return m(false);
    m[0] = x0[0];
    m[1] = x1[0];
    if (Return::Size >= 4) {
        m[2] = x2[0];
        m[3] = x3[0];
    }
    return m;
}
// 8 Scalar Masks to 1 SSE Mask {{{2
template <typename Return, typename T>
Vc_INTRINSIC Vc_CONST Return simd_cast(Scalar::Mask<T> x0,
                                       Scalar::Mask<T> x1,
                                       Scalar::Mask<T> x2,
                                       Scalar::Mask<T> x3,
                                       Scalar::Mask<T> x4,
                                       Scalar::Mask<T> x5,
                                       Scalar::Mask<T> x6,
                                       Scalar::Mask<T> x7,
                                       enable_if<SSE::is_mask<Return>::value> = nullarg)
{
    Return m(false);
    m[0] = x0[0];
    m[1] = x1[0];
    if (Return::Size >= 4) {
        m[2] = x2[0];
        m[3] = x3[0];
    }
    if (Return::Size >= 8) {
        m[4] = x4[0];
        m[5] = x5[0];
        m[6] = x6[0];
        m[7] = x7[0];
    }
    return m;
}

// 1 SSE::Mask to 1 Scalar::Mask {{{2
template <typename To, typename FromT>
Vc_INTRINSIC Vc_CONST To
    simd_cast(SSE::Mask<FromT> x, enable_if<Scalar::is_mask<To>::value> = nullarg)
{
    return static_cast<To>(x[0]);
}
// offset == 0 | convert from SSE::Mask/Vector to SSE::Mask/Vector {{{1
template <typename Return, int offset, typename V>
Vc_INTRINSIC Vc_CONST Return
    simd_cast(V &&x, enable_if<offset == 0 && ((SSE::is_vector<Traits::decay<V>>::value &&
                                                SSE::is_vector<Return>::value) ||
                                               (SSE::is_mask<Traits::decay<V>>::value &&
                                                SSE::is_mask<Return>::value))> = nullarg)
{
    return simd_cast<Return>(x);
}

template <typename Return, int offset, typename V>
Vc_INTRINSIC Vc_CONST Return
    simd_cast(V &&x,
              enable_if<offset == 0 && ((Scalar::is_vector<Traits::decay<V>>::value &&
                                         SSE::is_vector<Return>::value) ||
                                        (Scalar::is_mask<Traits::decay<V>>::value &&
                                         SSE::is_mask<Return>::value))> = nullarg)
{
    return simd_cast<Return>(x);
}

// Vector casts with offset {{{1
// SSE to SSE (Vector) {{{2
template <typename Return, int offset, typename V>
Vc_INTRINSIC Vc_CONST Return simd_cast(
    V x,
    enable_if<offset != 0 && (SSE::is_vector<Return>::value && SSE::is_vector<V>::value)> = nullarg)
{
    constexpr int shift = (sizeof(V) / V::Size) * offset * Return::Size;
    static_assert(shift > 0 && shift < 16, "");
    return simd_cast<Return>(V{SSE::sse_cast<typename V::VectorType>(
        _mm_srli_si128(SSE::sse_cast<__m128i>(x.data()), shift & 0xff))});
}

// SSE to Scalar (Vector) {{{2
template <typename Return, int offset, typename T>
Vc_INTRINSIC Vc_CONST Return
    simd_cast(SSE::Vector<T> x,
              enable_if<offset != 0 && Scalar::is_vector<Return>::value> = nullarg)
{
    return static_cast<typename Return::EntryType>(x[offset]);
}

// Mask casts with offset {{{1
// SSE to SSE (Mask)
template <typename Return, int offset, typename V>
Vc_INTRINSIC Vc_CONST Return simd_cast(
    V x,
    enable_if<offset != 0 && (SSE::is_mask<Return>::value && SSE::is_mask<V>::value)> = nullarg)
{
    constexpr int shift = (sizeof(V) / V::Size) * offset * Return::Size;
    static_assert(shift > 0 && shift < 16, "");
    return simd_cast<Return>(V{SSE::sse_cast<typename V::VectorType>(
        _mm_srli_si128(SSE::sse_cast<__m128i>(x.data()), shift & 0xff))});
}

// undef Vc_SIMD_CAST_[1248] {{{1
#undef Vc_SIMD_CAST_1
#undef Vc_SIMD_CAST_2
#undef Vc_SIMD_CAST_4
#undef Vc_SIMD_CAST_8
// }}}1

}  // namespace Vc

#endif // VC_SSE_SIMD_CAST_H_

// vim: foldmethod=marker
