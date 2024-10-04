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

#ifndef VC_SSE_INTRINSICS_H_
#define VC_SSE_INTRINSICS_H_

#ifdef Vc_MSVC
#include <intrin.h>
#else
#include <x86intrin.h>
#endif

#include "../common/storage.h"
#include "const_data.h"
#include <cstdlib>
#include "types.h"
#include "debug.h"

#if defined(Vc_GCC) && !defined(__OPTIMIZE__)
// GCC uses lots of old-style-casts in macros that disguise as intrinsics
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#endif

namespace Vc_VERSIONED_NAMESPACE
{
namespace SseIntrinsics
{
    using SSE::c_general;

    constexpr std::size_t VectorAlignment = 16;

#if defined(Vc_GCC) && Vc_GCC < 0x40600 && !defined(Vc_DONT_FIX_SSE_SHIFT)
    static Vc_INTRINSIC Vc_CONST __m128i _mm_sll_epi16(__m128i a, __m128i count) { __asm__("psllw %1,%0" : "+x"(a) : "x"(count)); return a; }
    static Vc_INTRINSIC Vc_CONST __m128i _mm_sll_epi32(__m128i a, __m128i count) { __asm__("pslld %1,%0" : "+x"(a) : "x"(count)); return a; }
    static Vc_INTRINSIC Vc_CONST __m128i _mm_sll_epi64(__m128i a, __m128i count) { __asm__("psllq %1,%0" : "+x"(a) : "x"(count)); return a; }
    static Vc_INTRINSIC Vc_CONST __m128i _mm_srl_epi16(__m128i a, __m128i count) { __asm__("psrlw %1,%0" : "+x"(a) : "x"(count)); return a; }
    static Vc_INTRINSIC Vc_CONST __m128i _mm_srl_epi32(__m128i a, __m128i count) { __asm__("psrld %1,%0" : "+x"(a) : "x"(count)); return a; }
    static Vc_INTRINSIC Vc_CONST __m128i _mm_srl_epi64(__m128i a, __m128i count) { __asm__("psrlq %1,%0" : "+x"(a) : "x"(count)); return a; }
#endif

#ifdef Vc_GCC
    // Redefine the mul/add/sub intrinsics to use GCC-specific operators instead of builtin
    // functions. This way the fp-contraction optimization step kicks in and creates FMAs! :)
    static Vc_INTRINSIC Vc_CONST __m128d _mm_mul_pd(__m128d a, __m128d b) { return static_cast<__m128d>(static_cast<__v2df>(a) * static_cast<__v2df>(b)); }
    static Vc_INTRINSIC Vc_CONST __m128d _mm_add_pd(__m128d a, __m128d b) { return static_cast<__m128d>(static_cast<__v2df>(a) + static_cast<__v2df>(b)); }
    static Vc_INTRINSIC Vc_CONST __m128d _mm_sub_pd(__m128d a, __m128d b) { return static_cast<__m128d>(static_cast<__v2df>(a) - static_cast<__v2df>(b)); }
    static Vc_INTRINSIC Vc_CONST __m128  _mm_mul_ps(__m128  a, __m128  b) { return static_cast<__m128 >(static_cast<__v4sf>(a) * static_cast<__v4sf>(b)); }
    static Vc_INTRINSIC Vc_CONST __m128  _mm_add_ps(__m128  a, __m128  b) { return static_cast<__m128 >(static_cast<__v4sf>(a) + static_cast<__v4sf>(b)); }
    static Vc_INTRINSIC Vc_CONST __m128  _mm_sub_ps(__m128  a, __m128  b) { return static_cast<__m128 >(static_cast<__v4sf>(a) - static_cast<__v4sf>(b)); }
#endif

    static Vc_INTRINSIC Vc_CONST __m128i _mm_setallone_si128() { return _mm_load_si128(reinterpret_cast<const __m128i *>(Common::AllBitsSet)); }
    static Vc_INTRINSIC Vc_CONST __m128d _mm_setallone_pd() { return _mm_load_pd(reinterpret_cast<const double *>(Common::AllBitsSet)); }
    static Vc_INTRINSIC Vc_CONST __m128  _mm_setallone_ps() { return _mm_load_ps(reinterpret_cast<const float *>(Common::AllBitsSet)); }

    static Vc_INTRINSIC __m128i Vc_CONST _mm_setone_epi16()  { return _mm_load_si128(reinterpret_cast<const __m128i *>(c_general::one16)); }
    static Vc_INTRINSIC __m128i Vc_CONST _mm_setone_epu16()  { return _mm_setone_epi16(); }
    static Vc_INTRINSIC __m128i Vc_CONST _mm_setone_epi32()  { return _mm_load_si128(reinterpret_cast<const __m128i *>(c_general::one32)); }
    static Vc_INTRINSIC __m128i Vc_CONST _mm_setone_epu32()  { return _mm_setone_epi32(); }

    static Vc_INTRINSIC __m128  Vc_CONST _mm_setone_ps()     { return _mm_load_ps(c_general::oneFloat); }
    static Vc_INTRINSIC __m128d Vc_CONST _mm_setone_pd()     { return _mm_load_pd(c_general::oneDouble); }

    static Vc_INTRINSIC __m128d Vc_CONST _mm_setabsmask_pd() { return _mm_load_pd(reinterpret_cast<const double *>(c_general::absMaskDouble)); }
    static Vc_INTRINSIC __m128  Vc_CONST _mm_setabsmask_ps() { return _mm_load_ps(reinterpret_cast<const float *>(c_general::absMaskFloat)); }
    static Vc_INTRINSIC __m128d Vc_CONST _mm_setsignmask_pd(){ return _mm_load_pd(reinterpret_cast<const double *>(c_general::signMaskDouble)); }
    static Vc_INTRINSIC __m128  Vc_CONST _mm_setsignmask_ps(){ return _mm_load_ps(reinterpret_cast<const float *>(c_general::signMaskFloat)); }

    static Vc_INTRINSIC __m128i Vc_CONST setmin_epi8 () { return _mm_set1_epi8(-0x80); }
    static Vc_INTRINSIC __m128i Vc_CONST setmin_epi16() { return _mm_load_si128(reinterpret_cast<const __m128i *>(c_general::minShort)); }
    static Vc_INTRINSIC __m128i Vc_CONST setmin_epi32() { return _mm_load_si128(reinterpret_cast<const __m128i *>(c_general::signMaskFloat)); }

#if defined(Vc_IMPL_XOP)
    static Vc_INTRINSIC __m128i Vc_CONST cmpgt_epu8(__m128i a, __m128i b) { return _mm_comgt_epu8(a, b); }
    static Vc_INTRINSIC __m128i Vc_CONST cmplt_epu16(__m128i a, __m128i b) { return _mm_comlt_epu16(a, b); }
    static Vc_INTRINSIC __m128i Vc_CONST cmpgt_epu16(__m128i a, __m128i b) { return _mm_comgt_epu16(a, b); }
    static Vc_INTRINSIC __m128i Vc_CONST cmplt_epu32(__m128i a, __m128i b) { return _mm_comlt_epu32(a, b); }
    static Vc_INTRINSIC __m128i Vc_CONST cmpgt_epu32(__m128i a, __m128i b) { return _mm_comgt_epu32(a, b); }
    static Vc_INTRINSIC __m128i Vc_CONST cmplt_epu64(__m128i a, __m128i b) { return _mm_comlt_epu64(a, b); }
#else
    static Vc_INTRINSIC __m128i Vc_CONST cmpgt_epu8(__m128i a, __m128i b)
    {
        return _mm_cmpgt_epi8(_mm_xor_si128(a, setmin_epi8()),
                              _mm_xor_si128(b, setmin_epi8()));
    }
    static Vc_INTRINSIC __m128i Vc_CONST cmplt_epu16(__m128i a, __m128i b)
    {
        return _mm_cmplt_epi16(_mm_xor_si128(a, setmin_epi16()),
                               _mm_xor_si128(b, setmin_epi16()));
    }
    static Vc_INTRINSIC __m128i Vc_CONST cmpgt_epu16(__m128i a, __m128i b)
    {
        return _mm_cmpgt_epi16(_mm_xor_si128(a, setmin_epi16()),
                               _mm_xor_si128(b, setmin_epi16()));
    }
    static Vc_INTRINSIC __m128i Vc_CONST cmplt_epu32(__m128i a, __m128i b)
    {
        return _mm_cmplt_epi32(_mm_xor_si128(a, setmin_epi32()),
                               _mm_xor_si128(b, setmin_epi32()));
    }
    static Vc_INTRINSIC __m128i Vc_CONST cmpgt_epu32(__m128i a, __m128i b)
    {
        return _mm_cmpgt_epi32(_mm_xor_si128(a, setmin_epi32()),
                               _mm_xor_si128(b, setmin_epi32()));
    }
    Vc_INTRINSIC __m128i Vc_CONST cmpgt_epi64(__m128i a, __m128i b)
    {
#ifdef Vc_IMPL_SSE4_2
        return _mm_cmpgt_epi64(a, b);
#else
        const auto aa = _mm_xor_si128(a, _mm_srli_epi64(setmin_epi32(),32));
        const auto bb = _mm_xor_si128(b, _mm_srli_epi64(setmin_epi32(),32));
        const auto gt = _mm_cmpgt_epi32(aa, bb);
        const auto eq = _mm_cmpeq_epi32(aa, bb);
        // Algorithm:
        // 1. if the high 32 bits of gt are true, make the full 64 bits true
        // 2. if the high 32 bits of gt are false and the high 32 bits of eq are true,
        //    duplicate the low 32 bits of gt to the high 32 bits (note that this requires
        //    unsigned compare on the lower 32 bits, which is the reason for the xors
        //    above)
        // 3. else make the full 64 bits false

        const auto gt2 =
            _mm_shuffle_epi32(gt, 0xf5);  // dup the high 32 bits to the low 32 bits
        const auto lo =
            _mm_shuffle_epi32(_mm_and_si128(_mm_srli_epi64(eq, 32), gt), 0xa0);
        return _mm_or_si128(gt2, lo);
#endif
    }
#endif
}  // namespace SseIntrinsics
}  // namespace Vc

// SSSE3
#ifdef Vc_IMPL_SSSE3
namespace Vc_VERSIONED_NAMESPACE
{
namespace SseIntrinsics
{
    // not overriding _mm_set1_epi8 because this one should only be used for non-constants
    Vc_INTRINSIC Vc_CONST __m128i abs_epi8(__m128i a) { return _mm_abs_epi8(a); }
    Vc_INTRINSIC Vc_CONST __m128i abs_epi16(__m128i a) { return _mm_abs_epi16(a); }
    Vc_INTRINSIC Vc_CONST __m128i abs_epi32(__m128i a) { return _mm_abs_epi32(a); }
    template <int s> Vc_INTRINSIC Vc_CONST __m128i alignr_epi8(__m128i a, __m128i b)
    {
        return _mm_alignr_epi8(a, b, s & 0x1fu);
    }
}  // namespace SseIntrinsics
}  // namespace Vc

#else

namespace Vc_VERSIONED_NAMESPACE
{
namespace SseIntrinsics
{
    Vc_INTRINSIC Vc_CONST __m128i abs_epi8 (__m128i a) {
        __m128i negative = _mm_cmplt_epi8 (a, _mm_setzero_si128());
        return _mm_add_epi8 (_mm_xor_si128(a, negative), _mm_and_si128(negative,  _mm_set1_epi8(1)));
    }
    // positive value:
    //   negative == 0
    //   a unchanged after xor
    //   0 >> 31 -> 0
    //   a + 0 -> a
    // negative value:
    //   negative == -1
    //   a xor -1 -> -a - 1
    //   -1 >> 31 -> 1
    //   -a - 1 + 1 -> -a
    Vc_INTRINSIC Vc_CONST __m128i abs_epi16(__m128i a) {
        __m128i negative = _mm_cmplt_epi16(a, _mm_setzero_si128());
        return _mm_add_epi16(_mm_xor_si128(a, negative), _mm_srli_epi16(negative, 15));
    }
    Vc_INTRINSIC Vc_CONST __m128i abs_epi32(__m128i a) {
        __m128i negative = _mm_cmplt_epi32(a, _mm_setzero_si128());
        return _mm_add_epi32(_mm_xor_si128(a, negative), _mm_srli_epi32(negative, 31));
    }
    template <int s> Vc_INTRINSIC Vc_CONST __m128i alignr_epi8(__m128i a, __m128i b)
    {
        switch (s & 0x1fu) {
            case  0: return b;
            case  1: return _mm_or_si128(_mm_slli_si128(a, 15), _mm_srli_si128(b,  1));
            case  2: return _mm_or_si128(_mm_slli_si128(a, 14), _mm_srli_si128(b,  2));
            case  3: return _mm_or_si128(_mm_slli_si128(a, 13), _mm_srli_si128(b,  3));
            case  4: return _mm_or_si128(_mm_slli_si128(a, 12), _mm_srli_si128(b,  4));
            case  5: return _mm_or_si128(_mm_slli_si128(a, 11), _mm_srli_si128(b,  5));
            case  6: return _mm_or_si128(_mm_slli_si128(a, 10), _mm_srli_si128(b,  6));
            case  7: return _mm_or_si128(_mm_slli_si128(a,  9), _mm_srli_si128(b,  7));
            case  8: return _mm_or_si128(_mm_slli_si128(a,  8), _mm_srli_si128(b,  8));
            case  9: return _mm_or_si128(_mm_slli_si128(a,  7), _mm_srli_si128(b,  9));
            case 10: return _mm_or_si128(_mm_slli_si128(a,  6), _mm_srli_si128(b, 10));
            case 11: return _mm_or_si128(_mm_slli_si128(a,  5), _mm_srli_si128(b, 11));
            case 12: return _mm_or_si128(_mm_slli_si128(a,  4), _mm_srli_si128(b, 12));
            case 13: return _mm_or_si128(_mm_slli_si128(a,  3), _mm_srli_si128(b, 13));
            case 14: return _mm_or_si128(_mm_slli_si128(a,  2), _mm_srli_si128(b, 14));
            case 15: return _mm_or_si128(_mm_slli_si128(a,  1), _mm_srli_si128(b, 15));
            case 16: return a;
            case 17: return _mm_srli_si128(a,  1);
            case 18: return _mm_srli_si128(a,  2);
            case 19: return _mm_srli_si128(a,  3);
            case 20: return _mm_srli_si128(a,  4);
            case 21: return _mm_srli_si128(a,  5);
            case 22: return _mm_srli_si128(a,  6);
            case 23: return _mm_srli_si128(a,  7);
            case 24: return _mm_srli_si128(a,  8);
            case 25: return _mm_srli_si128(a,  9);
            case 26: return _mm_srli_si128(a, 10);
            case 27: return _mm_srli_si128(a, 11);
            case 28: return _mm_srli_si128(a, 12);
            case 29: return _mm_srli_si128(a, 13);
            case 30: return _mm_srli_si128(a, 14);
            case 31: return _mm_srli_si128(a, 15);
        }
        return _mm_setzero_si128();
    }
}  // namespace SseIntrinsics
}  // namespace Vc
#endif

// SSE4.1
#ifdef Vc_IMPL_SSE4_1
namespace Vc_VERSIONED_NAMESPACE
{
namespace SseIntrinsics
{
Vc_INTRINSIC Vc_CONST __m128i cmpeq_epi64(__m128i a, __m128i b)
{
    return _mm_cmpeq_epi64(a, b);
}
template <int index> Vc_INTRINSIC Vc_CONST int extract_epi32(__m128i v)
{
    return _mm_extract_epi32(v, index);
}
Vc_INTRINSIC Vc_CONST __m128d blendv_pd(__m128d a, __m128d b, __m128d c)
{
    return _mm_blendv_pd(a, b, c);
}
Vc_INTRINSIC Vc_CONST __m128 blendv_ps(__m128 a, __m128 b, __m128 c)
{
    return _mm_blendv_ps(a, b, c);
}
Vc_INTRINSIC Vc_CONST __m128i blendv_epi8(__m128i a, __m128i b, __m128i c)
{
    return _mm_blendv_epi8(a, b, c);
}
template <int mask> Vc_INTRINSIC Vc_CONST __m128d blend_pd(__m128d a, __m128d b)
{
    return _mm_blend_pd(a, b, mask);
}
template <int mask> Vc_INTRINSIC Vc_CONST __m128 blend_ps(__m128 a, __m128 b)
{
    return _mm_blend_ps(a, b, mask);
}
template <int mask> Vc_INTRINSIC Vc_CONST __m128i blend_epi16(__m128i a, __m128i b)
{
    return _mm_blend_epi16(a, b, mask);
}
Vc_INTRINSIC Vc_CONST __m128i max_epi8(__m128i a, __m128i b)
{
    return _mm_max_epi8(a, b);
}
Vc_INTRINSIC Vc_CONST __m128i max_epi32(__m128i a, __m128i b)
{
    return _mm_max_epi32(a, b);
}
Vc_INTRINSIC Vc_CONST __m128i max_epu16(__m128i a, __m128i b)
{
    return _mm_max_epu16(a, b);
}
Vc_INTRINSIC Vc_CONST __m128i max_epu32(__m128i a, __m128i b)
{
    return _mm_max_epu32(a, b);
}
Vc_INTRINSIC Vc_CONST __m128i min_epu16(__m128i a, __m128i b)
{
    return _mm_min_epu16(a, b);
}
Vc_INTRINSIC Vc_CONST __m128i min_epu32(__m128i a, __m128i b)
{
    return _mm_min_epu32(a, b);
}
Vc_INTRINSIC Vc_CONST __m128i min_epi8(__m128i a, __m128i b)
{
    return _mm_min_epi8(a, b);
}
Vc_INTRINSIC Vc_CONST __m128i min_epi32(__m128i a, __m128i b)
{
    return _mm_min_epi32(a, b);
}
Vc_INTRINSIC Vc_CONST __m128i cvtepu8_epi16(__m128i epu8)
{
    return _mm_cvtepu8_epi16(epu8);
}
Vc_INTRINSIC Vc_CONST __m128i cvtepi8_epi16(__m128i epi8)
{
    return _mm_cvtepi8_epi16(epi8);
}
Vc_INTRINSIC Vc_CONST __m128i cvtepu16_epi32(__m128i epu16)
{
    return _mm_cvtepu16_epi32(epu16);
}
Vc_INTRINSIC Vc_CONST __m128i cvtepi16_epi32(__m128i epu16)
{
    return _mm_cvtepi16_epi32(epu16);
}
Vc_INTRINSIC Vc_CONST __m128i cvtepu8_epi32(__m128i epu8)
{
    return _mm_cvtepu8_epi32(epu8);
}
Vc_INTRINSIC Vc_CONST __m128i cvtepi8_epi32(__m128i epi8)
{
    return _mm_cvtepi8_epi32(epi8);
}
}  // namespace SseIntrinsics
}  // namespace Vc
#else

namespace Vc_VERSIONED_NAMESPACE
{
namespace SseIntrinsics
{
    Vc_INTRINSIC Vc_CONST __m128i cmpeq_epi64(__m128i a, __m128i b) {
        auto tmp = _mm_cmpeq_epi32(a, b);
        return _mm_and_si128(tmp, _mm_shuffle_epi32(tmp, 1*1 + 0*4 + 3*16 + 2*64));
    }
    template <int index> Vc_INTRINSIC Vc_CONST int extract_epi32(__m128i v)
    {
#ifdef Vc_USE_BUILTIN_VECTOR_TYPES
        typedef int int32v4 __attribute__((__vector_size__(16)));
        return aliasing_cast<int32v4>(v)[index];
#else
        return _mm_cvtsi128_si32(_mm_srli_si128(v, index * 4));
#endif
    }
    Vc_INTRINSIC Vc_CONST __m128d blendv_pd(__m128d a, __m128d b, __m128d c) {
#ifdef Vc_GCC
        return reinterpret_cast<__m128d>(
            (~reinterpret_cast<__m128i>(c) & reinterpret_cast<__m128i>(a)) |
            (reinterpret_cast<__m128i>(c) & reinterpret_cast<__m128i>(b)));
#else
        return _mm_or_pd(_mm_andnot_pd(c, a), _mm_and_pd(c, b));
#endif
    }
    Vc_INTRINSIC Vc_CONST __m128  blendv_ps(__m128  a, __m128  b, __m128  c) {
#ifdef Vc_GCC
        return reinterpret_cast<__m128>(
            (~reinterpret_cast<__m128i>(c) & reinterpret_cast<__m128i>(a)) |
            (reinterpret_cast<__m128i>(c) & reinterpret_cast<__m128i>(b)));
#else
        return _mm_or_ps(_mm_andnot_ps(c, a), _mm_and_ps(c, b));
#endif
    }
    Vc_INTRINSIC Vc_CONST __m128i blendv_epi8(__m128i a, __m128i b, __m128i c) {
#ifdef Vc_GCC
        return (~c & a) | (c & b);
#else
        return _mm_or_si128(_mm_andnot_si128(c, a), _mm_and_si128(c, b));
#endif
    }

    // only use the following blend functions with immediates as mask and, of course, compiling
    // with optimization
    template <int mask> Vc_INTRINSIC Vc_CONST __m128d blend_pd(__m128d a, __m128d b)
    {
        switch (mask) {
        case 0x0:
            return a;
        case 0x1:
            return _mm_shuffle_pd(b, a, 2);
        case 0x2:
            return _mm_shuffle_pd(a, b, 2);
        case 0x3:
            return b;
        default:
            abort();
            return a; // should never be reached, but MSVC needs it else it warns about 'not all control paths return a value'
        }
    }
    template <int mask> Vc_INTRINSIC Vc_CONST __m128 blend_ps(__m128 a, __m128 b)
    {
        __m128i c;
        switch (mask) {
        case 0x0:
            return a;
        case 0x1:
            c = _mm_srli_si128(_mm_setallone_si128(), 12);
            break;
        case 0x2:
            c = _mm_slli_si128(_mm_srli_si128(_mm_setallone_si128(), 12), 4);
            break;
        case 0x3:
            c = _mm_srli_si128(_mm_setallone_si128(), 8);
            break;
        case 0x4:
            c = _mm_slli_si128(_mm_srli_si128(_mm_setallone_si128(), 12), 8);
            break;
        case 0x5:
            c = _mm_set_epi32(0, -1, 0, -1);
            break;
        case 0x6:
            c = _mm_slli_si128(_mm_srli_si128(_mm_setallone_si128(), 8), 4);
            break;
        case 0x7:
            c = _mm_srli_si128(_mm_setallone_si128(), 4);
            break;
        case 0x8:
            c = _mm_slli_si128(_mm_setallone_si128(), 12);
            break;
        case 0x9:
            c = _mm_set_epi32(-1, 0, 0, -1);
            break;
        case 0xa:
            c = _mm_set_epi32(-1, 0, -1, 0);
            break;
        case 0xb:
            c = _mm_set_epi32(-1, 0, -1, -1);
            break;
        case 0xc:
            c = _mm_slli_si128(_mm_setallone_si128(), 8);
            break;
        case 0xd:
            c = _mm_set_epi32(-1, -1, 0, -1);
            break;
        case 0xe:
            c = _mm_slli_si128(_mm_setallone_si128(), 4);
            break;
        case 0xf:
            return b;
        default: // may not happen
            abort();
            c = _mm_setzero_si128();
            break;
        }
        __m128 _c = _mm_castsi128_ps(c);
        return _mm_or_ps(_mm_andnot_ps(_c, a), _mm_and_ps(_c, b));
    }
    template <int mask> Vc_INTRINSIC Vc_CONST __m128i blend_epi16(__m128i a, __m128i b)
    {
        __m128i c;
        switch (mask) {
        case 0x00:
            return a;
        case 0x01:
            c = _mm_srli_si128(_mm_setallone_si128(), 14);
            break;
        case 0x03:
            c = _mm_srli_si128(_mm_setallone_si128(), 12);
            break;
        case 0x07:
            c = _mm_srli_si128(_mm_setallone_si128(), 10);
            break;
        case 0x0f:
            return _mm_unpackhi_epi64(_mm_slli_si128(b, 8), a);
        case 0x1f:
            c = _mm_srli_si128(_mm_setallone_si128(), 6);
            break;
        case 0x3f:
            c = _mm_srli_si128(_mm_setallone_si128(), 4);
            break;
        case 0x7f:
            c = _mm_srli_si128(_mm_setallone_si128(), 2);
            break;
        case 0x80:
            c = _mm_slli_si128(_mm_setallone_si128(), 14);
            break;
        case 0xc0:
            c = _mm_slli_si128(_mm_setallone_si128(), 12);
            break;
        case 0xe0:
            c = _mm_slli_si128(_mm_setallone_si128(), 10);
            break;
        case 0xf0:
            c = _mm_slli_si128(_mm_setallone_si128(), 8);
            break;
        case 0xf8:
            c = _mm_slli_si128(_mm_setallone_si128(), 6);
            break;
        case 0xfc:
            c = _mm_slli_si128(_mm_setallone_si128(), 4);
            break;
        case 0xfe:
            c = _mm_slli_si128(_mm_setallone_si128(), 2);
            break;
        case 0xff:
            return b;
        case 0xcc:
            return _mm_unpacklo_epi32(_mm_shuffle_epi32(a, _MM_SHUFFLE(2, 0, 2, 0)), _mm_shuffle_epi32(b, _MM_SHUFFLE(3, 1, 3, 1)));
        case 0x33:
            return _mm_unpacklo_epi32(_mm_shuffle_epi32(b, _MM_SHUFFLE(2, 0, 2, 0)), _mm_shuffle_epi32(a, _MM_SHUFFLE(3, 1, 3, 1)));
        default:
            const __m128i shift = _mm_set_epi16(0x0100, 0x0200, 0x0400, 0x0800, 0x1000, 0x2000, 0x4000, -0x7fff);
            c = _mm_srai_epi16(_mm_mullo_epi16(_mm_set1_epi16(mask), shift), 15);
            break;
        }
        return _mm_or_si128(_mm_andnot_si128(c, a), _mm_and_si128(c, b));
    }

    Vc_INTRINSIC Vc_CONST __m128i max_epi8 (__m128i a, __m128i b) {
        return blendv_epi8(b, a, _mm_cmpgt_epi8 (a, b));
    }
    Vc_INTRINSIC Vc_CONST __m128i max_epi32(__m128i a, __m128i b) {
        return blendv_epi8(b, a, _mm_cmpgt_epi32(a, b));
    }
    Vc_INTRINSIC Vc_CONST __m128i max_epu16(__m128i a, __m128i b) {
        return blendv_epi8(b, a, cmpgt_epu16(a, b));
    }
    Vc_INTRINSIC Vc_CONST __m128i max_epu32(__m128i a, __m128i b) {
        return blendv_epi8(b, a, cmpgt_epu32(a, b));
    }
    Vc_INTRINSIC Vc_CONST __m128i min_epu16(__m128i a, __m128i b) {
        return blendv_epi8(a, b, cmpgt_epu16(a, b));
    }
    Vc_INTRINSIC Vc_CONST __m128i min_epu32(__m128i a, __m128i b) {
        return blendv_epi8(a, b, cmpgt_epu32(a, b));
    }
    Vc_INTRINSIC Vc_CONST __m128i min_epi8 (__m128i a, __m128i b) {
        return blendv_epi8(a, b, _mm_cmpgt_epi8 (a, b));
    }
    Vc_INTRINSIC Vc_CONST __m128i min_epi32(__m128i a, __m128i b) {
        return blendv_epi8(a, b, _mm_cmpgt_epi32(a, b));
    }
    Vc_INTRINSIC Vc_CONST __m128i cvtepu8_epi16(__m128i epu8) {
        return _mm_unpacklo_epi8(epu8, _mm_setzero_si128());
    }
    Vc_INTRINSIC Vc_CONST __m128i cvtepi8_epi16(__m128i epi8) {
        return _mm_unpacklo_epi8(epi8, _mm_cmplt_epi8(epi8, _mm_setzero_si128()));
    }
    Vc_INTRINSIC Vc_CONST __m128i cvtepu16_epi32(__m128i epu16) {
        return _mm_unpacklo_epi16(epu16, _mm_setzero_si128());
    }
    Vc_INTRINSIC Vc_CONST __m128i cvtepi16_epi32(__m128i epu16) {
        return _mm_unpacklo_epi16(epu16, _mm_cmplt_epi16(epu16, _mm_setzero_si128()));
    }
    Vc_INTRINSIC Vc_CONST __m128i cvtepu8_epi32(__m128i epu8) {
        return cvtepu16_epi32(cvtepu8_epi16(epu8));
    }
    Vc_INTRINSIC Vc_CONST __m128i cvtepi8_epi32(__m128i epi8) {
        const __m128i neg = _mm_cmplt_epi8(epi8, _mm_setzero_si128());
        const __m128i epi16 = _mm_unpacklo_epi8(epi8, neg);
        return _mm_unpacklo_epi16(epi16, _mm_unpacklo_epi8(neg, neg));
    }
}  // namespace SseIntrinsics
}  // namespace Vc
#endif

// SSE4.2
namespace Vc_VERSIONED_NAMESPACE
{
namespace SseIntrinsics
{
    static Vc_INTRINSIC Vc_PURE __m128  _mm_stream_load(const float *mem) {
#ifdef Vc_IMPL_SSE4_1
        return _mm_castsi128_ps(_mm_stream_load_si128(reinterpret_cast<__m128i *>(const_cast<float *>(mem))));
#else
        return _mm_load_ps(mem);
#endif
    }
    static Vc_INTRINSIC Vc_PURE __m128d _mm_stream_load(const double *mem) {
#ifdef Vc_IMPL_SSE4_1
        return _mm_castsi128_pd(_mm_stream_load_si128(reinterpret_cast<__m128i *>(const_cast<double *>(mem))));
#else
        return _mm_load_pd(mem);
#endif
    }
    static Vc_INTRINSIC Vc_PURE __m128i _mm_stream_load(const int *mem) {
#ifdef Vc_IMPL_SSE4_1
        return _mm_stream_load_si128(reinterpret_cast<__m128i *>(const_cast<int *>(mem)));
#else
        return _mm_load_si128(reinterpret_cast<const __m128i *>(mem));
#endif
    }
    static Vc_INTRINSIC Vc_PURE __m128i _mm_stream_load(const unsigned int *mem) {
        return _mm_stream_load(reinterpret_cast<const int *>(mem));
    }
    static Vc_INTRINSIC Vc_PURE __m128i _mm_stream_load(const short *mem) {
        return _mm_stream_load(reinterpret_cast<const int *>(mem));
    }
    static Vc_INTRINSIC Vc_PURE __m128i _mm_stream_load(const unsigned short *mem) {
        return _mm_stream_load(reinterpret_cast<const int *>(mem));
    }
    static Vc_INTRINSIC Vc_PURE __m128i _mm_stream_load(const signed char *mem) {
        return _mm_stream_load(reinterpret_cast<const int *>(mem));
    }
    static Vc_INTRINSIC Vc_PURE __m128i _mm_stream_load(const unsigned char *mem) {
        return _mm_stream_load(reinterpret_cast<const int *>(mem));
    }

#ifndef __x86_64__
    Vc_INTRINSIC Vc_PURE __m128i _mm_cvtsi64_si128(int64_t x) {
        return _mm_castpd_si128(_mm_load_sd(reinterpret_cast<const double *>(&x)));
    }
#endif

#ifdef Vc_IMPL_AVX2
template <int Scale> __m128 gather(const float *addr, __m128i idx)
{
    return _mm_i32gather_ps(addr, idx, Scale);
}
template <int Scale> __m128d gather(const double *addr, __m128i idx)
{
    return _mm_i32gather_pd(addr, idx, Scale);
}
template <int Scale> __m128i gather(const int *addr, __m128i idx)
{
    return _mm_i32gather_epi32(addr, idx, Scale);
}
template <int Scale> __m128i gather(const unsigned *addr, __m128i idx)
{
    return _mm_i32gather_epi32(aliasing_cast<int>(addr), idx, Scale);
}

template <int Scale> __m128 gather(__m128 src, __m128 k, const float *addr, __m128i idx)
{
    return _mm_mask_i32gather_ps(src, addr, idx, k, Scale);
}
template <int Scale>
__m128d gather(__m128d src, __m128d k, const double *addr, __m128i idx)
{
    return _mm_mask_i32gather_pd(src, addr, idx, k, Scale);
}
template <int Scale> __m128i gather(__m128i src, __m128i k, const int *addr, __m128i idx)
{
    return _mm_mask_i32gather_epi32(src, addr, idx, k, Scale);
}
template <int Scale>
__m128i gather(__m128i src, __m128i k, const unsigned *addr, __m128i idx)
{
    return _mm_mask_i32gather_epi32(src, aliasing_cast<int>(addr), idx, k, Scale);
}
#endif

}  // namespace SseIntrinsics
}  // namespace Vc

namespace Vc_VERSIONED_NAMESPACE
{
namespace SSE
{
using namespace SseIntrinsics;

template <typename T> struct ParameterHelper
{
    typedef T ByValue;
    typedef T &Reference;
    typedef const T &ConstRef;
};

template <typename T> struct VectorHelper
{
};

template <typename T> struct VectorTypeHelper
{
    typedef __m128i Type;
};
template <> struct VectorTypeHelper<double>
{
    typedef __m128d Type;
};
template <> struct VectorTypeHelper<float>
{
    typedef __m128 Type;
};

template <typename T> struct DetermineGatherMask
{
    typedef T Type;
};

template <typename T> struct VectorTraits
{
    typedef typename VectorTypeHelper<T>::Type VectorType;
    using EntryType = T;
    static constexpr size_t Size = sizeof(VectorType) / sizeof(EntryType);
    typedef Mask<T> MaskType;
    typedef typename DetermineGatherMask<MaskType>::Type GatherMaskType;
    typedef Common::VectorMemoryUnion<VectorType, EntryType> StorageType;
};

template <typename T> struct VectorHelperSize;
}  // namespace SSE
}  // namespace Vc

#if defined(Vc_GCC) && !defined(__OPTIMIZE__)
#pragma GCC diagnostic pop
#endif

#include "shuffle.h"

#endif // VC_SSE_INTRINSICS_H_
