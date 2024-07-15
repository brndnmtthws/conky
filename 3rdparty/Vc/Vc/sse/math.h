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

#ifndef VC_SSE_MATH_H_
#define VC_SSE_MATH_H_

#include "const.h"
#include "macros.h"

namespace Vc_VERSIONED_NAMESPACE
{
// copysign {{{1
Vc_INTRINSIC Vc_CONST SSE::float_v copysign(SSE::float_v mag, SSE::float_v sign)
{
    return _mm_or_ps(_mm_and_ps(sign.data(), SSE::_mm_setsignmask_ps()),
                     _mm_and_ps(mag.data(), SSE::_mm_setabsmask_ps()));
}
Vc_INTRINSIC Vc_CONST SSE::double_v copysign(SSE::double_v mag, SSE::double_v sign)
{
    return _mm_or_pd(_mm_and_pd(sign.data(), SSE::_mm_setsignmask_pd()),
                     _mm_and_pd(mag.data(), SSE::_mm_setabsmask_pd()));
}

//}}}1

/**
 * splits \p v into exponent and mantissa, the sign is kept with the mantissa
 *
 * The return value will be in the range [0.5, 1.0[
 * The \p e value will be an integer defining the power-of-two exponent
 */
inline SSE::double_v frexp(const SSE::double_v &v,
                           SimdArray<int, 2, Scalar::int_v, 1> *e)
{
    const __m128i exponentBits = SSE::Const<double>::exponentMask().dataI();
    const __m128i exponentPart = _mm_and_si128(_mm_castpd_si128(v.data()), exponentBits);
    SSE::int_v exponent =
        _mm_sub_epi32(_mm_srli_epi64(exponentPart, 52), _mm_set1_epi32(0x3fe));
    const __m128d exponentMaximized = _mm_or_pd(v.data(), _mm_castsi128_pd(exponentBits));
    SSE::double_v ret = _mm_and_pd(
        exponentMaximized,
        _mm_load_pd(reinterpret_cast<const double *>(&SSE::c_general::frexpMask[0])));
    SSE::double_m zeroMask = v == SSE::double_v::Zero();
    ret(isnan(v) || !isfinite(v) || zeroMask) = v;
    exponent.setZero(zeroMask.data());
    (*e)[0] = exponent[0];
    (*e)[1] = exponent[2];
    return ret;
}
inline SSE::float_v frexp(const SSE::float_v &v, SimdArray<int, 4, SSE::int_v, 4> *e)
{
    const __m128i exponentBits = SSE::Const<float>::exponentMask().dataI();
    const __m128i exponentPart = _mm_and_si128(_mm_castps_si128(v.data()), exponentBits);
    internal_data(*e) =
        _mm_sub_epi32(_mm_srli_epi32(exponentPart, 23), _mm_set1_epi32(0x7e));
    const __m128 exponentMaximized = _mm_or_ps(v.data(), _mm_castsi128_ps(exponentBits));
    SSE::float_v ret =
        _mm_and_ps(exponentMaximized, _mm_castsi128_ps(_mm_set1_epi32(0xbf7fffffu)));
    ret(isnan(v) || !isfinite(v) || v == SSE::float_v::Zero()) = v;
    e->setZero(v == SSE::float_v::Zero());
    return ret;
}

/*             -> x * 2^e
 * x == NaN    -> NaN
 * x == (-)inf -> (-)inf
 */
inline SSE::double_v ldexp(SSE::double_v::AsArg v,
                           const SimdArray<int, 2, Scalar::int_v, 1> &_e)
{
    SSE::int_v e = _mm_setr_epi32(_e[0], 0, _e[1], 0);
    e.setZero((v == SSE::double_v::Zero()).dataI());
    const __m128i exponentBits = _mm_slli_epi64(e.data(), 52);
    return _mm_castsi128_pd(_mm_add_epi64(_mm_castpd_si128(v.data()), exponentBits));
}
inline SSE::float_v ldexp(SSE::float_v::AsArg v,
                          const SimdArray<int, 4, SSE::int_v, 4> &_e)
{
    SSE::int_v e = internal_data(_e);
    e.setZero(simd_cast<SSE::int_m>(v == SSE::float_v::Zero()));
    return reinterpret_components_cast<SSE::float_v>(
        reinterpret_components_cast<SSE::int_v>(v) + (e << 23));
}

#ifdef Vc_IMPL_SSE4_1
inline SSE::double_v trunc(SSE::double_v::AsArg v) { return _mm_round_pd(v.data(), 0x3); }
inline SSE::float_v trunc(SSE::float_v::AsArg v) { return _mm_round_ps(v.data(), 0x3); }

inline SSE::double_v floor(SSE::double_v::AsArg v) { return _mm_floor_pd(v.data()); }
inline SSE::float_v floor(SSE::float_v::AsArg v) { return _mm_floor_ps(v.data()); }

inline SSE::double_v ceil(SSE::double_v::AsArg v) { return _mm_ceil_pd(v.data()); }
inline SSE::float_v ceil(SSE::float_v::AsArg v) { return _mm_ceil_ps(v.data()); }
#else
inline SSE::Vector<float> trunc(SSE::Vector<float> x)
{
    const auto truncated = _mm_cvtepi32_ps(_mm_cvttps_epi32(x.data()));
    const auto no_fractional_values = _mm_castsi128_ps(_mm_cmplt_epi32(
        _mm_and_si128(_mm_castps_si128(x.data()), _mm_set1_epi32(0x7f800000u)),
        _mm_set1_epi32(0x4b000000)));  // the exponent is so large that no mantissa bits
                                       // signify fractional values (0x3f8 + 23*8 = 0x4b0)
    return _mm_or_ps(_mm_andnot_ps(no_fractional_values, x.data()),
                     _mm_and_ps(no_fractional_values, truncated));
}

inline SSE::Vector<double> trunc(SSE::Vector<double> x)
{
    const auto abs_x = Vc::abs(x).data();
    const auto min_no_fractional_bits =
        _mm_castsi128_pd(_mm_set1_epi64x(0x4330000000000000ull));  // 0x3ff + 52 = 0x433
    __m128d truncated =
        _mm_sub_pd(_mm_add_pd(abs_x, min_no_fractional_bits), min_no_fractional_bits);
    // due to rounding, the result can be too large. In this case `truncated >
    // abs(x)` holds, so subtract 1 to truncated if `abs(x) < truncated`
    truncated = _mm_sub_pd(truncated,
                           _mm_and_pd(_mm_cmplt_pd(abs_x, truncated), _mm_set1_pd(1.)));
    // finally, fix the sign bit:
    return _mm_or_pd(
        _mm_and_pd(_mm_castsi128_pd(_mm_set1_epi64x(0x8000000000000000ull)), x.data()),
        truncated);
}

template <typename T> inline SSE::Vector<T> floor(SSE::Vector<T> x)
{
    auto y = trunc(x);
    y(!(y == x) && x < 0) -= 1;
    return y;
}

template <typename T> inline SSE::Vector<T> ceil(SSE::Vector<T> x)
{
    auto y = trunc(x);
    y(!(y == x || x < 0)) += 1;
    return y;
}
#endif
// fma {{{1
template <typename T>
Vc_ALWAYS_INLINE Vector<T, VectorAbi::Sse> fma(Vector<T, VectorAbi::Sse> a,
                                               Vector<T, VectorAbi::Sse> b,
                                               Vector<T, VectorAbi::Sse> c)
{
    SSE::VectorHelper<T>::fma(a.data(), b.data(), c.data());
    return a;
}
// }}}1
}  // namespace Vc

#endif // VC_SSE_MATH_H_

// vim: foldmethod=marker
