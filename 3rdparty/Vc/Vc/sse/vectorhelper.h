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

#ifndef VC_SSE_VECTORHELPER_H_
#define VC_SSE_VECTORHELPER_H_

#include "types.h"
#include "../common/loadstoreflags.h"
#include <limits>
#include "const_data.h"
#include "macros.h"

namespace Vc_VERSIONED_NAMESPACE
{
namespace SSE
{
#define Vc_OP0(name, code) static Vc_ALWAYS_INLINE Vc_CONST VectorType name() { return code; }
#define Vc_OP1(name, code) static Vc_ALWAYS_INLINE Vc_CONST VectorType name(const VectorType a) { return code; }
#define Vc_OP2(name, code) static Vc_ALWAYS_INLINE Vc_CONST VectorType name(const VectorType a, const VectorType b) { return code; }
#define Vc_OP3(name, code) static Vc_ALWAYS_INLINE Vc_CONST VectorType name(const VectorType a, const VectorType b, const VectorType c) { return code; }

        template<> struct VectorHelper<__m128>
        {
            typedef __m128 VectorType;

            template<typename Flags> static Vc_ALWAYS_INLINE Vc_PURE VectorType load(const float *x, typename Flags::EnableIfAligned  = nullptr) { return _mm_load_ps(x); }
            template<typename Flags> static Vc_ALWAYS_INLINE Vc_PURE VectorType load(const float *x, typename Flags::EnableIfUnaligned = nullptr) { return _mm_loadu_ps(x); }
            template<typename Flags> static Vc_ALWAYS_INLINE Vc_PURE VectorType load(const float *x, typename Flags::EnableIfStreaming = nullptr) { return _mm_stream_load(x); }

            template<typename Flags> static Vc_ALWAYS_INLINE void store(float *mem, VectorType x, typename Flags::EnableIfAligned               = nullptr) { _mm_store_ps(mem, x); }
            template<typename Flags> static Vc_ALWAYS_INLINE void store(float *mem, VectorType x, typename Flags::EnableIfUnalignedNotStreaming = nullptr) { _mm_storeu_ps(mem, x); }
            template<typename Flags> static Vc_ALWAYS_INLINE void store(float *mem, VectorType x, typename Flags::EnableIfStreaming             = nullptr) { _mm_stream_ps(mem, x); }
            template<typename Flags> static Vc_ALWAYS_INLINE void store(float *mem, VectorType x, typename Flags::EnableIfUnalignedAndStreaming = nullptr) { _mm_maskmoveu_si128(_mm_castps_si128(x), _mm_setallone_si128(), reinterpret_cast<char *>(mem)); }

            // before AVX there was only one maskstore. load -> blend -> store would break the C++ memory model (read/write of memory that is actually not touched by this thread)
            template<typename Flags> static Vc_ALWAYS_INLINE void store(float *mem, VectorType x, VectorType m) { _mm_maskmoveu_si128(_mm_castps_si128(x), _mm_castps_si128(m), reinterpret_cast<char *>(mem)); }

            Vc_OP0(allone, _mm_setallone_ps())
            Vc_OP0(zero, _mm_setzero_ps())
            Vc_OP3(blend, blendv_ps(a, b, c))
        };


        template<> struct VectorHelper<__m128d>
        {
            typedef __m128d VectorType;

            template<typename Flags> static Vc_ALWAYS_INLINE Vc_PURE VectorType load(const double *x, typename Flags::EnableIfAligned   = nullptr) { return _mm_load_pd(x); }
            template<typename Flags> static Vc_ALWAYS_INLINE Vc_PURE VectorType load(const double *x, typename Flags::EnableIfUnaligned = nullptr) { return _mm_loadu_pd(x); }
            template<typename Flags> static Vc_ALWAYS_INLINE Vc_PURE VectorType load(const double *x, typename Flags::EnableIfStreaming = nullptr) { return _mm_stream_load(x); }

            template<typename Flags> static Vc_ALWAYS_INLINE void store(double *mem, VectorType x, typename Flags::EnableIfAligned               = nullptr) { _mm_store_pd(mem, x); }
            template<typename Flags> static Vc_ALWAYS_INLINE void store(double *mem, VectorType x, typename Flags::EnableIfUnalignedNotStreaming = nullptr) { _mm_storeu_pd(mem, x); }
            template<typename Flags> static Vc_ALWAYS_INLINE void store(double *mem, VectorType x, typename Flags::EnableIfStreaming             = nullptr) { _mm_stream_pd(mem, x); }
            template<typename Flags> static Vc_ALWAYS_INLINE void store(double *mem, VectorType x, typename Flags::EnableIfUnalignedAndStreaming = nullptr) { _mm_maskmoveu_si128(_mm_castpd_si128(x), _mm_setallone_si128(), reinterpret_cast<char *>(mem)); }

            // before AVX there was only one maskstore. load -> blend -> store would break the C++ memory model (read/write of memory that is actually not touched by this thread)
            template<typename Flags> static Vc_ALWAYS_INLINE void store(double *mem, VectorType x, VectorType m) { _mm_maskmoveu_si128(_mm_castpd_si128(x), _mm_castpd_si128(m), reinterpret_cast<char *>(mem)); }

            Vc_OP0(allone, _mm_setallone_pd())
            Vc_OP0(zero, _mm_setzero_pd())
            Vc_OP3(blend, blendv_pd(a, b, c))
        };

        template<> struct VectorHelper<__m128i>
        {
            typedef __m128i VectorType;

            template<typename Flags, typename T> static Vc_ALWAYS_INLINE Vc_PURE VectorType load(const T *x, typename Flags::EnableIfAligned   = nullptr) { return _mm_load_si128(reinterpret_cast<const VectorType *>(x)); }
            template<typename Flags, typename T> static Vc_ALWAYS_INLINE Vc_PURE VectorType load(const T *x, typename Flags::EnableIfUnaligned = nullptr) { return _mm_loadu_si128(reinterpret_cast<const VectorType *>(x)); }
            template<typename Flags, typename T> static Vc_ALWAYS_INLINE Vc_PURE VectorType load(const T *x, typename Flags::EnableIfStreaming = nullptr) { return _mm_stream_load(x); }

            template<typename Flags, typename T> static Vc_ALWAYS_INLINE void store(T *mem, VectorType x, typename Flags::EnableIfAligned               = nullptr) { _mm_store_si128(reinterpret_cast<VectorType *>(mem), x); }
            template<typename Flags, typename T> static Vc_ALWAYS_INLINE void store(T *mem, VectorType x, typename Flags::EnableIfUnalignedNotStreaming = nullptr) { _mm_storeu_si128(reinterpret_cast<VectorType *>(mem), x); }
            template<typename Flags, typename T> static Vc_ALWAYS_INLINE void store(T *mem, VectorType x, typename Flags::EnableIfStreaming             = nullptr) { _mm_stream_si128(reinterpret_cast<VectorType *>(mem), x); }
            template<typename Flags, typename T> static Vc_ALWAYS_INLINE void store(T *mem, VectorType x, typename Flags::EnableIfUnalignedAndStreaming = nullptr) { _mm_maskmoveu_si128(x, _mm_setallone_si128(), reinterpret_cast<char *>(mem)); }

            // before AVX there was only one maskstore. load -> blend -> store would break the C++ memory model (read/write of memory that is actually not touched by this thread)
            template<typename Flags, typename T> static Vc_ALWAYS_INLINE void store(T *mem, VectorType x, VectorType m) { _mm_maskmoveu_si128(x, m, reinterpret_cast<char *>(mem)); }

            Vc_OP0(allone, _mm_setallone_si128())
            Vc_OP0(zero, _mm_setzero_si128())
            Vc_OP3(blend, blendv_epi8(a, b, c))
        };

#undef Vc_OP1
#undef Vc_OP2
#undef Vc_OP3

#define Vc_OP1(op) \
        static Vc_ALWAYS_INLINE Vc_CONST VectorType op(const VectorType a) { return Vc_CAT2(_mm_##op##_, Vc_SUFFIX)(a); }
#define Vc_OP(op) \
        static Vc_ALWAYS_INLINE Vc_CONST VectorType op(const VectorType a, const VectorType b) { return Vc_CAT2(_mm_##op##_ , Vc_SUFFIX)(a, b); }
#define Vc_OP_(op) \
        static Vc_ALWAYS_INLINE Vc_CONST VectorType op(const VectorType a, const VectorType b) { return Vc_CAT2(_mm_##op    , Vc_SUFFIX)(a, b); }
#define Vc_OPx(op, op2) \
        static Vc_ALWAYS_INLINE Vc_CONST VectorType op(const VectorType a, const VectorType b) { return Vc_CAT2(_mm_##op2##_, Vc_SUFFIX)(a, b); }
#define Vc_OP_CAST_(op) \
        static Vc_ALWAYS_INLINE Vc_CONST VectorType op(const VectorType a, const VectorType b) { return Vc_CAT2(_mm_castps_, Vc_SUFFIX)( \
            _mm_##op##ps(Vc_CAT2(Vc_CAT2(_mm_cast, Vc_SUFFIX), _ps)(a), \
              Vc_CAT2(Vc_CAT2(_mm_cast, Vc_SUFFIX), _ps)(b))); \
        }
#define Vc_MINMAX \
        static Vc_ALWAYS_INLINE Vc_CONST VectorType min(VectorType a, VectorType b) { return Vc_CAT2(_mm_min_, Vc_SUFFIX)(a, b); } \
        static Vc_ALWAYS_INLINE Vc_CONST VectorType max(VectorType a, VectorType b) { return Vc_CAT2(_mm_max_, Vc_SUFFIX)(a, b); }

        template<> struct VectorHelper<double> {
            typedef __m128d VectorType;
            typedef double EntryType;
#define Vc_SUFFIX pd

            Vc_OP_(or_) Vc_OP_(and_) Vc_OP_(xor_)
            static Vc_ALWAYS_INLINE Vc_CONST VectorType notMaskedToZero(VectorType a, __m128 mask) { return Vc_CAT2(_mm_and_, Vc_SUFFIX)(_mm_castps_pd(mask), a); }
            static Vc_ALWAYS_INLINE Vc_CONST VectorType set(const double a) { return Vc_CAT2(_mm_set1_, Vc_SUFFIX)(a); }
            static Vc_ALWAYS_INLINE Vc_CONST VectorType set(const double a, const double b) { return Vc_CAT2(_mm_set_, Vc_SUFFIX)(a, b); }
            static Vc_ALWAYS_INLINE Vc_CONST VectorType zero() { return Vc_CAT2(_mm_setzero_, Vc_SUFFIX)(); }
            static Vc_ALWAYS_INLINE Vc_CONST VectorType one()  { return Vc_CAT2(_mm_setone_, Vc_SUFFIX)(); }// set(1.); }

#ifdef Vc_IMPL_FMA4
            static Vc_ALWAYS_INLINE void fma(VectorType &v1, VectorType v2, VectorType v3) {
                v1 = _mm_macc_pd(v1, v2, v3);
            }
#else
            static inline void fma(VectorType &v1, VectorType v2, VectorType v3) {
                VectorType h1 = _mm_and_pd(v1, _mm_load_pd(reinterpret_cast<const double *>(&c_general::highMaskDouble)));
                VectorType h2 = _mm_and_pd(v2, _mm_load_pd(reinterpret_cast<const double *>(&c_general::highMaskDouble)));
#if defined(Vc_GCC) && Vc_GCC < 0x40703
                // GCC before 4.7.3 uses an incorrect optimization where it replaces the subtraction with an andnot
                // http://gcc.gnu.org/bugzilla/show_bug.cgi?id=54703
                asm("":"+x"(h1), "+x"(h2));
#endif
                const VectorType l1 = _mm_sub_pd(v1, h1);
                const VectorType l2 = _mm_sub_pd(v2, h2);
                const VectorType ll = mul(l1, l2);
                const VectorType lh = add(mul(l1, h2), mul(h1, l2));
                const VectorType hh = mul(h1, h2);
                // ll < lh < hh for all entries is certain
                const VectorType lh_lt_v3 = _mm_cmplt_pd(abs(lh), abs(v3)); // |lh| < |v3|
                const VectorType b = blendv_pd(v3, lh, lh_lt_v3);
                const VectorType c = blendv_pd(lh, v3, lh_lt_v3);
                v1 = add(add(ll, b), add(c, hh));
            }
#endif

            Vc_OP(add) Vc_OP(sub) Vc_OP(mul)

            Vc_OP1(sqrt)
            static Vc_ALWAYS_INLINE Vc_CONST VectorType rsqrt(VectorType x) {
                return _mm_div_pd(one(), sqrt(x));
            }
            static Vc_ALWAYS_INLINE Vc_CONST VectorType reciprocal(VectorType x) {
                return _mm_div_pd(one(), x);
            }
            static Vc_ALWAYS_INLINE Vc_CONST VectorType isNaN(VectorType x) {
                return _mm_cmpunord_pd(x, x);
            }
            static Vc_ALWAYS_INLINE Vc_CONST VectorType isFinite(VectorType x) {
                return _mm_cmpord_pd(x, _mm_mul_pd(zero(), x));
            }
            static Vc_ALWAYS_INLINE Vc_CONST VectorType isInfinite(VectorType x) {
                return _mm_castsi128_pd(cmpeq_epi64(_mm_castpd_si128(abs(x)), _mm_castpd_si128(_mm_load_pd(c_log<double>::d(1)))));
            }
            static Vc_ALWAYS_INLINE Vc_CONST VectorType abs(const VectorType a) {
                return Vc_CAT2(_mm_and_, Vc_SUFFIX)(a, _mm_setabsmask_pd());
            }

            Vc_MINMAX
            static Vc_ALWAYS_INLINE Vc_CONST EntryType min(VectorType a) {
                a = _mm_min_sd(a, _mm_unpackhi_pd(a, a));
                return _mm_cvtsd_f64(a);
            }
            static Vc_ALWAYS_INLINE Vc_CONST EntryType max(VectorType a) {
                a = _mm_max_sd(a, _mm_unpackhi_pd(a, a));
                return _mm_cvtsd_f64(a);
            }
            static Vc_ALWAYS_INLINE Vc_CONST EntryType mul(VectorType a) {
                a = _mm_mul_sd(a, _mm_shuffle_pd(a, a, _MM_SHUFFLE2(0, 1)));
                return _mm_cvtsd_f64(a);
            }
            static Vc_ALWAYS_INLINE Vc_CONST EntryType add(VectorType a) {
                a = _mm_add_sd(a, _mm_shuffle_pd(a, a, _MM_SHUFFLE2(0, 1)));
                return _mm_cvtsd_f64(a);
            }
#undef Vc_SUFFIX
            static Vc_ALWAYS_INLINE Vc_CONST VectorType round(VectorType a) {
#ifdef Vc_IMPL_SSE4_1
                return _mm_round_pd(a, _MM_FROUND_NINT);
#else
                //XXX: slow: _MM_SET_ROUNDING_MODE(_MM_ROUND_NEAREST);
                return _mm_cvtepi32_pd(_mm_cvtpd_epi32(a));
#endif
            }
        };

        template<> struct VectorHelper<float> {
            typedef float EntryType;
            typedef __m128 VectorType;
#define Vc_SUFFIX ps

            Vc_OP_(or_) Vc_OP_(and_) Vc_OP_(xor_)
            static Vc_ALWAYS_INLINE Vc_CONST VectorType notMaskedToZero(VectorType a, __m128 mask) { return Vc_CAT2(_mm_and_, Vc_SUFFIX)(mask, a); }
            static Vc_ALWAYS_INLINE Vc_CONST VectorType set(const float a) { return Vc_CAT2(_mm_set1_, Vc_SUFFIX)(a); }
            static Vc_ALWAYS_INLINE Vc_CONST VectorType set(const float a, const float b, const float c, const float d) { return Vc_CAT2(_mm_set_, Vc_SUFFIX)(a, b, c, d); }
            static Vc_ALWAYS_INLINE Vc_CONST VectorType zero() { return Vc_CAT2(_mm_setzero_, Vc_SUFFIX)(); }
            static Vc_ALWAYS_INLINE Vc_CONST VectorType one()  { return Vc_CAT2(_mm_setone_, Vc_SUFFIX)(); }// set(1.f); }
            static Vc_ALWAYS_INLINE Vc_CONST __m128 concat(__m128d a, __m128d b) { return _mm_movelh_ps(_mm_cvtpd_ps(a), _mm_cvtpd_ps(b)); }

#ifdef Vc_IMPL_FMA4
            static Vc_ALWAYS_INLINE void fma(VectorType &v1, VectorType v2, VectorType v3) {
                v1 = _mm_macc_ps(v1, v2, v3);
            }
#else
            static inline void fma(VectorType &v1, VectorType v2, VectorType v3) {
                __m128d v1_0 = _mm_cvtps_pd(v1);
                __m128d v1_1 = _mm_cvtps_pd(_mm_movehl_ps(v1, v1));
                __m128d v2_0 = _mm_cvtps_pd(v2);
                __m128d v2_1 = _mm_cvtps_pd(_mm_movehl_ps(v2, v2));
                __m128d v3_0 = _mm_cvtps_pd(v3);
                __m128d v3_1 = _mm_cvtps_pd(_mm_movehl_ps(v3, v3));
                v1 = _mm_movelh_ps(
                        _mm_cvtpd_ps(_mm_add_pd(_mm_mul_pd(v1_0, v2_0), v3_0)),
                        _mm_cvtpd_ps(_mm_add_pd(_mm_mul_pd(v1_1, v2_1), v3_1)));
            }
#endif

            Vc_OP(add) Vc_OP(sub) Vc_OP(mul)

            Vc_OP1(sqrt) Vc_OP1(rsqrt)
            static Vc_ALWAYS_INLINE Vc_CONST VectorType isNaN(VectorType x) {
                return _mm_cmpunord_ps(x, x);
            }
            static Vc_ALWAYS_INLINE Vc_CONST VectorType isFinite(VectorType x) {
                return _mm_cmpord_ps(x, _mm_mul_ps(zero(), x));
            }
            static Vc_ALWAYS_INLINE Vc_CONST VectorType isInfinite(VectorType x) {
                return _mm_castsi128_ps(_mm_cmpeq_epi32(_mm_castps_si128(abs(x)), _mm_castps_si128(_mm_load_ps(c_log<float>::d(1)))));
            }
            static Vc_ALWAYS_INLINE Vc_CONST VectorType reciprocal(VectorType x) {
                return _mm_rcp_ps(x);
            }
            static Vc_ALWAYS_INLINE Vc_CONST VectorType abs(const VectorType a) {
                return Vc_CAT2(_mm_and_, Vc_SUFFIX)(a, _mm_setabsmask_ps());
            }

            Vc_MINMAX
            static Vc_ALWAYS_INLINE Vc_CONST EntryType min(VectorType a) {
                a = _mm_min_ps(a, _mm_movehl_ps(a, a));   // a = min(a0, a2), min(a1, a3), min(a2, a2), min(a3, a3)
                a = _mm_min_ss(a, _mm_shuffle_ps(a, a, _MM_SHUFFLE(1, 1, 1, 1))); // a = min(a0, a1), a1, a2, a3
                return _mm_cvtss_f32(a);
            }
            static Vc_ALWAYS_INLINE Vc_CONST EntryType max(VectorType a) {
                a = _mm_max_ps(a, _mm_movehl_ps(a, a));   // a = max(a0, a2), max(a1, a3), max(a2, a2), max(a3, a3)
                a = _mm_max_ss(a, _mm_shuffle_ps(a, a, _MM_SHUFFLE(1, 1, 1, 1))); // a = max(a0, a1), a1, a2, a3
                return _mm_cvtss_f32(a);
            }
            static Vc_ALWAYS_INLINE Vc_CONST EntryType mul(VectorType a) {
                a = _mm_mul_ps(a, _mm_shuffle_ps(a, a, _MM_SHUFFLE(0, 1, 2, 3)));
                a = _mm_mul_ss(a, _mm_shuffle_ps(a, a, _MM_SHUFFLE(3, 2, 0, 1)));
                return _mm_cvtss_f32(a);
            }
            static Vc_ALWAYS_INLINE Vc_CONST EntryType add(VectorType a) {
                a = _mm_add_ps(a, _mm_shuffle_ps(a, a, _MM_SHUFFLE(0, 1, 2, 3)));
                a = _mm_add_ss(a, _mm_shuffle_ps(a, a, _MM_SHUFFLE(3, 2, 0, 1)));
                return _mm_cvtss_f32(a);
            }
#undef Vc_SUFFIX
            static Vc_ALWAYS_INLINE Vc_CONST VectorType round(VectorType a) {
#ifdef Vc_IMPL_SSE4_1
                return _mm_round_ps(a, _MM_FROUND_NINT);
#else
                //XXX slow: _MM_SET_ROUNDING_MODE(_MM_ROUND_NEAREST);
                return _mm_cvtepi32_ps(_mm_cvtps_epi32(a));
#endif
            }
        };

        template<> struct VectorHelper<int> {
            typedef int EntryType;
            typedef __m128i VectorType;
#define Vc_SUFFIX si128

            Vc_OP_(or_) Vc_OP_(and_) Vc_OP_(xor_)
            static Vc_ALWAYS_INLINE Vc_CONST VectorType zero() { return Vc_CAT2(_mm_setzero_, Vc_SUFFIX)(); }
            static Vc_ALWAYS_INLINE Vc_CONST VectorType notMaskedToZero(VectorType a, __m128 mask) { return Vc_CAT2(_mm_and_, Vc_SUFFIX)(_mm_castps_si128(mask), a); }
#undef Vc_SUFFIX
#define Vc_SUFFIX epi32
            static Vc_ALWAYS_INLINE Vc_CONST VectorType one() { return Vc_CAT2(_mm_setone_, Vc_SUFFIX)(); }

            static Vc_ALWAYS_INLINE Vc_CONST VectorType set(const int a) { return Vc_CAT2(_mm_set1_, Vc_SUFFIX)(a); }
            static Vc_ALWAYS_INLINE Vc_CONST VectorType set(const int a, const int b, const int c, const int d) { return Vc_CAT2(_mm_set_, Vc_SUFFIX)(a, b, c, d); }

            static Vc_ALWAYS_INLINE void fma(VectorType &v1, VectorType v2, VectorType v3) { v1 = add(mul(v1, v2), v3); }

            static Vc_ALWAYS_INLINE Vc_CONST VectorType shiftLeft(VectorType a, int shift) {
                return Vc_CAT2(_mm_slli_, Vc_SUFFIX)(a, shift);
            }
            static Vc_ALWAYS_INLINE Vc_CONST VectorType shiftRight(VectorType a, int shift) {
                return Vc_CAT2(_mm_srai_, Vc_SUFFIX)(a, shift);
            }
            static Vc_ALWAYS_INLINE Vc_CONST VectorType abs(const VectorType a) { return abs_epi32(a); }
            static Vc_ALWAYS_INLINE Vc_CONST VectorType min(VectorType a, VectorType b) { return min_epi32(a, b); }
            static Vc_ALWAYS_INLINE Vc_CONST VectorType max(VectorType a, VectorType b) { return max_epi32(a, b); }
            static Vc_ALWAYS_INLINE Vc_CONST EntryType min(VectorType a) {
                a = min(a, _mm_shuffle_epi32(a, _MM_SHUFFLE(1, 0, 3, 2)));
                // using lo_epi16 for speed here
                a = min(a, _mm_shufflelo_epi16(a, _MM_SHUFFLE(1, 0, 3, 2)));
                return _mm_cvtsi128_si32(a);
            }
            static Vc_ALWAYS_INLINE Vc_CONST EntryType max(VectorType a) {
                a = max(a, _mm_shuffle_epi32(a, _MM_SHUFFLE(1, 0, 3, 2)));
                // using lo_epi16 for speed here
                a = max(a, _mm_shufflelo_epi16(a, _MM_SHUFFLE(1, 0, 3, 2)));
                return _mm_cvtsi128_si32(a);
            }
            static Vc_ALWAYS_INLINE Vc_CONST EntryType add(VectorType a) {
                a = add(a, _mm_shuffle_epi32(a, _MM_SHUFFLE(1, 0, 3, 2)));
                a = add(a, _mm_shufflelo_epi16(a, _MM_SHUFFLE(1, 0, 3, 2)));
                return _mm_cvtsi128_si32(a);
            }
#ifdef Vc_IMPL_SSE4_1
            static Vc_ALWAYS_INLINE Vc_CONST VectorType mul(VectorType a, VectorType b) { return _mm_mullo_epi32(a, b); }
            static Vc_ALWAYS_INLINE Vc_CONST EntryType mul(VectorType a) {
                a = mul(a, _mm_shuffle_epi32(a, _MM_SHUFFLE(1, 0, 3, 2)));
                a = mul(a, _mm_shufflelo_epi16(a, _MM_SHUFFLE(1, 0, 3, 2)));
                return _mm_cvtsi128_si32(a);
            }
#else
            static inline Vc_CONST VectorType mul(const VectorType a, const VectorType b) {
                const VectorType aShift = _mm_srli_si128(a, 4);
                const VectorType ab02 = _mm_mul_epu32(a, b); // [a0 * b0, a2 * b2]
                const VectorType bShift = _mm_srli_si128(b, 4);
                const VectorType ab13 = _mm_mul_epu32(aShift, bShift); // [a1 * b1, a3 * b3]
                return _mm_unpacklo_epi32(_mm_shuffle_epi32(ab02, 8), _mm_shuffle_epi32(ab13, 8));
            }
#endif

            Vc_OP(add) Vc_OP(sub)
#undef Vc_SUFFIX
            static Vc_ALWAYS_INLINE Vc_CONST VectorType round(VectorType a) { return a; }
        };

        template<> struct VectorHelper<unsigned int> {
            typedef unsigned int EntryType;
            typedef __m128i VectorType;
#define Vc_SUFFIX si128
            Vc_OP_CAST_(or_) Vc_OP_CAST_(and_) Vc_OP_CAST_(xor_)
            static Vc_ALWAYS_INLINE Vc_CONST VectorType zero() { return Vc_CAT2(_mm_setzero_, Vc_SUFFIX)(); }
            static Vc_ALWAYS_INLINE Vc_CONST VectorType notMaskedToZero(VectorType a, __m128 mask) { return Vc_CAT2(_mm_and_, Vc_SUFFIX)(_mm_castps_si128(mask), a); }

#undef Vc_SUFFIX
#define Vc_SUFFIX epu32
            static Vc_ALWAYS_INLINE Vc_CONST VectorType one() { return Vc_CAT2(_mm_setone_, Vc_SUFFIX)(); }

            static Vc_ALWAYS_INLINE Vc_CONST VectorType min(VectorType a, VectorType b) { return min_epu32(a, b); }
            static Vc_ALWAYS_INLINE Vc_CONST VectorType max(VectorType a, VectorType b) { return max_epu32(a, b); }
            static Vc_ALWAYS_INLINE Vc_CONST EntryType min(VectorType a) {
                a = min(a, _mm_shuffle_epi32(a, _MM_SHUFFLE(1, 0, 3, 2)));
                // using lo_epi16 for speed here
                a = min(a, _mm_shufflelo_epi16(a, _MM_SHUFFLE(1, 0, 3, 2)));
                return _mm_cvtsi128_si32(a);
            }
            static Vc_ALWAYS_INLINE Vc_CONST EntryType max(VectorType a) {
                a = max(a, _mm_shuffle_epi32(a, _MM_SHUFFLE(1, 0, 3, 2)));
                // using lo_epi16 for speed here
                a = max(a, _mm_shufflelo_epi16(a, _MM_SHUFFLE(1, 0, 3, 2)));
                return _mm_cvtsi128_si32(a);
            }
            static Vc_ALWAYS_INLINE Vc_CONST EntryType mul(VectorType a) {
                a = mul(a, _mm_shuffle_epi32(a, _MM_SHUFFLE(1, 0, 3, 2)));
                // using lo_epi16 for speed here
                a = mul(a, _mm_shufflelo_epi16(a, _MM_SHUFFLE(1, 0, 3, 2)));
                return _mm_cvtsi128_si32(a);
            }
            static Vc_ALWAYS_INLINE Vc_CONST EntryType add(VectorType a) {
                a = add(a, _mm_shuffle_epi32(a, _MM_SHUFFLE(1, 0, 3, 2)));
                // using lo_epi16 for speed here
                a = add(a, _mm_shufflelo_epi16(a, _MM_SHUFFLE(1, 0, 3, 2)));
                return _mm_cvtsi128_si32(a);
            }

            static Vc_ALWAYS_INLINE void fma(VectorType &v1, VectorType v2, VectorType v3) { v1 = add(mul(v1, v2), v3); }

            static Vc_ALWAYS_INLINE Vc_CONST VectorType mul(const VectorType a, const VectorType b) {
                return VectorHelper<int>::mul(a, b);
            }
//X             template<unsigned int b> static Vc_ALWAYS_INLINE Vc_CONST VectorType mul(const VectorType a) {
//X                 switch (b) {
//X                     case    0: return zero();
//X                     case    1: return a;
//X                     case    2: return _mm_slli_epi32(a,  1);
//X                     case    4: return _mm_slli_epi32(a,  2);
//X                     case    8: return _mm_slli_epi32(a,  3);
//X                     case   16: return _mm_slli_epi32(a,  4);
//X                     case   32: return _mm_slli_epi32(a,  5);
//X                     case   64: return _mm_slli_epi32(a,  6);
//X                     case  128: return _mm_slli_epi32(a,  7);
//X                     case  256: return _mm_slli_epi32(a,  8);
//X                     case  512: return _mm_slli_epi32(a,  9);
//X                     case 1024: return _mm_slli_epi32(a, 10);
//X                     case 2048: return _mm_slli_epi32(a, 11);
//X                 }
//X                 return mul(a, set(b));
//X             }

#undef Vc_SUFFIX
#define Vc_SUFFIX epi32
            static Vc_ALWAYS_INLINE Vc_CONST VectorType shiftLeft(VectorType a, int shift) {
                return Vc_CAT2(_mm_slli_, Vc_SUFFIX)(a, shift);
            }
            static Vc_ALWAYS_INLINE Vc_CONST VectorType shiftRight(VectorType a, int shift) {
                return Vc_CAT2(_mm_srli_, Vc_SUFFIX)(a, shift);
            }
            static Vc_ALWAYS_INLINE Vc_CONST VectorType set(const unsigned int a) { return Vc_CAT2(_mm_set1_, Vc_SUFFIX)(a); }
            static Vc_ALWAYS_INLINE Vc_CONST VectorType set(const unsigned int a, const unsigned int b, const unsigned int c, const unsigned int d) { return Vc_CAT2(_mm_set_, Vc_SUFFIX)(a, b, c, d); }

            Vc_OP(add) Vc_OP(sub)
#undef Vc_SUFFIX
            static Vc_ALWAYS_INLINE Vc_CONST VectorType round(VectorType a) { return a; }
        };

        template<> struct VectorHelper<signed short> {
            typedef __m128i VectorType;
            typedef signed short EntryType;
#define Vc_SUFFIX si128

            Vc_OP_(or_) Vc_OP_(and_) Vc_OP_(xor_)
            static Vc_ALWAYS_INLINE Vc_CONST VectorType zero() { return Vc_CAT2(_mm_setzero_, Vc_SUFFIX)(); }
            static Vc_ALWAYS_INLINE Vc_CONST VectorType notMaskedToZero(VectorType a, __m128 mask) { return Vc_CAT2(_mm_and_, Vc_SUFFIX)(_mm_castps_si128(mask), a); }
            static Vc_ALWAYS_INLINE Vc_CONST __m128i concat(__m128i a, __m128i b) { return _mm_packs_epi32(a, b); }
            static Vc_ALWAYS_INLINE Vc_CONST __m128i expand0(__m128i x) { return _mm_srai_epi32(_mm_unpacklo_epi16(x, x), 16); }
            static Vc_ALWAYS_INLINE Vc_CONST __m128i expand1(__m128i x) { return _mm_srai_epi32(_mm_unpackhi_epi16(x, x), 16); }

#undef Vc_SUFFIX
#define Vc_SUFFIX epi16
            static Vc_ALWAYS_INLINE Vc_CONST VectorType one() { return Vc_CAT2(_mm_setone_, Vc_SUFFIX)(); }

            static Vc_ALWAYS_INLINE Vc_CONST VectorType shiftLeft(VectorType a, int shift) {
                return Vc_CAT2(_mm_slli_, Vc_SUFFIX)(a, shift);
            }
            static Vc_ALWAYS_INLINE Vc_CONST VectorType shiftRight(VectorType a, int shift) {
                return Vc_CAT2(_mm_srai_, Vc_SUFFIX)(a, shift);
            }
            static Vc_ALWAYS_INLINE Vc_CONST VectorType set(const EntryType a) { return Vc_CAT2(_mm_set1_, Vc_SUFFIX)(a); }
            static Vc_ALWAYS_INLINE Vc_CONST VectorType set(const EntryType a, const EntryType b, const EntryType c, const EntryType d,
                    const EntryType e, const EntryType f, const EntryType g, const EntryType h) {
                return Vc_CAT2(_mm_set_, Vc_SUFFIX)(a, b, c, d, e, f, g, h);
            }

            static Vc_ALWAYS_INLINE void fma(VectorType &v1, VectorType v2, VectorType v3) {
                v1 = add(mul(v1, v2), v3); }

            static Vc_ALWAYS_INLINE Vc_CONST VectorType abs(const VectorType a) { return abs_epi16(a); }

            Vc_OPx(mul, mullo)
            Vc_OP(min) Vc_OP(max)
            static Vc_ALWAYS_INLINE Vc_CONST EntryType min(VectorType a) {
                // reminder: _MM_SHUFFLE(3, 2, 1, 0) means "no change"
                a = min(a, _mm_shuffle_epi32(a, _MM_SHUFFLE(1, 0, 3, 2)));
                a = min(a, _mm_shufflelo_epi16(a, _MM_SHUFFLE(1, 0, 3, 2)));
                a = min(a, _mm_shufflelo_epi16(a, _MM_SHUFFLE(1, 1, 1, 1)));
                return _mm_cvtsi128_si32(a); // & 0xffff is implicit
            }
            static Vc_ALWAYS_INLINE Vc_CONST EntryType max(VectorType a) {
                // reminder: _MM_SHUFFLE(3, 2, 1, 0) means "no change"
                a = max(a, _mm_shuffle_epi32(a, _MM_SHUFFLE(1, 0, 3, 2)));
                a = max(a, _mm_shufflelo_epi16(a, _MM_SHUFFLE(1, 0, 3, 2)));
                a = max(a, _mm_shufflelo_epi16(a, _MM_SHUFFLE(1, 1, 1, 1)));
                return _mm_cvtsi128_si32(a); // & 0xffff is implicit
            }
            static Vc_ALWAYS_INLINE Vc_CONST EntryType mul(VectorType a) {
                a = mul(a, _mm_shuffle_epi32(a, _MM_SHUFFLE(1, 0, 3, 2)));
                a = mul(a, _mm_shufflelo_epi16(a, _MM_SHUFFLE(1, 0, 3, 2)));
                a = mul(a, _mm_shufflelo_epi16(a, _MM_SHUFFLE(1, 1, 1, 1)));
                return _mm_cvtsi128_si32(a); // & 0xffff is implicit
            }
            static Vc_ALWAYS_INLINE Vc_CONST EntryType add(VectorType a) {
                a = add(a, _mm_shuffle_epi32(a, _MM_SHUFFLE(1, 0, 3, 2)));
                a = add(a, _mm_shufflelo_epi16(a, _MM_SHUFFLE(1, 0, 3, 2)));
                a = add(a, _mm_shufflelo_epi16(a, _MM_SHUFFLE(1, 1, 1, 1)));
                return _mm_cvtsi128_si32(a); // & 0xffff is implicit
            }

            Vc_OP(add) Vc_OP(sub)
#undef Vc_SUFFIX
            static Vc_ALWAYS_INLINE Vc_CONST VectorType round(VectorType a) { return a; }
        };

        template<> struct VectorHelper<unsigned short> {
            typedef __m128i VectorType;
            typedef unsigned short EntryType;
#define Vc_SUFFIX si128
            Vc_OP_CAST_(or_) Vc_OP_CAST_(and_) Vc_OP_CAST_(xor_)
            static Vc_ALWAYS_INLINE Vc_CONST VectorType zero() { return Vc_CAT2(_mm_setzero_, Vc_SUFFIX)(); }
            static Vc_ALWAYS_INLINE Vc_CONST VectorType notMaskedToZero(VectorType a, __m128 mask) { return Vc_CAT2(_mm_and_, Vc_SUFFIX)(_mm_castps_si128(mask), a); }
#ifdef Vc_IMPL_SSE4_1
            static Vc_ALWAYS_INLINE Vc_CONST __m128i concat(__m128i a, __m128i b) { return _mm_packus_epi32(a, b); }
#else
            // FIXME too bad, but this is broken without SSE 4.1
            static Vc_ALWAYS_INLINE Vc_CONST __m128i concat(__m128i a, __m128i b) {
                auto tmp0 = _mm_unpacklo_epi16(a, b); // 0 4 X X 1 5 X X
                auto tmp1 = _mm_unpackhi_epi16(a, b); // 2 6 X X 3 7 X X
                auto tmp2 = _mm_unpacklo_epi16(tmp0, tmp1); // 0 2 4 6 X X X X
                auto tmp3 = _mm_unpackhi_epi16(tmp0, tmp1); // 1 3 5 7 X X X X
                return _mm_unpacklo_epi16(tmp2, tmp3); // 0 1 2 3 4 5 6 7
            }
#endif
            static Vc_ALWAYS_INLINE Vc_CONST __m128i expand0(__m128i x) { return _mm_unpacklo_epi16(x, _mm_setzero_si128()); }
            static Vc_ALWAYS_INLINE Vc_CONST __m128i expand1(__m128i x) { return _mm_unpackhi_epi16(x, _mm_setzero_si128()); }

#undef Vc_SUFFIX
#define Vc_SUFFIX epu16
            static Vc_ALWAYS_INLINE Vc_CONST VectorType one() { return Vc_CAT2(_mm_setone_, Vc_SUFFIX)(); }

//X             template<unsigned int b> static Vc_ALWAYS_INLINE Vc_CONST VectorType mul(const VectorType a) {
//X                 switch (b) {
//X                     case    0: return zero();
//X                     case    1: return a;
//X                     case    2: return _mm_slli_epi16(a,  1);
//X                     case    4: return _mm_slli_epi16(a,  2);
//X                     case    8: return _mm_slli_epi16(a,  3);
//X                     case   16: return _mm_slli_epi16(a,  4);
//X                     case   32: return _mm_slli_epi16(a,  5);
//X                     case   64: return _mm_slli_epi16(a,  6);
//X                     case  128: return _mm_slli_epi16(a,  7);
//X                     case  256: return _mm_slli_epi16(a,  8);
//X                     case  512: return _mm_slli_epi16(a,  9);
//X                     case 1024: return _mm_slli_epi16(a, 10);
//X                     case 2048: return _mm_slli_epi16(a, 11);
//X                 }
//X                 return mul(a, set(b));
//X             }
#if !defined(USE_INCORRECT_UNSIGNED_COMPARE) || Vc_IMPL_SSE4_1
            static Vc_ALWAYS_INLINE Vc_CONST VectorType min(VectorType a, VectorType b) { return min_epu16(a, b); }
            static Vc_ALWAYS_INLINE Vc_CONST VectorType max(VectorType a, VectorType b) { return max_epu16(a, b); }
#endif
#undef Vc_SUFFIX
#define Vc_SUFFIX epi16
            static Vc_ALWAYS_INLINE Vc_CONST VectorType shiftLeft(VectorType a, int shift) {
                return Vc_CAT2(_mm_slli_, Vc_SUFFIX)(a, shift);
            }
            static Vc_ALWAYS_INLINE Vc_CONST VectorType shiftRight(VectorType a, int shift) {
                return Vc_CAT2(_mm_srli_, Vc_SUFFIX)(a, shift);
            }

            static Vc_ALWAYS_INLINE void fma(VectorType &v1, VectorType v2, VectorType v3) { v1 = add(mul(v1, v2), v3); }

            Vc_OPx(mul, mullo) // should work correctly for all values
#if defined(USE_INCORRECT_UNSIGNED_COMPARE) && !defined(Vc_IMPL_SSE4_1)
            Vc_OP(min) Vc_OP(max) // XXX breaks for values with MSB set
#endif
            static Vc_ALWAYS_INLINE Vc_CONST EntryType min(VectorType a) {
                // reminder: _MM_SHUFFLE(3, 2, 1, 0) means "no change"
                a = min(a, _mm_shuffle_epi32(a, _MM_SHUFFLE(1, 0, 3, 2)));
                a = min(a, _mm_shufflelo_epi16(a, _MM_SHUFFLE(1, 0, 3, 2)));
                a = min(a, _mm_shufflelo_epi16(a, _MM_SHUFFLE(1, 1, 1, 1)));
                return _mm_cvtsi128_si32(a); // & 0xffff is implicit
            }
            static Vc_ALWAYS_INLINE Vc_CONST EntryType max(VectorType a) {
                // reminder: _MM_SHUFFLE(3, 2, 1, 0) means "no change"
                a = max(a, _mm_shuffle_epi32(a, _MM_SHUFFLE(1, 0, 3, 2)));
                a = max(a, _mm_shufflelo_epi16(a, _MM_SHUFFLE(1, 0, 3, 2)));
                a = max(a, _mm_shufflelo_epi16(a, _MM_SHUFFLE(1, 1, 1, 1)));
                return _mm_cvtsi128_si32(a); // & 0xffff is implicit
            }
            static Vc_ALWAYS_INLINE Vc_CONST EntryType mul(VectorType a) {
                // reminder: _MM_SHUFFLE(3, 2, 1, 0) means "no change"
                a = mul(a, _mm_shuffle_epi32(a, _MM_SHUFFLE(1, 0, 3, 2)));
                a = mul(a, _mm_shufflelo_epi16(a, _MM_SHUFFLE(1, 0, 3, 2)));
                a = mul(a, _mm_shufflelo_epi16(a, _MM_SHUFFLE(1, 1, 1, 1)));
                return _mm_cvtsi128_si32(a); // & 0xffff is implicit
            }
            static Vc_ALWAYS_INLINE Vc_CONST EntryType add(VectorType a) {
                // reminder: _MM_SHUFFLE(3, 2, 1, 0) means "no change"
                a = add(a, _mm_shuffle_epi32(a, _MM_SHUFFLE(1, 0, 3, 2)));
                a = add(a, _mm_shufflelo_epi16(a, _MM_SHUFFLE(1, 0, 3, 2)));
                a = add(a, _mm_shufflelo_epi16(a, _MM_SHUFFLE(1, 1, 1, 1)));
                return _mm_cvtsi128_si32(a); // & 0xffff is implicit
            }
            static Vc_ALWAYS_INLINE Vc_CONST VectorType set(const EntryType a) { return Vc_CAT2(_mm_set1_, Vc_SUFFIX)(a); }
            static Vc_ALWAYS_INLINE Vc_CONST VectorType set(const EntryType a, const EntryType b, const EntryType c,
                    const EntryType d, const EntryType e, const EntryType f,
                    const EntryType g, const EntryType h) {
                return Vc_CAT2(_mm_set_, Vc_SUFFIX)(a, b, c, d, e, f, g, h);
            }

            Vc_OP(add) Vc_OP(sub)
#undef Vc_SUFFIX
            static Vc_ALWAYS_INLINE Vc_CONST VectorType round(VectorType a) { return a; }
        };
#undef Vc_OP1
#undef Vc_OP
#undef Vc_OP_
#undef Vc_OPx
#undef Vc_OP_CAST_
#undef Vc_MINMAX

}  // namespace SSE
}  // namespace Vc

#include "vectorhelper.tcc"

#endif // VC_SSE_VECTORHELPER_H_
