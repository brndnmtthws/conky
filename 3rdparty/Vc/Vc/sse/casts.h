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

#ifndef VC_SSE_CASTS_H_
#define VC_SSE_CASTS_H_

#include "intrinsics.h"
#include "types.h"
#include "macros.h"

namespace Vc_VERSIONED_NAMESPACE
{
namespace SSE
{
using uint = unsigned int;
using ushort = unsigned short;
using uchar = unsigned char;
using schar = signed char;

// sse_cast {{{1
template <typename To, typename From> Vc_ALWAYS_INLINE Vc_CONST To sse_cast(From v)
{
    return v;
}
template<> Vc_ALWAYS_INLINE Vc_CONST __m128i sse_cast<__m128i, __m128 >(__m128  v) { return _mm_castps_si128(v); }
template<> Vc_ALWAYS_INLINE Vc_CONST __m128i sse_cast<__m128i, __m128d>(__m128d v) { return _mm_castpd_si128(v); }
template<> Vc_ALWAYS_INLINE Vc_CONST __m128  sse_cast<__m128 , __m128d>(__m128d v) { return _mm_castpd_ps(v);    }
template<> Vc_ALWAYS_INLINE Vc_CONST __m128  sse_cast<__m128 , __m128i>(__m128i v) { return _mm_castsi128_ps(v); }
template<> Vc_ALWAYS_INLINE Vc_CONST __m128d sse_cast<__m128d, __m128i>(__m128i v) { return _mm_castsi128_pd(v); }
template<> Vc_ALWAYS_INLINE Vc_CONST __m128d sse_cast<__m128d, __m128 >(__m128  v) { return _mm_castps_pd(v);    }

// convert {{{1
template <typename From, typename To> struct ConvertTag
{
};
template <typename From, typename To>
Vc_INTRINSIC typename VectorTraits<To>::VectorType convert(
    typename VectorTraits<From>::VectorType v)
{
    return convert(v, ConvertTag<From, To>());
}

Vc_INTRINSIC __m128i convert(__m128  v, ConvertTag<float , int   >) { return _mm_cvttps_epi32(v); }
Vc_INTRINSIC __m128i convert(__m128d v, ConvertTag<double, int   >) { return _mm_cvttpd_epi32(v); }
Vc_INTRINSIC __m128i convert(__m128i v, ConvertTag<int   , int   >) { return v; }
Vc_INTRINSIC __m128i convert(__m128i v, ConvertTag<uint  , int   >) { return v; }
Vc_INTRINSIC __m128i convert(__m128i v, ConvertTag<short , int   >) {
#ifdef Vc_IMPL_SSE4_1
    return _mm_cvtepi16_epi32(v);
#else
    return _mm_srai_epi32(_mm_unpacklo_epi16(v, v), 16);
#endif
}
Vc_INTRINSIC __m128i convert(__m128i v, ConvertTag<ushort, int   >) {
#ifdef Vc_IMPL_SSE4_1
    return _mm_cvtepu16_epi32(v);
#else
    return _mm_srli_epi32(_mm_unpacklo_epi16(v, v), 16);
#endif
}
Vc_INTRINSIC __m128i convert(__m128  v, ConvertTag<float , uint  >) {
    return _mm_castps_si128(
        blendv_ps(_mm_castsi128_ps(_mm_cvttps_epi32(v)),
                  _mm_castsi128_ps(_mm_xor_si128(
                      _mm_cvttps_epi32(_mm_sub_ps(v, _mm_set1_ps(1u << 31))),
                      _mm_set1_epi32(1 << 31))),
                  _mm_cmpge_ps(v, _mm_set1_ps(1u << 31))));
}
Vc_INTRINSIC __m128i convert(__m128d v, ConvertTag<double, uint  >) {
#ifdef Vc_IMPL_SSE4_1
    return _mm_xor_si128(_mm_cvttpd_epi32(_mm_sub_pd(_mm_floor_pd(v), _mm_set1_pd(0x80000000u))),
                         _mm_cvtsi64_si128(0x8000000080000000ull));
#else
    return blendv_epi8(_mm_cvttpd_epi32(v),
                       _mm_xor_si128(_mm_cvttpd_epi32(_mm_sub_pd(v, _mm_set1_pd(0x80000000u))),
                                     _mm_cvtsi64_si128(0x8000000080000000ull)),
                       _mm_castpd_si128(_mm_cmpge_pd(v, _mm_set1_pd(0x80000000u))));
#endif
}
Vc_INTRINSIC __m128i convert(__m128i v, ConvertTag<int   , uint  >) { return v; }
Vc_INTRINSIC __m128i convert(__m128i v, ConvertTag<uint  , uint  >) { return v; }
Vc_INTRINSIC __m128i convert(__m128i v, ConvertTag<short , uint  >) { return convert(v, ConvertTag<short, int>()); }
Vc_INTRINSIC __m128i convert(__m128i v, ConvertTag<ushort, uint  >) { return convert(v, ConvertTag<ushort, int>()); }
Vc_INTRINSIC __m128  convert(__m128  v, ConvertTag<float , float >) { return v; }
Vc_INTRINSIC __m128  convert(__m128d v, ConvertTag<double, float >) { return _mm_cvtpd_ps(v); }
Vc_INTRINSIC __m128  convert(__m128i v, ConvertTag<int   , float >) { return _mm_cvtepi32_ps(v); }
Vc_INTRINSIC __m128  convert(__m128i v, ConvertTag<uint  , float >) {
    // see AVX::convert<uint, float> for an explanation of the math behind the
    // implementation
    using namespace SSE;
    return blendv_ps(_mm_cvtepi32_ps(v),
        _mm_add_ps(_mm_cvtepi32_ps(_mm_and_si128(v, _mm_set1_epi32(0x7ffffe00))),
                      _mm_add_ps(_mm_set1_ps(1u << 31), _mm_cvtepi32_ps(_mm_and_si128(
                                                          v, _mm_set1_epi32(0x000001ff))))),
        _mm_castsi128_ps(_mm_cmplt_epi32(v, _mm_setzero_si128())));
}
Vc_INTRINSIC __m128  convert(__m128i v, ConvertTag<short , float >) { return convert(convert(v, ConvertTag<short, int>()), ConvertTag<int, float>()); }
Vc_INTRINSIC __m128  convert(__m128i v, ConvertTag<ushort, float >) { return convert(convert(v, ConvertTag<ushort, int>()), ConvertTag<int, float>()); }
Vc_INTRINSIC __m128d convert(__m128  v, ConvertTag<float , double>) { return _mm_cvtps_pd(v); }
Vc_INTRINSIC __m128d convert(__m128d v, ConvertTag<double, double>) { return v; }
Vc_INTRINSIC __m128d convert(__m128i v, ConvertTag<int   , double>) { return _mm_cvtepi32_pd(v); }
Vc_INTRINSIC __m128d convert(__m128i v, ConvertTag<uint  , double>) { return _mm_add_pd(_mm_cvtepi32_pd(_mm_xor_si128(v, setmin_epi32())), _mm_set1_pd(1u << 31)); }
Vc_INTRINSIC __m128d convert(__m128i v, ConvertTag<short , double>) { return convert(convert(v, ConvertTag<short, int>()), ConvertTag<int, double>()); }
Vc_INTRINSIC __m128d convert(__m128i v, ConvertTag<ushort, double>) { return convert(convert(v, ConvertTag<ushort, int>()), ConvertTag<int, double>()); }
Vc_INTRINSIC __m128i convert(__m128  v, ConvertTag<float , short >) { return _mm_packs_epi32(_mm_cvttps_epi32(v), _mm_setzero_si128()); }
Vc_INTRINSIC __m128i convert(__m128i v, ConvertTag<int   , short >) { return _mm_packs_epi32(v, _mm_setzero_si128()); }
Vc_INTRINSIC __m128i convert(__m128i v, ConvertTag<uint  , short >) { return _mm_packs_epi32(v, _mm_setzero_si128()); }
Vc_INTRINSIC __m128i convert(__m128i v, ConvertTag<short , short >) { return v; }
Vc_INTRINSIC __m128i convert(__m128i v, ConvertTag<ushort, short >) { return v; }
Vc_INTRINSIC __m128i convert(__m128d v, ConvertTag<double, short >) { return convert(convert(v, ConvertTag<double, int>()), ConvertTag<int, short>()); }
Vc_INTRINSIC __m128i convert(__m128i v, ConvertTag<int   , ushort>) {
    auto tmp0 = _mm_unpacklo_epi16(v, _mm_setzero_si128());  // 0 4 X X 1 5 X X
    auto tmp1 = _mm_unpackhi_epi16(v, _mm_setzero_si128());  // 2 6 X X 3 7 X X
    auto tmp2 = _mm_unpacklo_epi16(tmp0, tmp1);              // 0 2 4 6 X X X X
    auto tmp3 = _mm_unpackhi_epi16(tmp0, tmp1);              // 1 3 5 7 X X X X
    return _mm_unpacklo_epi16(tmp2, tmp3);                   // 0 1 2 3 4 5 6 7
}
Vc_INTRINSIC __m128i convert(__m128i v, ConvertTag<uint  , ushort>) {
    auto tmp0 = _mm_unpacklo_epi16(v, _mm_setzero_si128());  // 0 4 X X 1 5 X X
    auto tmp1 = _mm_unpackhi_epi16(v, _mm_setzero_si128());  // 2 6 X X 3 7 X X
    auto tmp2 = _mm_unpacklo_epi16(tmp0, tmp1);              // 0 2 4 6 X X X X
    auto tmp3 = _mm_unpackhi_epi16(tmp0, tmp1);              // 1 3 5 7 X X X X
    return _mm_unpacklo_epi16(tmp2, tmp3);                   // 0 1 2 3 4 5 6 7
}
Vc_INTRINSIC __m128i convert(__m128  v, ConvertTag<float , ushort>) { return convert(_mm_cvttps_epi32(v), ConvertTag<int, ushort>()); }
Vc_INTRINSIC __m128i convert(__m128i v, ConvertTag<short , ushort>) { return v; }
Vc_INTRINSIC __m128i convert(__m128i v, ConvertTag<ushort, ushort>) { return v; }
Vc_INTRINSIC __m128i convert(__m128d v, ConvertTag<double, ushort>) { return convert(convert(v, ConvertTag<double, int>()), ConvertTag<int, ushort>()); }

// }}}1
}  // namespace SSE
}  // namespace Vc

#endif // VC_SSE_CASTS_H_

// vim: foldmethod=marker
