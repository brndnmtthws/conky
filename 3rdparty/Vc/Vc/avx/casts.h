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

#ifndef VC_AVX_CASTS_H_
#define VC_AVX_CASTS_H_

#include "intrinsics.h"
#include "types.h"
#include "../sse/casts.h"
#include "shuffle.h"
#include "macros.h"

namespace Vc_VERSIONED_NAMESPACE
{
namespace AVX
{
namespace Casts
{
    template<typename T> Vc_INTRINSIC_L T avx_cast(__m128  v) Vc_INTRINSIC_R;
    template<typename T> Vc_INTRINSIC_L T avx_cast(__m128i v) Vc_INTRINSIC_R;
    template<typename T> Vc_INTRINSIC_L T avx_cast(__m128d v) Vc_INTRINSIC_R;
    template<typename T> Vc_INTRINSIC_L T avx_cast(__m256  v) Vc_INTRINSIC_R;
    template<typename T> Vc_INTRINSIC_L T avx_cast(__m256i v) Vc_INTRINSIC_R;
    template<typename T> Vc_INTRINSIC_L T avx_cast(__m256d v) Vc_INTRINSIC_R;

    // 128 -> 128
    template<> Vc_INTRINSIC __m128  avx_cast(__m128  v) { return v; }
    template<> Vc_INTRINSIC __m128  avx_cast(__m128i v) { return _mm_castsi128_ps(v); }
    template<> Vc_INTRINSIC __m128  avx_cast(__m128d v) { return _mm_castpd_ps(v); }
    template<> Vc_INTRINSIC __m128i avx_cast(__m128  v) { return _mm_castps_si128(v); }
    template<> Vc_INTRINSIC __m128i avx_cast(__m128i v) { return v; }
    template<> Vc_INTRINSIC __m128i avx_cast(__m128d v) { return _mm_castpd_si128(v); }
    template<> Vc_INTRINSIC __m128d avx_cast(__m128  v) { return _mm_castps_pd(v); }
    template<> Vc_INTRINSIC __m128d avx_cast(__m128i v) { return _mm_castsi128_pd(v); }
    template<> Vc_INTRINSIC __m128d avx_cast(__m128d v) { return v; }

    // 128 -> 256
    // FIXME: the following casts leave the upper 128bits undefined. With GCC and ICC I've never
    // seen the cast not do what I want though: after a VEX-coded SSE instruction the register's
    // upper 128bits are zero. Thus using the same register as AVX register will have the upper
    // 128bits zeroed. MSVC, though, implements _mm256_castxx128_xx256 with a 128bit move to memory
    // + 256bit load. Thus the upper 128bits are really undefined. But there is no intrinsic to do
    // what I want (i.e. alias the register, disallowing the move to memory in-between). I'm stuck,
    // do we really want to rely on specific compiler behavior here?
    template<> Vc_INTRINSIC __m256  avx_cast(__m128  v) { return _mm256_castps128_ps256(v); }
    template<> Vc_INTRINSIC __m256  avx_cast(__m128i v) { return _mm256_castps128_ps256(_mm_castsi128_ps(v)); }
    template<> Vc_INTRINSIC __m256  avx_cast(__m128d v) { return _mm256_castps128_ps256(_mm_castpd_ps(v)); }
    template<> Vc_INTRINSIC __m256i avx_cast(__m128  v) { return _mm256_castsi128_si256(_mm_castps_si128(v)); }
    template<> Vc_INTRINSIC __m256i avx_cast(__m128i v) { return _mm256_castsi128_si256(v); }
    template<> Vc_INTRINSIC __m256i avx_cast(__m128d v) { return _mm256_castsi128_si256(_mm_castpd_si128(v)); }
    template<> Vc_INTRINSIC __m256d avx_cast(__m128  v) { return _mm256_castpd128_pd256(_mm_castps_pd(v)); }
    template<> Vc_INTRINSIC __m256d avx_cast(__m128i v) { return _mm256_castpd128_pd256(_mm_castsi128_pd(v)); }
    template<> Vc_INTRINSIC __m256d avx_cast(__m128d v) { return _mm256_castpd128_pd256(v); }

#if defined Vc_MSVC || defined Vc_CLANG || defined Vc_APPLECLANG
    static Vc_INTRINSIC Vc_CONST __m256  zeroExtend(__m128  v) { return _mm256_permute2f128_ps   (_mm256_castps128_ps256(v), _mm256_castps128_ps256(v), 0x80); }
    static Vc_INTRINSIC Vc_CONST __m256i zeroExtend(__m128i v) { return _mm256_permute2f128_si256(_mm256_castsi128_si256(v), _mm256_castsi128_si256(v), 0x80); }
    static Vc_INTRINSIC Vc_CONST __m256d zeroExtend(__m128d v) { return _mm256_permute2f128_pd   (_mm256_castpd128_pd256(v), _mm256_castpd128_pd256(v), 0x80); }
#else
    static Vc_INTRINSIC Vc_CONST __m256  zeroExtend(__m128  v) { return _mm256_castps128_ps256(v); }
    static Vc_INTRINSIC Vc_CONST __m256i zeroExtend(__m128i v) { return _mm256_castsi128_si256(v); }
    static Vc_INTRINSIC Vc_CONST __m256d zeroExtend(__m128d v) { return _mm256_castpd128_pd256(v); }
#endif

    // 256 -> 128
    template<> Vc_INTRINSIC __m128  avx_cast(__m256  v) { return _mm256_castps256_ps128(v); }
    template<> Vc_INTRINSIC __m128  avx_cast(__m256i v) { return _mm256_castps256_ps128(_mm256_castsi256_ps(v)); }
    template<> Vc_INTRINSIC __m128  avx_cast(__m256d v) { return _mm256_castps256_ps128(_mm256_castpd_ps(v)); }
    template<> Vc_INTRINSIC __m128i avx_cast(__m256  v) { return _mm256_castsi256_si128(_mm256_castps_si256(v)); }
    template<> Vc_INTRINSIC __m128i avx_cast(__m256i v) { return _mm256_castsi256_si128(v); }
    template<> Vc_INTRINSIC __m128i avx_cast(__m256d v) { return _mm256_castsi256_si128(_mm256_castpd_si256(v)); }
    template<> Vc_INTRINSIC __m128d avx_cast(__m256  v) { return _mm256_castpd256_pd128(_mm256_castps_pd(v)); }
    template<> Vc_INTRINSIC __m128d avx_cast(__m256i v) { return _mm256_castpd256_pd128(_mm256_castsi256_pd(v)); }
    template<> Vc_INTRINSIC __m128d avx_cast(__m256d v) { return _mm256_castpd256_pd128(v); }

    // 256 -> 256
    template<> Vc_INTRINSIC __m256  avx_cast(__m256  v) { return v; }
    template<> Vc_INTRINSIC __m256  avx_cast(__m256i v) { return _mm256_castsi256_ps(v); }
    template<> Vc_INTRINSIC __m256  avx_cast(__m256d v) { return _mm256_castpd_ps(v); }
    template<> Vc_INTRINSIC __m256i avx_cast(__m256  v) { return _mm256_castps_si256(v); }
    template<> Vc_INTRINSIC __m256i avx_cast(__m256i v) { return v; }
    template<> Vc_INTRINSIC __m256i avx_cast(__m256d v) { return _mm256_castpd_si256(v); }
    template<> Vc_INTRINSIC __m256d avx_cast(__m256  v) { return _mm256_castps_pd(v); }
    template<> Vc_INTRINSIC __m256d avx_cast(__m256i v) { return _mm256_castsi256_pd(v); }
    template<> Vc_INTRINSIC __m256d avx_cast(__m256d v) { return v; }

    // simplify splitting 256-bit registers in 128-bit registers
    Vc_INTRINSIC Vc_CONST __m128  lo128(__m256  v) { return avx_cast<__m128>(v); }
    Vc_INTRINSIC Vc_CONST __m128d lo128(__m256d v) { return avx_cast<__m128d>(v); }
    Vc_INTRINSIC Vc_CONST __m128i lo128(__m256i v) { return avx_cast<__m128i>(v); }
    Vc_INTRINSIC Vc_CONST __m128  hi128(__m256  v) { return extract128<1>(v); }
    Vc_INTRINSIC Vc_CONST __m128d hi128(__m256d v) { return extract128<1>(v); }
    Vc_INTRINSIC Vc_CONST __m128i hi128(__m256i v) { return extract128<1>(v); }

    // simplify combining 128-bit registers in 256-bit registers
    Vc_INTRINSIC Vc_CONST __m256  concat(__m128  a, __m128  b) { return insert128<1>(avx_cast<__m256 >(a), b); }
    Vc_INTRINSIC Vc_CONST __m256d concat(__m128d a, __m128d b) { return insert128<1>(avx_cast<__m256d>(a), b); }
    Vc_INTRINSIC Vc_CONST __m256i concat(__m128i a, __m128i b) { return insert128<1>(avx_cast<__m256i>(a), b); }

}  // namespace Casts
using namespace Casts;
}  // namespace AVX

namespace AVX2
{
using namespace AVX::Casts;
}  // namespace AVX2

namespace AVX
{
template <typename From, typename To> struct ConvertTag {};

Vc_INTRINSIC __m256i convert(__m256  v, ConvertTag<float , int>) { return _mm256_cvttps_epi32(v); }
Vc_INTRINSIC __m128i convert(__m256d v, ConvertTag<double, int>) { return _mm256_cvttpd_epi32(v); }
Vc_INTRINSIC __m256i convert(__m256i v, ConvertTag<int   , int>) { return v; }
Vc_INTRINSIC __m256i convert(__m256i v, ConvertTag<uint  , int>) { return v; }
Vc_INTRINSIC __m256i convert(__m128i v, ConvertTag<short , int>) {
#ifdef Vc_IMPL_AVX2
    return _mm256_cvtepi16_epi32(v);
#else
    return AVX::srai_epi32<16>(
        concat(_mm_unpacklo_epi16(v, v), _mm_unpackhi_epi16(v, v)));
#endif
}
Vc_INTRINSIC __m256i convert(__m128i v, ConvertTag<ushort, int>) {
#ifdef Vc_IMPL_AVX2
    return _mm256_cvtepu16_epi32(v);
#else
    return AVX::srli_epi32<16>(
        concat(_mm_unpacklo_epi16(v, v), _mm_unpackhi_epi16(v, v)));
#endif
}

Vc_INTRINSIC __m256i convert(__m256  v, ConvertTag<float , uint>) {
    using namespace AVX;
    return _mm256_castps_si256(_mm256_blendv_ps(
        _mm256_castsi256_ps(_mm256_cvttps_epi32(v)),
        _mm256_castsi256_ps(add_epi32(_mm256_cvttps_epi32(_mm256_sub_ps(v, set2power31_ps())),
                                      set2power31_epu32())),
        cmpge_ps(v, set2power31_ps())));
}
Vc_INTRINSIC __m128i convert(__m256d v, ConvertTag<double, uint>) {
    using namespace AVX;
    return _mm_xor_si128(
        _mm256_cvttpd_epi32(_mm256_sub_pd(_mm256_floor_pd(v), set1_pd(0x80000000u))),
        _mm_set2power31_epu32());
}
Vc_INTRINSIC __m256i convert(__m256i v, ConvertTag<int   , uint>) { return v; }
Vc_INTRINSIC __m256i convert(__m256i v, ConvertTag<uint  , uint>) { return v; }
Vc_INTRINSIC __m256i convert(__m128i v, ConvertTag<short , uint>) {
#ifdef Vc_IMPL_AVX2
    return _mm256_cvtepi16_epi32(v);
#else
    return AVX::srai_epi32<16>(
        concat(_mm_unpacklo_epi16(v, v), _mm_unpackhi_epi16(v, v)));
#endif
}
Vc_INTRINSIC __m256i convert(__m128i v, ConvertTag<ushort, uint>) {
#ifdef Vc_IMPL_AVX2
    return _mm256_cvtepu16_epi32(v);
#else
    return AVX::srli_epi32<16>(
        concat(_mm_unpacklo_epi16(v, v), _mm_unpackhi_epi16(v, v)));
#endif
}

Vc_INTRINSIC __m256  convert(__m256  v, ConvertTag<float , float>) { return v; }
Vc_INTRINSIC __m128  convert(__m256d v, ConvertTag<double, float>) { return _mm256_cvtpd_ps(v); }
Vc_INTRINSIC __m256  convert(__m256i v, ConvertTag<int   , float>) { return _mm256_cvtepi32_ps(v); }
Vc_INTRINSIC __m256  convert(__m256i v, ConvertTag<uint  , float>) {
    // this is complicated because cvtepi32_ps only supports signed input. Thus, all
    // input values with the MSB set would produce a negative result. We can reuse the
    // cvtepi32_ps instruction if we unset the MSB. But then the rounding results can be
    // different. Since float uses 24 bits for the mantissa (effectively), the 9-bit LSB
    // determines the rounding direction. (Consider the bits ...8'7654'3210. The bits [0:7]
    // need to be dropped and if > 0x80 round up, if < 0x80 round down. If [0:7] == 0x80
    // then the rounding direction is determined by bit [8] for round to even. That's why
    // the 9th bit is relevant for the rounding decision.)
    // If the MSB of the input is set to 0, the cvtepi32_ps instruction makes its rounding
    // decision on the lowest 8 bits instead. A second rounding decision is made when
    // float(0x8000'0000) is added. This will rarely fix the rounding issue.
    //
    // Here's what the standard rounding mode expects:
    // 0xc0000080 should cvt to 0xc0000000
    // 0xc0000081 should cvt to 0xc0000100
    //     --     should cvt to 0xc0000100
    // 0xc000017f should cvt to 0xc0000100
    // 0xc0000180 should cvt to 0xc0000200
    //
    // However: using float(input ^ 0x8000'0000) + float(0x8000'0000) we get:
    // 0xc0000081 would cvt to 0xc0000000
    // 0xc00000c0 would cvt to 0xc0000000
    // 0xc00000c1 would cvt to 0xc0000100
    // 0xc000013f would cvt to 0xc0000100
    // 0xc0000140 would cvt to 0xc0000200
    //
    // Solution: float(input & 0x7fff'fe00) + (float(0x8000'0000) + float(input & 0x1ff))
    // This ensures the rounding decision is made on the 9-bit LSB when 0x8000'0000 is
    // added to the float value of the low 8 bits of the input.
    using namespace AVX;
    return _mm256_blendv_ps(
        _mm256_cvtepi32_ps(v),
        _mm256_add_ps(_mm256_cvtepi32_ps(and_si256(v, set1_epi32(0x7ffffe00))),
                      _mm256_add_ps(set2power31_ps(), _mm256_cvtepi32_ps(and_si256(
                                                          v, set1_epi32(0x000001ff))))),
        _mm256_castsi256_ps(cmplt_epi32(v, _mm256_setzero_si256())));
}
Vc_INTRINSIC __m256  convert(__m128i v, ConvertTag<short , float>) { return _mm256_cvtepi32_ps(convert(v, ConvertTag< short, int>())); }
Vc_INTRINSIC __m256  convert(__m128i v, ConvertTag<ushort, float>) { return _mm256_cvtepi32_ps(convert(v, ConvertTag<ushort, int>())); }

Vc_INTRINSIC __m256d convert(__m128  v, ConvertTag<float , double>) { return _mm256_cvtps_pd(v); }
Vc_INTRINSIC __m256d convert(__m256d v, ConvertTag<double, double>) { return v; }
Vc_INTRINSIC __m256d convert(__m128i v, ConvertTag<int   , double>) { return _mm256_cvtepi32_pd(v); }
Vc_INTRINSIC __m256d convert(__m128i v, ConvertTag<uint  , double>) {
    using namespace AVX;
    return _mm256_add_pd(
        _mm256_cvtepi32_pd(_mm_xor_si128(v, _mm_setmin_epi32())),
        set1_pd(1u << 31)); }
Vc_INTRINSIC __m256d convert(__m128i v, ConvertTag<short , double>) { return convert(convert(v, SSE::ConvertTag< short, int>()), ConvertTag<int, double>()); }
Vc_INTRINSIC __m256d convert(__m128i v, ConvertTag<ushort, double>) { return convert(convert(v, SSE::ConvertTag<ushort, int>()), ConvertTag<int, double>()); }

Vc_INTRINSIC __m128i convert(__m256i v, ConvertTag<int   , short>) {
#ifdef Vc_IMPL_AVX2
    auto a = _mm256_shuffle_epi8(
        v, _mm256_setr_epi8(0, 1, 4, 5, 8, 9, 12, 13, -0x80, -0x80, -0x80, -0x80, -0x80,
                            -0x80, -0x80, -0x80, 0, 1, 4, 5, 8, 9, 12, 13, -0x80, -0x80,
                            -0x80, -0x80, -0x80, -0x80, -0x80, -0x80));
    return lo128(_mm256_permute4x64_epi64(a, 0xf8));  // a[0] a[2] | a[3] a[3]
#else
    const auto tmp0 = _mm_unpacklo_epi16(lo128(v), hi128(v));
    const auto tmp1 = _mm_unpackhi_epi16(lo128(v), hi128(v));
    const auto tmp2 = _mm_unpacklo_epi16(tmp0, tmp1);
    const auto tmp3 = _mm_unpackhi_epi16(tmp0, tmp1);
    return _mm_unpacklo_epi16(tmp2, tmp3);
#endif
}
Vc_INTRINSIC __m128i convert(__m256i v, ConvertTag<uint  , short>) { return convert(v, ConvertTag<int, short>()); }
Vc_INTRINSIC __m128i convert(__m256  v, ConvertTag<float , short>) { return convert(convert(v, ConvertTag<float, int>()), ConvertTag<int, short>()); }
Vc_INTRINSIC __m128i convert(__m256d v, ConvertTag<double, short>) { return convert(convert(v, ConvertTag<double, int>()), SSE::ConvertTag<int, short>()); }
Vc_INTRINSIC __m256i convert(__m256i v, ConvertTag<short , short>) { return v; }
Vc_INTRINSIC __m256i convert(__m256i v, ConvertTag<ushort, short>) { return v; }

Vc_INTRINSIC __m128i convert(__m256i v, ConvertTag<int   , ushort>) {
    auto tmp0 = _mm_unpacklo_epi16(lo128(v), hi128(v));
    auto tmp1 = _mm_unpackhi_epi16(lo128(v), hi128(v));
    auto tmp2 = _mm_unpacklo_epi16(tmp0, tmp1);
    auto tmp3 = _mm_unpackhi_epi16(tmp0, tmp1);
    return _mm_unpacklo_epi16(tmp2, tmp3);
}
Vc_INTRINSIC __m128i convert(__m256i v, ConvertTag<uint  , ushort>) {
    auto tmp0 = _mm_unpacklo_epi16(lo128(v), hi128(v));
    auto tmp1 = _mm_unpackhi_epi16(lo128(v), hi128(v));
    auto tmp2 = _mm_unpacklo_epi16(tmp0, tmp1);
    auto tmp3 = _mm_unpackhi_epi16(tmp0, tmp1);
    return _mm_unpacklo_epi16(tmp2, tmp3);
}
Vc_INTRINSIC __m128i convert(__m256  v, ConvertTag<float , ushort>) { return convert(convert(v, ConvertTag<float, uint>()), ConvertTag<uint, ushort>()); }
Vc_INTRINSIC __m128i convert(__m256d v, ConvertTag<double, ushort>) { return convert(convert(v, ConvertTag<double, uint>()), SSE::ConvertTag<uint, ushort>()); }
Vc_INTRINSIC __m256i convert(__m256i v, ConvertTag<short , ushort>) { return v; }
Vc_INTRINSIC __m256i convert(__m256i v, ConvertTag<ushort, ushort>) { return v; }

template <typename From, typename To>
Vc_INTRINSIC auto convert(
    typename std::conditional<(sizeof(From) < sizeof(To)),
                              typename SSE::VectorTraits<From>::VectorType,
                              typename AVX::VectorTypeHelper<From>::Type>::type v)
    -> decltype(convert(v, ConvertTag<From, To>()))
{
    return convert(v, ConvertTag<From, To>());
}

template <typename From, typename To, typename = enable_if<(sizeof(From) < sizeof(To))>>
Vc_INTRINSIC auto convert(typename AVX::VectorTypeHelper<From>::Type v)
    -> decltype(convert(lo128(v), ConvertTag<From, To>()))
{
    return convert(lo128(v), ConvertTag<From, To>());
}
}  // namespace AVX
}  // namespace Vc

#endif // VC_AVX_CASTS_H_
