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

#ifndef VC_AVX_MATH_H_
#define VC_AVX_MATH_H_

#include "const.h"
#include "limits.h"
#include "macros.h"

namespace Vc_VERSIONED_NAMESPACE
{
// min & max {{{1
#ifdef Vc_IMPL_AVX2
Vc_ALWAYS_INLINE AVX2::int_v    min(const AVX2::int_v    &x, const AVX2::int_v    &y) { return _mm256_min_epi32(x.data(), y.data()); }
Vc_ALWAYS_INLINE AVX2::uint_v   min(const AVX2::uint_v   &x, const AVX2::uint_v   &y) { return _mm256_min_epu32(x.data(), y.data()); }
Vc_ALWAYS_INLINE AVX2::short_v  min(const AVX2::short_v  &x, const AVX2::short_v  &y) { return _mm256_min_epi16(x.data(), y.data()); }
Vc_ALWAYS_INLINE AVX2::ushort_v min(const AVX2::ushort_v &x, const AVX2::ushort_v &y) { return _mm256_min_epu16(x.data(), y.data()); }
Vc_ALWAYS_INLINE AVX2::int_v    max(const AVX2::int_v    &x, const AVX2::int_v    &y) { return _mm256_max_epi32(x.data(), y.data()); }
Vc_ALWAYS_INLINE AVX2::uint_v   max(const AVX2::uint_v   &x, const AVX2::uint_v   &y) { return _mm256_max_epu32(x.data(), y.data()); }
Vc_ALWAYS_INLINE AVX2::short_v  max(const AVX2::short_v  &x, const AVX2::short_v  &y) { return _mm256_max_epi16(x.data(), y.data()); }
Vc_ALWAYS_INLINE AVX2::ushort_v max(const AVX2::ushort_v &x, const AVX2::ushort_v &y) { return _mm256_max_epu16(x.data(), y.data()); }
#endif
Vc_ALWAYS_INLINE AVX2::float_v  min(const AVX2::float_v  &x, const AVX2::float_v  &y) { return _mm256_min_ps(x.data(), y.data()); }
Vc_ALWAYS_INLINE AVX2::double_v min(const AVX2::double_v &x, const AVX2::double_v &y) { return _mm256_min_pd(x.data(), y.data()); }
Vc_ALWAYS_INLINE AVX2::float_v  max(const AVX2::float_v  &x, const AVX2::float_v  &y) { return _mm256_max_ps(x.data(), y.data()); }
Vc_ALWAYS_INLINE AVX2::double_v max(const AVX2::double_v &x, const AVX2::double_v &y) { return _mm256_max_pd(x.data(), y.data()); }

// sqrt {{{1
template <typename T>
Vc_ALWAYS_INLINE Vc_PURE AVX2::Vector<T> sqrt(const AVX2::Vector<T> &x)
{
    return AVX::VectorHelper<T>::sqrt(x.data());
}

// rsqrt {{{1
template <typename T>
Vc_ALWAYS_INLINE Vc_PURE AVX2::Vector<T> rsqrt(const AVX2::Vector<T> &x)
{
    return AVX::VectorHelper<T>::rsqrt(x.data());
}

// reciprocal {{{1
template <typename T>
Vc_ALWAYS_INLINE Vc_PURE AVX2::Vector<T> reciprocal(const AVX2::Vector<T> &x)
{
    return AVX::VectorHelper<T>::reciprocal(x.data());
}

// round {{{1
template <typename T>
Vc_ALWAYS_INLINE Vc_PURE AVX2::Vector<T> round(const AVX2::Vector<T> &x)
{
    return AVX::VectorHelper<T>::round(x.data());
}

// abs {{{1
Vc_INTRINSIC Vc_CONST AVX2::double_v abs(AVX2::double_v x)
{
    return Detail::and_(x.data(), AVX::setabsmask_pd());
}
Vc_INTRINSIC Vc_CONST AVX2::float_v abs(AVX2::float_v x)
{
    return Detail::and_(x.data(), AVX::setabsmask_ps());
}
#ifdef Vc_IMPL_AVX2
Vc_INTRINSIC Vc_CONST AVX2::int_v abs(AVX2::int_v x)
{
    return _mm256_abs_epi32(x.data());
}
Vc_INTRINSIC Vc_CONST AVX2::short_v abs(AVX2::short_v x)
{
    return _mm256_abs_epi16(x.data());
}
#endif

// isfinite {{{1
Vc_ALWAYS_INLINE Vc_PURE AVX2::double_m isfinite(const AVX2::double_v &x)
{
    return AVX::cmpord_pd(x.data(), _mm256_mul_pd(Detail::zero<__m256d>(), x.data()));
}

Vc_ALWAYS_INLINE Vc_PURE AVX2::float_m isfinite(const AVX2::float_v &x)
{
    return AVX::cmpord_ps(x.data(), _mm256_mul_ps(Detail::zero<__m256>(), x.data()));
}

// isinf {{{1
Vc_ALWAYS_INLINE Vc_PURE AVX2::double_m isinf(const AVX2::double_v &x)
{
    return _mm256_castsi256_pd(AVX::cmpeq_epi64(
        _mm256_castpd_si256(abs(x).data()),
        _mm256_castpd_si256(Detail::avx_broadcast(AVX::c_log<double>::d(1)))));
}

Vc_ALWAYS_INLINE Vc_PURE AVX2::float_m isinf(const AVX2::float_v &x)
{
    return _mm256_castsi256_ps(
        AVX::cmpeq_epi32(_mm256_castps_si256(abs(x).data()),
                         _mm256_castps_si256(Detail::avx_broadcast(AVX::c_log<float>::d(1)))));
}

// isnan {{{1
Vc_ALWAYS_INLINE Vc_PURE AVX2::double_m isnan(const AVX2::double_v &x)
{
    return AVX::cmpunord_pd(x.data(), x.data());
}

Vc_ALWAYS_INLINE Vc_PURE AVX2::float_m isnan(const AVX2::float_v &x)
{
    return AVX::cmpunord_ps(x.data(), x.data());
}

// copysign {{{1
Vc_INTRINSIC Vc_CONST AVX2::float_v copysign(AVX2::float_v mag, AVX2::float_v sign)
{
    return _mm256_or_ps(_mm256_and_ps(sign.data(), AVX::setsignmask_ps()),
                        _mm256_and_ps(mag.data(), AVX::setabsmask_ps()));
}
Vc_INTRINSIC Vc_CONST AVX2::double_v copysign(AVX2::double_v::AsArg mag,
                                              AVX2::double_v::AsArg sign)
{
    return _mm256_or_pd(_mm256_and_pd(sign.data(), AVX::setsignmask_pd()),
                        _mm256_and_pd(mag.data(), AVX::setabsmask_pd()));
}

//}}}1
// frexp {{{1
/**
 * splits \p v into exponent and mantissa, the sign is kept with the mantissa
 *
 * The return value will be in the range [0.5, 1.0[
 * The \p e value will be an integer defining the power-of-two exponent
 */
inline AVX2::double_v frexp(AVX2::double_v::AsArg v, SimdArray<int, 4> *e)
{
    const __m256d exponentBits = AVX::Const<double>::exponentMask().dataD();
    const __m256d exponentPart = _mm256_and_pd(v.data(), exponentBits);
    auto lo = AVX::avx_cast<__m128i>(AVX::lo128(exponentPart));
    auto hi = AVX::avx_cast<__m128i>(AVX::hi128(exponentPart));
    lo = _mm_sub_epi32(_mm_srli_epi64(lo, 52), _mm_set1_epi64x(0x3fe));
    hi = _mm_sub_epi32(_mm_srli_epi64(hi, 52), _mm_set1_epi64x(0x3fe));
    SSE::int_v exponent = Mem::shuffle<X0, X2, Y0, Y2>(lo, hi);
    const __m256d exponentMaximized = _mm256_or_pd(v.data(), exponentBits);
    AVX2::double_v ret =
        _mm256_and_pd(exponentMaximized,
                      _mm256_broadcast_sd(reinterpret_cast<const double *>(&AVX::c_general::frexpMask)));
    const double_m zeroMask = v == AVX2::double_v::Zero();
    ret(isnan(v) || !isfinite(v) || zeroMask) = v;
    exponent.setZero(simd_cast<SSE::int_m>(zeroMask));
    internal_data(*e) = exponent;
    return ret;
}

#ifdef Vc_IMPL_AVX2
inline SimdArray<double, 8> frexp(const SimdArray<double, 8> &v, SimdArray<int, 8> *e)
{
    const __m256d exponentBits = AVX::Const<double>::exponentMask().dataD();
    const __m256d w[2] = {internal_data(internal_data0(v)).data(),
                          internal_data(internal_data1(v)).data()};
    const __m256i exponentPart[2] = {
        _mm256_castpd_si256(_mm256_and_pd(w[0], exponentBits)),
        _mm256_castpd_si256(_mm256_and_pd(w[1], exponentBits))};
    const __m256i lo = _mm256_sub_epi32(_mm256_srli_epi64(exponentPart[0], 52),
                                        _mm256_set1_epi32(0x3fe));   // 0.1. 2.3.
    const __m256i hi = _mm256_sub_epi32(_mm256_srli_epi64(exponentPart[1], 52),
                                        _mm256_set1_epi32(0x3fe));   // 4.5. 6.7.
    const __m256i a = _mm256_unpacklo_epi32(lo, hi);                 // 04.. 26..
    const __m256i b = _mm256_unpackhi_epi32(lo, hi);                 // 15.. 37..
    const __m256i tmp = _mm256_unpacklo_epi32(a, b);                 // 0145 2367
    const __m256i exponent =
        AVX::concat(_mm_unpacklo_epi64(AVX::lo128(tmp), AVX::hi128(tmp)),
                    _mm_unpackhi_epi64(AVX::lo128(tmp), AVX::hi128(tmp)));  // 0123 4567
    const __m256d exponentMaximized[2] = {_mm256_or_pd(w[0], exponentBits),
                                          _mm256_or_pd(w[1], exponentBits)};
    const auto frexpMask =
        _mm256_broadcast_sd(reinterpret_cast<const double *>(&AVX::c_general::frexpMask));
    fixed_size_simd<double, 8> ret = {
        fixed_size_simd<double, 4>(
            AVX::double_v(_mm256_and_pd(exponentMaximized[0], frexpMask))),
        fixed_size_simd<double, 4>(
            AVX::double_v(_mm256_and_pd(exponentMaximized[1], frexpMask)))};
    const auto zeroMask = v == v.Zero();
    ret(isnan(v) || !isfinite(v) || zeroMask) = v;
    internal_data(*e) =
        Detail::andnot_(simd_cast<AVX2::int_m>(zeroMask).dataI(), exponent);
    return ret;
}
#endif  // Vc_IMPL_AVX2

namespace Detail
{
Vc_INTRINSIC AVX2::float_v::IndexType extractExponent(__m256 e)
{
    SimdArray<uint, float_v::Size> exponentPart;
    const auto ee = AVX::avx_cast<__m256i>(e);
#ifdef Vc_IMPL_AVX2
    exponentPart = AVX2::uint_v(ee);
#else
    internal_data(internal_data0(exponentPart)) = AVX::lo128(ee);
    internal_data(internal_data1(exponentPart)) = AVX::hi128(ee);
#endif
    return (exponentPart >> 23) - 0x7e;
}
}  // namespace Detail
inline AVX2::float_v frexp(AVX2::float_v::AsArg v, SimdArray<int, 8> *e)
{
    using namespace Detail;
    using namespace AVX2;
    const __m256 exponentBits = Const<float>::exponentMask().data();
    *e = extractExponent(and_(v.data(), exponentBits));
    const __m256 exponentMaximized = or_(v.data(), exponentBits);
    AVX2::float_v ret = _mm256_and_ps(exponentMaximized, avx_cast<__m256>(set1_epi32(0xbf7fffffu)));
    ret(isnan(v) || !isfinite(v) || v == AVX2::float_v::Zero()) = v;
    e->setZero(simd_cast<decltype(*e == *e)>(v == AVX2::float_v::Zero()));
    return ret;
}

// ldexp {{{1
/*             -> x * 2^e
 * x == NaN    -> NaN
 * x == (-)inf -> (-)inf
 */
inline AVX2::double_v ldexp(AVX2::double_v::AsArg v, const SimdArray<int, 4> &_e)
{
    SSE::int_v e = internal_data(_e);
    e.setZero(simd_cast<SSE::int_m>(v == AVX2::double_v::Zero()));
    const __m256i exponentBits =
        AVX::concat(_mm_slli_epi64(_mm_unpacklo_epi32(e.data(), e.data()), 52),
                    _mm_slli_epi64(_mm_unpackhi_epi32(e.data(), e.data()), 52));
    return AVX::avx_cast<__m256d>(
        AVX::add_epi64(AVX::avx_cast<__m256i>(v.data()), exponentBits));
}
inline AVX2::float_v ldexp(AVX2::float_v::AsArg v, SimdArray<int, 8> e)
{
    e.setZero(simd_cast<decltype(e == e)>(v == AVX2::float_v::Zero()));
    e <<= 23;
#ifdef Vc_IMPL_AVX2
    return {AVX::avx_cast<__m256>(
        AVX::concat(_mm_add_epi32(AVX::avx_cast<__m128i>(AVX::lo128(v.data())),
                                  AVX::lo128(internal_data(e).data())),
                    _mm_add_epi32(AVX::avx_cast<__m128i>(AVX::hi128(v.data())),
                                  AVX::hi128(internal_data(e).data()))))};
#else
    return {AVX::avx_cast<__m256>(
        AVX::concat(_mm_add_epi32(AVX::avx_cast<__m128i>(AVX::lo128(v.data())),
                                  internal_data(internal_data0(e)).data()),
                    _mm_add_epi32(AVX::avx_cast<__m128i>(AVX::hi128(v.data())),
                                  internal_data(internal_data1(e)).data())))};
#endif
}

// trunc {{{1
Vc_ALWAYS_INLINE AVX2::float_v trunc(AVX2::float_v::AsArg v)
{
    return _mm256_round_ps(v.data(), 0x3);
}
Vc_ALWAYS_INLINE AVX2::double_v trunc(AVX2::double_v::AsArg v)
{
    return _mm256_round_pd(v.data(), 0x3);
}

// floor {{{1
Vc_ALWAYS_INLINE AVX2::float_v floor(AVX2::float_v::AsArg v)
{
    return _mm256_floor_ps(v.data());
}
Vc_ALWAYS_INLINE AVX2::double_v floor(AVX2::double_v::AsArg v)
{
    return _mm256_floor_pd(v.data());
}

// ceil {{{1
Vc_ALWAYS_INLINE AVX2::float_v ceil(AVX2::float_v::AsArg v)
{
    return _mm256_ceil_ps(v.data());
}
Vc_ALWAYS_INLINE AVX2::double_v ceil(AVX2::double_v::AsArg v)
{
    return _mm256_ceil_pd(v.data());
}

// fma {{{1
template <typename T>
Vc_ALWAYS_INLINE Vector<T, VectorAbi::Avx> fma(Vector<T, VectorAbi::Avx> a,
                                               Vector<T, VectorAbi::Avx> b,
                                               Vector<T, VectorAbi::Avx> c)
{
    return Detail::fma(a.data(), b.data(), c.data(), T());
}

// }}}1
}  // namespace Vc

#endif // VC_AVX_MATH_H_

// vim: foldmethod=marker
