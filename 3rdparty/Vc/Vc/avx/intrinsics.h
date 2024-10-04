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

#ifndef VC_AVX_INTRINSICS_H_
#define VC_AVX_INTRINSICS_H_

#include "../global.h"
#include "../traits/type_traits.h"

// see comment in sse/intrinsics.h
extern "C" {
// AVX
#include <immintrin.h>

#if (defined(Vc_IMPL_XOP) || defined(Vc_IMPL_FMA4)) && !defined(Vc_MSVC)
#include <x86intrin.h>
#endif
}

#include "../common/fix_clang_emmintrin.h"

#include "const_data.h"
#include "../common/types.h"
#include "macros.h"
#include <cstdlib>

#if (defined Vc_CLANG && Vc_CLANG >= 0x30900 && Vc_CLANG < 0x70000)
#ifdef _mm256_permute2f128_si256
#undef _mm256_permute2f128_si256
#define _mm256_permute2f128_si256(V1, V2, M) __extension__ ({ \
  (__m256i)__builtin_ia32_vperm2f128_si256((__v8si)(__m256i)(V1), \
                                           (__v8si)(__m256i)(V2), (char)(M)); })
#endif

#ifdef _mm256_permute2f128_ps
#undef _mm256_permute2f128_ps
#define _mm256_permute2f128_ps(V1, V2, M) __extension__ ({ \
  (__m256)__builtin_ia32_vperm2f128_ps256((__v8sf)(__m256)(V1), \
                                          (__v8sf)(__m256)(V2), (char)(M)); })
#endif

#ifdef _mm256_permute2x128_si256
#undef _mm256_permute2x128_si256
#define _mm256_permute2x128_si256(V1, V2, M) __extension__ ({ \
  (__m256i)__builtin_ia32_permti256((__m256i)(V1), (__m256i)(V2), (char)(M)); })
#endif
#endif

namespace Vc_VERSIONED_NAMESPACE
{
namespace AvxIntrinsics
{
    using AVX::c_general;
    using AVX::_IndexesFromZero32;
    using AVX::_IndexesFromZero16;
    using AVX::_IndexesFromZero8;

    typedef __m128  m128 ;
    typedef __m128d m128d;
    typedef __m128i m128i;
    typedef __m256  m256 ;
    typedef __m256d m256d;
    typedef __m256i m256i;

#ifdef Vc_GCC
    // Redefine the mul/add/sub intrinsics to use GCC-specific operators instead of builtin
    // functions. This way the fp-contraction optimization step kicks in and creates FMAs! :)
    static Vc_INTRINSIC Vc_CONST m256d _mm256_mul_pd(m256d a, m256d b) { return static_cast<m256d>(static_cast<__v4df>(a) * static_cast<__v4df>(b)); }
    static Vc_INTRINSIC Vc_CONST m256d _mm256_add_pd(m256d a, m256d b) { return static_cast<m256d>(static_cast<__v4df>(a) + static_cast<__v4df>(b)); }
    static Vc_INTRINSIC Vc_CONST m256d _mm256_sub_pd(m256d a, m256d b) { return static_cast<m256d>(static_cast<__v4df>(a) - static_cast<__v4df>(b)); }
    static Vc_INTRINSIC Vc_CONST m256 _mm256_mul_ps(m256 a, m256 b) { return static_cast<m256>(static_cast<__v8sf>(a) * static_cast<__v8sf>(b)); }
    static Vc_INTRINSIC Vc_CONST m256 _mm256_add_ps(m256 a, m256 b) { return static_cast<m256>(static_cast<__v8sf>(a) + static_cast<__v8sf>(b)); }
    static Vc_INTRINSIC Vc_CONST m256 _mm256_sub_ps(m256 a, m256 b) { return static_cast<m256>(static_cast<__v8sf>(a) - static_cast<__v8sf>(b)); }
#endif

    static Vc_INTRINSIC m256d Vc_CONST set1_pd   (double a) { return _mm256_set1_pd   (a); }
    static Vc_INTRINSIC m256i Vc_CONST set1_epi32(int    a) { return _mm256_set1_epi32(a); }

    static Vc_INTRINSIC Vc_CONST m128i _mm_setallone_si128() { return _mm_load_si128(reinterpret_cast<const __m128i *>(Common::AllBitsSet)); }
    static Vc_INTRINSIC Vc_CONST m128  _mm_setallone_ps() { return _mm_load_ps(reinterpret_cast<const float *>(Common::AllBitsSet)); }
    static Vc_INTRINSIC Vc_CONST m128d _mm_setallone_pd() { return _mm_load_pd(reinterpret_cast<const double *>(Common::AllBitsSet)); }

    static Vc_INTRINSIC Vc_CONST m256i setallone_si256() { return _mm256_castps_si256(_mm256_load_ps(reinterpret_cast<const float *>(Common::AllBitsSet))); }
    static Vc_INTRINSIC Vc_CONST m256d setallone_pd() { return _mm256_load_pd(reinterpret_cast<const double *>(Common::AllBitsSet)); }
    static Vc_INTRINSIC Vc_CONST m256  setallone_ps() { return _mm256_load_ps(reinterpret_cast<const float *>(Common::AllBitsSet)); }

    static Vc_INTRINSIC m256i Vc_CONST setone_epi8 ()  { return _mm256_set1_epi8(1); }
    static Vc_INTRINSIC m256i Vc_CONST setone_epu8 ()  { return setone_epi8(); }
    static Vc_INTRINSIC m256i Vc_CONST setone_epi16()  { return _mm256_castps_si256(_mm256_broadcast_ss(reinterpret_cast<const float *>(c_general::one16))); }
    static Vc_INTRINSIC m256i Vc_CONST setone_epu16()  { return setone_epi16(); }
    static Vc_INTRINSIC m256i Vc_CONST setone_epi32()  { return _mm256_castps_si256(_mm256_broadcast_ss(reinterpret_cast<const float *>(&_IndexesFromZero32[1]))); }
    static Vc_INTRINSIC m256i Vc_CONST setone_epu32()  { return setone_epi32(); }

    static Vc_INTRINSIC m256  Vc_CONST setone_ps()     { return _mm256_broadcast_ss(&c_general::oneFloat); }
    static Vc_INTRINSIC m256d Vc_CONST setone_pd()     { return _mm256_broadcast_sd(&c_general::oneDouble); }

    static Vc_INTRINSIC m256d Vc_CONST setabsmask_pd() { return _mm256_broadcast_sd(reinterpret_cast<const double *>(&c_general::absMaskFloat[0])); }
    static Vc_INTRINSIC m256  Vc_CONST setabsmask_ps() { return _mm256_broadcast_ss(reinterpret_cast<const float *>(&c_general::absMaskFloat[1])); }
    static Vc_INTRINSIC m256d Vc_CONST setsignmask_pd(){ return _mm256_broadcast_sd(reinterpret_cast<const double *>(&c_general::signMaskFloat[0])); }
    static Vc_INTRINSIC m256  Vc_CONST setsignmask_ps(){ return _mm256_broadcast_ss(reinterpret_cast<const float *>(&c_general::signMaskFloat[1])); }

    static Vc_INTRINSIC m256  Vc_CONST set2power31_ps()    { return _mm256_broadcast_ss(&c_general::_2power31); }
    static Vc_INTRINSIC m128  Vc_CONST _mm_set2power31_ps()    { return _mm_broadcast_ss(&c_general::_2power31); }
    static Vc_INTRINSIC m256i Vc_CONST set2power31_epu32() { return _mm256_castps_si256(_mm256_broadcast_ss(reinterpret_cast<const float *>(&c_general::signMaskFloat[1]))); }
    static Vc_INTRINSIC m128i Vc_CONST _mm_set2power31_epu32() { return _mm_castps_si128(_mm_broadcast_ss(reinterpret_cast<const float *>(&c_general::signMaskFloat[1]))); }

    static Vc_INTRINSIC m256i Vc_CONST setmin_epi8 () { return _mm256_set1_epi8(-0x80); }
    static Vc_INTRINSIC m128i Vc_CONST _mm_setmin_epi16() { return _mm_castps_si128(_mm_broadcast_ss(reinterpret_cast<const float *>(c_general::minShort))); }
    static Vc_INTRINSIC m128i Vc_CONST _mm_setmin_epi32() { return _mm_castps_si128(_mm_broadcast_ss(reinterpret_cast<const float *>(&c_general::signMaskFloat[1]))); }
    static Vc_INTRINSIC m256i Vc_CONST setmin_epi16() { return _mm256_castps_si256(_mm256_broadcast_ss(reinterpret_cast<const float *>(c_general::minShort))); }
    static Vc_INTRINSIC m256i Vc_CONST setmin_epi32() { return _mm256_castps_si256(_mm256_broadcast_ss(reinterpret_cast<const float *>(&c_general::signMaskFloat[1]))); }

    template <int i>
    static Vc_INTRINSIC Vc_CONST unsigned int extract_epu32(__m128i x)
    {
        return _mm_extract_epi32(x, i);
    }

    template <int offset> Vc_INTRINSIC __m256  insert128(__m256  a, __m128  b) { return _mm256_insertf128_ps(a, b, offset); }
    template <int offset> Vc_INTRINSIC __m256d insert128(__m256d a, __m128d b) { return _mm256_insertf128_pd(a, b, offset); }
    template <int offset> Vc_INTRINSIC __m256i insert128(__m256i a, __m128i b) {
#ifdef Vc_IMPL_AVX2
        return _mm256_inserti128_si256(a, b, offset);
#else
        return _mm256_insertf128_si256(a, b, offset);
#endif
    }

    template <int offset> Vc_INTRINSIC __m128  extract128(__m256  a) { return _mm256_extractf128_ps(a, offset); }
    template <int offset> Vc_INTRINSIC __m128d extract128(__m256d a) { return _mm256_extractf128_pd(a, offset); }
    template <int offset> Vc_INTRINSIC __m128i extract128(__m256i a) {
#ifdef Vc_IMPL_AVX2
        return _mm256_extracti128_si256(a, offset);
#else
        return _mm256_extractf128_si256(a, offset);
#endif
    }

    /////////////////////// COMPARE OPS ///////////////////////
#ifdef Vc_GCC
    // GCC needs builtin compare operators to enable constant folding
    Vc_INTRINSIC __m256d cmpeq_pd   (__m256d a, __m256d b) { return reinterpret_cast<__m256d>(a == b); }
    Vc_INTRINSIC __m256d cmpneq_pd  (__m256d a, __m256d b) { return reinterpret_cast<__m256d>(a != b); }
    Vc_INTRINSIC __m256d cmplt_pd   (__m256d a, __m256d b) { return reinterpret_cast<__m256d>(a < b); }
    Vc_INTRINSIC __m256d cmpge_pd   (__m256d a, __m256d b) { return reinterpret_cast<__m256d>(a >= b); }
    Vc_INTRINSIC __m256d cmple_pd   (__m256d a, __m256d b) { return reinterpret_cast<__m256d>(a <= b); }
    Vc_INTRINSIC __m256d cmpgt_pd   (__m256d a, __m256d b) { return reinterpret_cast<__m256d>(a > b); }

    Vc_INTRINSIC __m256  cmpeq_ps   (__m256  a, __m256  b) { return reinterpret_cast<__m256 >(a == b); }
    Vc_INTRINSIC __m256  cmpneq_ps  (__m256  a, __m256  b) { return reinterpret_cast<__m256 >(a != b); }
    Vc_INTRINSIC __m256  cmplt_ps   (__m256  a, __m256  b) { return reinterpret_cast<__m256 >(a < b); }
    Vc_INTRINSIC __m256  cmpge_ps   (__m256  a, __m256  b) { return reinterpret_cast<__m256 >(a >= b); }
    Vc_INTRINSIC __m256  cmple_ps   (__m256  a, __m256  b) { return reinterpret_cast<__m256 >(a <= b); }
    Vc_INTRINSIC __m256  cmpgt_ps   (__m256  a, __m256  b) { return reinterpret_cast<__m256 >(a > b); }
#else
    Vc_INTRINSIC __m256d cmpeq_pd   (__m256d a, __m256d b) { return _mm256_cmp_pd(a, b, _CMP_EQ_OQ); }
    Vc_INTRINSIC __m256d cmpneq_pd  (__m256d a, __m256d b) { return _mm256_cmp_pd(a, b, _CMP_NEQ_UQ); }
    Vc_INTRINSIC __m256d cmplt_pd   (__m256d a, __m256d b) { return _mm256_cmp_pd(a, b, _CMP_LT_OS); }
    Vc_INTRINSIC __m256d cmpge_pd   (__m256d a, __m256d b) { return _mm256_cmp_pd(a, b, _CMP_NLT_US); }
    Vc_INTRINSIC __m256d cmple_pd   (__m256d a, __m256d b) { return _mm256_cmp_pd(a, b, _CMP_LE_OS); }
    Vc_INTRINSIC __m256d cmpgt_pd   (__m256d a, __m256d b) { return _mm256_cmp_pd(a, b, _CMP_NLE_US); }

    Vc_INTRINSIC __m256  cmpeq_ps   (__m256  a, __m256  b) { return _mm256_cmp_ps(a, b, _CMP_EQ_OQ); }
    Vc_INTRINSIC __m256  cmpneq_ps  (__m256  a, __m256  b) { return _mm256_cmp_ps(a, b, _CMP_NEQ_UQ); }
    Vc_INTRINSIC __m256  cmplt_ps   (__m256  a, __m256  b) { return _mm256_cmp_ps(a, b, _CMP_LT_OS); }
    Vc_INTRINSIC __m256  cmpge_ps   (__m256  a, __m256  b) { return _mm256_cmp_ps(a, b, _CMP_NLT_US); }
    Vc_INTRINSIC __m256  cmple_ps   (__m256  a, __m256  b) { return _mm256_cmp_ps(a, b, _CMP_LE_OS); }
    Vc_INTRINSIC __m256  cmpgt_ps   (__m256  a, __m256  b) { return _mm256_cmp_ps(a, b, _CMP_NLE_US); }
#endif
    Vc_INTRINSIC __m256d cmpnlt_pd  (__m256d a, __m256d b) { return cmpge_pd(a, b); }
    Vc_INTRINSIC __m256d cmpnle_pd  (__m256d a, __m256d b) { return cmpgt_pd(a, b); }
    Vc_INTRINSIC __m256  cmpnlt_ps  (__m256  a, __m256  b) { return cmpge_ps(a, b); }
    Vc_INTRINSIC __m256  cmpnle_ps  (__m256  a, __m256  b) { return cmpgt_ps(a, b); }

    Vc_INTRINSIC __m256d cmpord_pd  (__m256d a, __m256d b) { return _mm256_cmp_pd(a, b, _CMP_ORD_Q); }
    Vc_INTRINSIC __m256d cmpunord_pd(__m256d a, __m256d b) { return _mm256_cmp_pd(a, b, _CMP_UNORD_Q); }
    Vc_INTRINSIC __m256  cmpord_ps  (__m256  a, __m256  b) { return _mm256_cmp_ps(a, b, _CMP_ORD_Q); }
    Vc_INTRINSIC __m256  cmpunord_ps(__m256  a, __m256  b) { return _mm256_cmp_ps(a, b, _CMP_UNORD_Q); }

#if defined(Vc_IMPL_XOP)
    static Vc_INTRINSIC m128i cmplt_epu16(__m128i a, __m128i b) {
        return _mm_comlt_epu16(a, b);
    }
    static Vc_INTRINSIC m128i cmpgt_epu16(__m128i a, __m128i b) {
        return _mm_comgt_epu16(a, b);
    }
#else
    static Vc_INTRINSIC m128i cmplt_epu16(__m128i a, __m128i b) {
        return _mm_cmplt_epi16(_mm_xor_si128(a, _mm_setmin_epi16()), _mm_xor_si128(b, _mm_setmin_epi16()));
    }
    static Vc_INTRINSIC m128i cmpgt_epu16(__m128i a, __m128i b) {
        return _mm_cmpgt_epi16(_mm_xor_si128(a, _mm_setmin_epi16()), _mm_xor_si128(b, _mm_setmin_epi16()));
    }
#endif

#ifdef Vc_IMPL_AVX2
    template <int shift> Vc_INTRINSIC Vc_CONST m256i alignr(__m256i s1, __m256i s2)
    {
        return _mm256_alignr_epi8(s1, s2, shift);
    }
#else
    template <int shift> Vc_INTRINSIC Vc_CONST m256i alignr(__m256i s1, __m256i s2)
    {
        return insert128<1>(
            _mm256_castsi128_si256(_mm_alignr_epi8(_mm256_castsi256_si128(s1),
                                                   _mm256_castsi256_si128(s2), shift)),
            _mm_alignr_epi8(extract128<1>(s1), extract128<1>(s2), shift));
    }
#endif

#ifdef Vc_IMPL_AVX2
#define Vc_AVX_TO_SSE_2_NEW(name)                                                        \
    Vc_INTRINSIC Vc_CONST m256i name(__m256i a0, __m256i b0)                             \
    {                                                                                    \
        return _mm256_##name(a0, b0);                                                    \
    }
#define Vc_AVX_TO_SSE_256_128(name)                                                      \
    Vc_INTRINSIC Vc_CONST m256i name(__m256i a0, __m128i b0)                             \
    {                                                                                    \
        return _mm256_##name(a0, b0);                                                    \
    }
#define Vc_AVX_TO_SSE_1i(name)                                                           \
    template <int i> Vc_INTRINSIC Vc_CONST m256i name(__m256i a0)                        \
    {                                                                                    \
        return _mm256_##name(a0, i);                                                     \
    }
#define Vc_AVX_TO_SSE_1(name)                                                            \
    Vc_INTRINSIC Vc_CONST __m256i name(__m256i a0) { return _mm256_##name(a0); }
#define Vc_AVX_TO_SSE_1_128(name, shift__)                                               \
    Vc_INTRINSIC Vc_CONST __m256i name(__m128i a0) { return _mm256_##name(a0); }
#else
/**\internal
 * Defines the function \p name, which takes to __m256i arguments and calls `_mm_##name` on the low
 * and high 128 bit halfs of the arguments.
 *
 * In case the AVX2 intrinsics are enabled, the arguments are directly passed to a single
 * `_mm256_##name` call.
 */
#define Vc_AVX_TO_SSE_1(name)                                                            \
    Vc_INTRINSIC Vc_CONST __m256i name(__m256i a0)                                       \
    {                                                                                    \
        __m128i a1 = extract128<1>(a0);                                                  \
        __m128i r0 = _mm_##name(_mm256_castsi256_si128(a0));                             \
        __m128i r1 = _mm_##name(a1);                                                     \
        return insert128<1>(_mm256_castsi128_si256(r0), r1);                             \
    }
#define Vc_AVX_TO_SSE_1_128(name, shift__)                                               \
    Vc_INTRINSIC Vc_CONST __m256i name(__m128i a0)                                       \
    {                                                                                    \
        __m128i r0 = _mm_##name(a0);                                                     \
        __m128i r1 = _mm_##name(_mm_srli_si128(a0, shift__));                            \
        return insert128<1>(_mm256_castsi128_si256(r0), r1);                             \
    }
#define Vc_AVX_TO_SSE_2_NEW(name)                                                        \
    Vc_INTRINSIC Vc_CONST m256i name(__m256i a0, __m256i b0)                             \
    {                                                                                    \
        m128i a1 = extract128<1>(a0);                                                    \
        m128i b1 = extract128<1>(b0);                                                    \
        m128i r0 = _mm_##name(_mm256_castsi256_si128(a0), _mm256_castsi256_si128(b0));   \
        m128i r1 = _mm_##name(a1, b1);                                                   \
        return insert128<1>(_mm256_castsi128_si256(r0), r1);                             \
    }
#define Vc_AVX_TO_SSE_256_128(name)                                                      \
    Vc_INTRINSIC Vc_CONST m256i name(__m256i a0, __m128i b0)                             \
    {                                                                                    \
        m128i a1 = extract128<1>(a0);                                                    \
        m128i r0 = _mm_##name(_mm256_castsi256_si128(a0), b0);                           \
        m128i r1 = _mm_##name(a1, b0);                                                   \
        return insert128<1>(_mm256_castsi128_si256(r0), r1);                             \
    }
#define Vc_AVX_TO_SSE_1i(name)                                                           \
    template <int i> Vc_INTRINSIC Vc_CONST m256i name(__m256i a0)                        \
    {                                                                                    \
        m128i a1 = extract128<1>(a0);                                                    \
        m128i r0 = _mm_##name(_mm256_castsi256_si128(a0), i);                            \
        m128i r1 = _mm_##name(a1, i);                                                    \
        return insert128<1>(_mm256_castsi128_si256(r0), r1);                             \
    }
#endif
    Vc_INTRINSIC Vc_CONST __m128i sll_epi16(__m128i a, __m128i b) { return _mm_sll_epi16(a, b); }
    Vc_INTRINSIC Vc_CONST __m128i sll_epi32(__m128i a, __m128i b) { return _mm_sll_epi32(a, b); }
    Vc_INTRINSIC Vc_CONST __m128i sll_epi64(__m128i a, __m128i b) { return _mm_sll_epi64(a, b); }
    Vc_INTRINSIC Vc_CONST __m128i srl_epi16(__m128i a, __m128i b) { return _mm_srl_epi16(a, b); }
    Vc_INTRINSIC Vc_CONST __m128i srl_epi32(__m128i a, __m128i b) { return _mm_srl_epi32(a, b); }
    Vc_INTRINSIC Vc_CONST __m128i srl_epi64(__m128i a, __m128i b) { return _mm_srl_epi64(a, b); }
    Vc_INTRINSIC Vc_CONST __m128i sra_epi16(__m128i a, __m128i b) { return _mm_sra_epi16(a, b); }
    Vc_INTRINSIC Vc_CONST __m128i sra_epi32(__m128i a, __m128i b) { return _mm_sra_epi32(a, b); }

    Vc_AVX_TO_SSE_1i(slli_epi16)
    Vc_AVX_TO_SSE_1i(slli_epi32)
    Vc_AVX_TO_SSE_1i(slli_epi64)
    Vc_AVX_TO_SSE_1i(srai_epi16)
    Vc_AVX_TO_SSE_1i(srai_epi32)
    Vc_AVX_TO_SSE_1i(srli_epi16)
    Vc_AVX_TO_SSE_1i(srli_epi32)
    Vc_AVX_TO_SSE_1i(srli_epi64)

    Vc_AVX_TO_SSE_256_128(sll_epi16)
    Vc_AVX_TO_SSE_256_128(sll_epi32)
    Vc_AVX_TO_SSE_256_128(sll_epi64)
    Vc_AVX_TO_SSE_256_128(srl_epi16)
    Vc_AVX_TO_SSE_256_128(srl_epi32)
    Vc_AVX_TO_SSE_256_128(srl_epi64)
    Vc_AVX_TO_SSE_256_128(sra_epi16)
    Vc_AVX_TO_SSE_256_128(sra_epi32)

    Vc_AVX_TO_SSE_2_NEW(cmpeq_epi8)
    Vc_AVX_TO_SSE_2_NEW(cmpeq_epi16)
    Vc_AVX_TO_SSE_2_NEW(cmpeq_epi32)
    Vc_AVX_TO_SSE_2_NEW(cmpeq_epi64)
    Vc_AVX_TO_SSE_2_NEW(cmpgt_epi8)
    Vc_AVX_TO_SSE_2_NEW(cmpgt_epi16)
    Vc_AVX_TO_SSE_2_NEW(cmpgt_epi32)
    Vc_AVX_TO_SSE_2_NEW(cmpgt_epi64)
    Vc_AVX_TO_SSE_2_NEW(unpackhi_epi16)
    Vc_AVX_TO_SSE_2_NEW(unpacklo_epi16)
    Vc_AVX_TO_SSE_2_NEW(add_epi16)
    Vc_AVX_TO_SSE_2_NEW(add_epi32)
    Vc_AVX_TO_SSE_2_NEW(add_epi64)
    Vc_AVX_TO_SSE_2_NEW(sub_epi16)
    Vc_AVX_TO_SSE_2_NEW(sub_epi32)
    Vc_AVX_TO_SSE_2_NEW(mullo_epi16)
    Vc_AVX_TO_SSE_2_NEW(sign_epi16)
    Vc_AVX_TO_SSE_2_NEW(sign_epi32)
    Vc_AVX_TO_SSE_2_NEW(min_epi8)
    Vc_AVX_TO_SSE_2_NEW(max_epi8)
    Vc_AVX_TO_SSE_2_NEW(min_epu16)
    Vc_AVX_TO_SSE_2_NEW(max_epu16)
    Vc_AVX_TO_SSE_2_NEW(min_epi32)
    Vc_AVX_TO_SSE_2_NEW(max_epi32)
    Vc_AVX_TO_SSE_2_NEW(min_epu32)
    Vc_AVX_TO_SSE_2_NEW(max_epu32)
    Vc_AVX_TO_SSE_2_NEW(mullo_epi32)

    Vc_AVX_TO_SSE_1(abs_epi8)
    Vc_AVX_TO_SSE_1(abs_epi16)
    Vc_AVX_TO_SSE_1(abs_epi32)
    Vc_AVX_TO_SSE_1_128(cvtepi8_epi16, 8)
    Vc_AVX_TO_SSE_1_128(cvtepi8_epi32, 4)
    Vc_AVX_TO_SSE_1_128(cvtepi8_epi64, 2)
    Vc_AVX_TO_SSE_1_128(cvtepi16_epi32, 8)
    Vc_AVX_TO_SSE_1_128(cvtepi16_epi64, 4)
    Vc_AVX_TO_SSE_1_128(cvtepi32_epi64, 8)
    Vc_AVX_TO_SSE_1_128(cvtepu8_epi16, 8)
    Vc_AVX_TO_SSE_1_128(cvtepu8_epi32, 4)
    Vc_AVX_TO_SSE_1_128(cvtepu8_epi64, 2)
    Vc_AVX_TO_SSE_1_128(cvtepu16_epi32, 8)
    Vc_AVX_TO_SSE_1_128(cvtepu16_epi64, 4)
    Vc_AVX_TO_SSE_1_128(cvtepu32_epi64, 8)
#ifndef Vc_IMPL_AVX2

/////////////////////////////////////////////////////////////////////////
// implementation of the intrinsics missing in AVX
/////////////////////////////////////////////////////////////////////////

    static Vc_INTRINSIC m256i Vc_CONST and_si256(__m256i x, __m256i y) {
        return _mm256_castps_si256(_mm256_and_ps(_mm256_castsi256_ps(x), _mm256_castsi256_ps(y)));
    }
    static Vc_INTRINSIC m256i Vc_CONST andnot_si256(__m256i x, __m256i y) {
        return _mm256_castps_si256(_mm256_andnot_ps(_mm256_castsi256_ps(x), _mm256_castsi256_ps(y)));
    }
    static Vc_INTRINSIC m256i Vc_CONST or_si256(__m256i x, __m256i y) {
        return _mm256_castps_si256(_mm256_or_ps(_mm256_castsi256_ps(x), _mm256_castsi256_ps(y)));
    }
    static Vc_INTRINSIC m256i Vc_CONST xor_si256(__m256i x, __m256i y) {
        return _mm256_castps_si256(_mm256_xor_ps(_mm256_castsi256_ps(x), _mm256_castsi256_ps(y)));
    }

    Vc_INTRINSIC Vc_CONST int movemask_epi8(__m256i a0)
    {
        m128i a1 = extract128<1>(a0);
        return (_mm_movemask_epi8(a1) << 16) | _mm_movemask_epi8(_mm256_castsi256_si128(a0));
    }
    template <int m> Vc_INTRINSIC Vc_CONST m256i blend_epi16(__m256i a0, __m256i b0)
    {
        m128i a1 = extract128<1>(a0);
        m128i b1 = extract128<1>(b0);
        m128i r0 = _mm_blend_epi16(_mm256_castsi256_si128(a0), _mm256_castsi256_si128(b0), m & 0xff);
        m128i r1 = _mm_blend_epi16(a1, b1, m >> 8);
        return insert128<1>(_mm256_castsi128_si256(r0), r1);
    }
    Vc_INTRINSIC Vc_CONST m256i blendv_epi8(__m256i a0, __m256i b0, __m256i m0) {
        m128i a1 = extract128<1>(a0);
        m128i b1 = extract128<1>(b0);
        m128i m1 = extract128<1>(m0);
        m128i r0 = _mm_blendv_epi8(_mm256_castsi256_si128(a0), _mm256_castsi256_si128(b0), _mm256_castsi256_si128(m0));
        m128i r1 = _mm_blendv_epi8(a1, b1, m1);
        return insert128<1>(_mm256_castsi128_si256(r0), r1);
    }
    // mpsadbw_epu8 (__m128i __X, __m128i __Y, const int __M)

#else // Vc_IMPL_AVX2

static Vc_INTRINSIC Vc_CONST m256i xor_si256(__m256i x, __m256i y) { return _mm256_xor_si256(x, y); }
static Vc_INTRINSIC Vc_CONST m256i or_si256(__m256i x, __m256i y) { return _mm256_or_si256(x, y); }
static Vc_INTRINSIC Vc_CONST m256i and_si256(__m256i x, __m256i y) { return _mm256_and_si256(x, y); }
static Vc_INTRINSIC Vc_CONST m256i andnot_si256(__m256i x, __m256i y) { return _mm256_andnot_si256(x, y); }

/////////////////////////////////////////////////////////////////////////
// implementation of the intrinsics missing in AVX2
/////////////////////////////////////////////////////////////////////////
Vc_INTRINSIC Vc_CONST m256i blendv_epi8(__m256i a0, __m256i b0, __m256i m0)
{
    return _mm256_blendv_epi8(a0, b0, m0);
}
Vc_INTRINSIC Vc_CONST int movemask_epi8(__m256i a0)
{
    return _mm256_movemask_epi8(a0);
}

#endif // Vc_IMPL_AVX2

/////////////////////////////////////////////////////////////////////////
// implementation of intrinsics missing in AVX and AVX2
/////////////////////////////////////////////////////////////////////////

static Vc_INTRINSIC m256i cmplt_epi64(__m256i a, __m256i b) {
    return cmpgt_epi64(b, a);
}
static Vc_INTRINSIC m256i cmplt_epi32(__m256i a, __m256i b) {
    return cmpgt_epi32(b, a);
}
static Vc_INTRINSIC m256i cmplt_epi16(__m256i a, __m256i b) {
    return cmpgt_epi16(b, a);
}
static Vc_INTRINSIC m256i cmplt_epi8(__m256i a, __m256i b) {
    return cmpgt_epi8(b, a);
}

static Vc_INTRINSIC m256i cmpgt_epu8(__m256i a, __m256i b) {
    return cmpgt_epi8(xor_si256(a, setmin_epi8()), xor_si256(b, setmin_epi8()));
}
#if defined(Vc_IMPL_XOP)
    Vc_AVX_TO_SSE_2_NEW(comlt_epu32)
    Vc_AVX_TO_SSE_2_NEW(comgt_epu32)
    Vc_AVX_TO_SSE_2_NEW(comlt_epu16)
    Vc_AVX_TO_SSE_2_NEW(comgt_epu16)
    static Vc_INTRINSIC m256i Vc_CONST cmplt_epu32(__m256i a, __m256i b) { return comlt_epu32(a, b); }
    static Vc_INTRINSIC m256i Vc_CONST cmpgt_epu32(__m256i a, __m256i b) { return comgt_epu32(a, b); }
    static Vc_INTRINSIC m256i Vc_CONST cmplt_epu16(__m256i a, __m256i b) { return comlt_epu16(a, b); }
    static Vc_INTRINSIC m256i Vc_CONST cmpgt_epu16(__m256i a, __m256i b) { return comgt_epu16(a, b); }
#else
    static Vc_INTRINSIC m256i Vc_CONST cmplt_epu32(__m256i _a, __m256i _b) {
        m256i a = _mm256_castps_si256(_mm256_xor_ps(_mm256_castsi256_ps(_a), _mm256_castsi256_ps(setmin_epi32())));
        m256i b = _mm256_castps_si256(_mm256_xor_ps(_mm256_castsi256_ps(_b), _mm256_castsi256_ps(setmin_epi32())));
        return cmplt_epi32(a, b);
    }
    static Vc_INTRINSIC m256i Vc_CONST cmpgt_epu32(__m256i _a, __m256i _b) {
        m256i a = _mm256_castps_si256(_mm256_xor_ps(_mm256_castsi256_ps(_a), _mm256_castsi256_ps(setmin_epi32())));
        m256i b = _mm256_castps_si256(_mm256_xor_ps(_mm256_castsi256_ps(_b), _mm256_castsi256_ps(setmin_epi32())));
        return cmpgt_epi32(a, b);
    }
    static Vc_INTRINSIC m256i Vc_CONST cmplt_epu16(__m256i _a, __m256i _b) {
        m256i a = _mm256_castps_si256(_mm256_xor_ps(_mm256_castsi256_ps(_a), _mm256_castsi256_ps(setmin_epi16())));
        m256i b = _mm256_castps_si256(_mm256_xor_ps(_mm256_castsi256_ps(_b), _mm256_castsi256_ps(setmin_epi16())));
        return cmplt_epi16(a, b);
    }
    static Vc_INTRINSIC m256i Vc_CONST cmpgt_epu16(__m256i _a, __m256i _b) {
        m256i a = _mm256_castps_si256(_mm256_xor_ps(_mm256_castsi256_ps(_a), _mm256_castsi256_ps(setmin_epi16())));
        m256i b = _mm256_castps_si256(_mm256_xor_ps(_mm256_castsi256_ps(_b), _mm256_castsi256_ps(setmin_epi16())));
        return cmpgt_epi16(a, b);
    }
#endif

static Vc_INTRINSIC void _mm256_maskstore(float *mem, const __m256 mask, const __m256 v) {
    _mm256_maskstore_ps(mem, _mm256_castps_si256(mask), v);
}
static Vc_INTRINSIC void _mm256_maskstore(double *mem, const __m256d mask, const __m256d v) {
    _mm256_maskstore_pd(mem, _mm256_castpd_si256(mask), v);
}
static Vc_INTRINSIC void _mm256_maskstore(int *mem, const __m256i mask, const __m256i v) {
#ifdef Vc_IMPL_AVX2
    _mm256_maskstore_epi32(mem, mask, v);
#else
    _mm256_maskstore_ps(reinterpret_cast<float *>(mem), mask, _mm256_castsi256_ps(v));
#endif
}
static Vc_INTRINSIC void _mm256_maskstore(unsigned int *mem, const __m256i mask, const __m256i v) {
    _mm256_maskstore(reinterpret_cast<int *>(mem), mask, v);
}
static Vc_INTRINSIC void _mm256_maskstore(short *mem, const __m256i mask, const __m256i v) {
    using namespace AVX;
    _mm_maskmoveu_si128(_mm256_castsi256_si128(v), _mm256_castsi256_si128(mask), reinterpret_cast<char *>(&mem[0]));
    _mm_maskmoveu_si128(extract128<1>(v), extract128<1>(mask), reinterpret_cast<char *>(&mem[8]));
}
static Vc_INTRINSIC void _mm256_maskstore(unsigned short *mem, const __m256i mask, const __m256i v) {
    _mm256_maskstore(reinterpret_cast<short *>(mem), mask, v);
}

#undef Vc_AVX_TO_SSE_1
#undef Vc_AVX_TO_SSE_1_128
#undef Vc_AVX_TO_SSE_2_NEW
#undef Vc_AVX_TO_SSE_256_128
#undef Vc_AVX_TO_SSE_1i

template<typename R> Vc_INTRINSIC_L R stream_load(const float *mem) Vc_INTRINSIC_R;
template<> Vc_INTRINSIC m128 stream_load<m128>(const float *mem)
{
    return _mm_castsi128_ps(_mm_stream_load_si128(reinterpret_cast<__m128i *>(const_cast<float *>(mem))));
}
template<> Vc_INTRINSIC m256 stream_load<m256>(const float *mem)
{
    return insert128<1>(_mm256_castps128_ps256(stream_load<m128>(mem)),
                                stream_load<m128>(mem + 4));
}

template<typename R> Vc_INTRINSIC_L R stream_load(const double *mem) Vc_INTRINSIC_R;
template<> Vc_INTRINSIC m128d stream_load<m128d>(const double *mem)
{
    return _mm_castsi128_pd(_mm_stream_load_si128(reinterpret_cast<__m128i *>(const_cast<double *>(mem))));
}
template<> Vc_INTRINSIC m256d stream_load<m256d>(const double *mem)
{
    return insert128<1>(_mm256_castpd128_pd256(stream_load<m128d>(mem)),
                                stream_load<m128d>(mem + 2));
}

template<typename R> Vc_INTRINSIC_L R stream_load(const void *mem) Vc_INTRINSIC_R;
template<> Vc_INTRINSIC m128i stream_load<m128i>(const void *mem)
{
    return _mm_stream_load_si128(reinterpret_cast<__m128i *>(const_cast<void *>(mem)));
}
template<> Vc_INTRINSIC m256i stream_load<m256i>(const void *mem)
{
    return insert128<1>(_mm256_castsi128_si256(stream_load<m128i>(mem)),
                                stream_load<m128i>(static_cast<const __m128i *>(mem) + 1));
}

Vc_INTRINSIC void stream_store(float *mem, __m128 value, __m128 mask)
{
    _mm_maskmoveu_si128(_mm_castps_si128(value), _mm_castps_si128(mask), reinterpret_cast<char *>(mem));
}
Vc_INTRINSIC void stream_store(float *mem, __m256 value, __m256 mask)
{
    stream_store(mem, _mm256_castps256_ps128(value), _mm256_castps256_ps128(mask));
    stream_store(mem + 4, extract128<1>(value), extract128<1>(mask));
}
Vc_INTRINSIC void stream_store(double *mem, __m128d value, __m128d mask)
{
    _mm_maskmoveu_si128(_mm_castpd_si128(value), _mm_castpd_si128(mask), reinterpret_cast<char *>(mem));
}
Vc_INTRINSIC void stream_store(double *mem, __m256d value, __m256d mask)
{
    stream_store(mem, _mm256_castpd256_pd128(value), _mm256_castpd256_pd128(mask));
    stream_store(mem + 2, extract128<1>(value), extract128<1>(mask));
}
Vc_INTRINSIC void stream_store(void *mem, __m128i value, __m128i mask)
{
    _mm_maskmoveu_si128(value, mask, reinterpret_cast<char *>(mem));
}
Vc_INTRINSIC void stream_store(void *mem, __m256i value, __m256i mask)
{
    stream_store(mem, _mm256_castsi256_si128(value), _mm256_castsi256_si128(mask));
    stream_store(static_cast<__m128i *>(mem) + 1, extract128<1>(value), extract128<1>(mask));
}

#ifndef __x86_64__
Vc_INTRINSIC Vc_PURE __m128i _mm_cvtsi64_si128(int64_t x) {
    return _mm_castpd_si128(_mm_load_sd(reinterpret_cast<const double *>(&x)));
}
#endif

#ifdef Vc_IMPL_AVX2
template <int Scale> __m256 gather(const float *addr, __m256i idx)
{
    return _mm256_i32gather_ps(addr, idx, Scale);
}
template <int Scale> __m256d gather(const double *addr, __m128i idx)
{
    return _mm256_i32gather_pd(addr, idx, Scale);
}
template <int Scale> __m256i gather(const int *addr, __m256i idx)
{
    return _mm256_i32gather_epi32(addr, idx, Scale);
}
template <int Scale> __m256i gather(const unsigned *addr, __m256i idx)
{
    return _mm256_i32gather_epi32(aliasing_cast<int>(addr), idx, Scale);
}

template <int Scale> __m256 gather(__m256 src, __m256 k, const float *addr, __m256i idx)
{
    return _mm256_mask_i32gather_ps(src, addr, idx, k, Scale);
}
template <int Scale>
__m256d gather(__m256d src, __m256d k, const double *addr, __m128i idx)
{
    return _mm256_mask_i32gather_pd(src, addr, idx, k, Scale);
}
template <int Scale> __m256i gather(__m256i src, __m256i k, const int *addr, __m256i idx)
{
    return _mm256_mask_i32gather_epi32(src, addr, idx, k, Scale);
}
template <int Scale>
__m256i gather(__m256i src, __m256i k, const unsigned *addr, __m256i idx)
{
    return _mm256_mask_i32gather_epi32(src, aliasing_cast<int>(addr), idx, k, Scale);
}
#endif

}  // namespace AvxIntrinsics
}  // namespace Vc

namespace Vc_VERSIONED_NAMESPACE
{
namespace AVX
{
    using namespace AvxIntrinsics;
}  // namespace AVX
namespace AVX2
{
    using namespace AvxIntrinsics;
}  // namespace AVX2
namespace AVX
{
    template<typename T> struct VectorTypeHelper;
    template<> struct VectorTypeHelper<         char > { typedef __m256i Type; };
    template<> struct VectorTypeHelper<  signed char > { typedef __m256i Type; };
    template<> struct VectorTypeHelper<unsigned char > { typedef __m256i Type; };
    template<> struct VectorTypeHelper<         short> { typedef __m256i Type; };
    template<> struct VectorTypeHelper<unsigned short> { typedef __m256i Type; };
    template<> struct VectorTypeHelper<         int  > { typedef __m256i Type; };
    template<> struct VectorTypeHelper<unsigned int  > { typedef __m256i Type; };
    template<> struct VectorTypeHelper<         long > { typedef __m256i Type; };
    template<> struct VectorTypeHelper<unsigned long > { typedef __m256i Type; };
    template<> struct VectorTypeHelper<         long long> { typedef __m256i Type; };
    template<> struct VectorTypeHelper<unsigned long long> { typedef __m256i Type; };
    template<> struct VectorTypeHelper<         float> { typedef __m256  Type; };
    template<> struct VectorTypeHelper<        double> { typedef __m256d Type; };

    template <typename T>
    using IntegerVectorType =
        typename std::conditional<sizeof(T) == 16, __m128i, __m256i>::type;
    template <typename T>
    using DoubleVectorType =
        typename std::conditional<sizeof(T) == 16, __m128d, __m256d>::type;
    template <typename T>
    using FloatVectorType =
        typename std::conditional<sizeof(T) == 16, __m128, __m256>::type;

    template<typename T> struct VectorHelper {};
    template<typename T> struct VectorHelperSize;
}  // namespace AVX
}  // namespace Vc

#endif // VC_AVX_INTRINSICS_H_
