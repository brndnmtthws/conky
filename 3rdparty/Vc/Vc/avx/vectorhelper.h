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

#ifndef VC_AVX_VECTORHELPER_H_
#define VC_AVX_VECTORHELPER_H_

#include <limits>
#include "types.h"
#include "intrinsics.h"
#include "casts.h"
#include "../common/loadstoreflags.h"
#include "macros.h"

namespace Vc_VERSIONED_NAMESPACE
{
namespace AVX
{
        template<> struct VectorHelper<__m256>
        {
            typedef __m256 VectorType;
            typedef const VectorType VTArg;

            template<typename Flags> static Vc_ALWAYS_INLINE void store(float *mem, VTArg x, typename Flags::EnableIfAligned               = nullptr) { _mm256_store_ps(mem, x); }
            template<typename Flags> static Vc_ALWAYS_INLINE void store(float *mem, VTArg x, typename Flags::EnableIfUnalignedNotStreaming = nullptr) { _mm256_storeu_ps(mem, x); }
            template<typename Flags> static Vc_ALWAYS_INLINE void store(float *mem, VTArg x, typename Flags::EnableIfStreaming             = nullptr) { _mm256_stream_ps(mem, x); }
            template<typename Flags> static Vc_ALWAYS_INLINE void store(float *mem, VTArg x, typename Flags::EnableIfUnalignedAndStreaming = nullptr) { AvxIntrinsics::stream_store(mem, x, setallone_ps()); }

            template<typename Flags> static Vc_ALWAYS_INLINE void store(float *mem, VTArg x, VTArg m, typename std::enable_if<!Flags::IsStreaming, void *>::type = nullptr) { _mm256_maskstore(mem, m, x); }
            template<typename Flags> static Vc_ALWAYS_INLINE void store(float *mem, VTArg x, VTArg m, typename std::enable_if< Flags::IsStreaming, void *>::type = nullptr) { AvxIntrinsics::stream_store(mem, x, m); }
        };

        template<> struct VectorHelper<__m256d>
        {
            typedef __m256d VectorType;
            typedef const VectorType VTArg;

            template<typename Flags> static Vc_ALWAYS_INLINE void store(double *mem, VTArg x, typename Flags::EnableIfAligned               = nullptr) { _mm256_store_pd(mem, x); }
            template<typename Flags> static Vc_ALWAYS_INLINE void store(double *mem, VTArg x, typename Flags::EnableIfUnalignedNotStreaming = nullptr) { _mm256_storeu_pd(mem, x); }
            template<typename Flags> static Vc_ALWAYS_INLINE void store(double *mem, VTArg x, typename Flags::EnableIfStreaming             = nullptr) { _mm256_stream_pd(mem, x); }
            template<typename Flags> static Vc_ALWAYS_INLINE void store(double *mem, VTArg x, typename Flags::EnableIfUnalignedAndStreaming = nullptr) { AvxIntrinsics::stream_store(mem, x, setallone_pd()); }

            template<typename Flags> static Vc_ALWAYS_INLINE void store(double *mem, VTArg x, VTArg m, typename std::enable_if<!Flags::IsStreaming, void *>::type = nullptr) { _mm256_maskstore(mem, m, x); }
            template<typename Flags> static Vc_ALWAYS_INLINE void store(double *mem, VTArg x, VTArg m, typename std::enable_if< Flags::IsStreaming, void *>::type = nullptr) { AvxIntrinsics::stream_store(mem, x, m); }
        };

        template<> struct VectorHelper<__m256i>
        {
            typedef __m256i VectorType;
            typedef const VectorType VTArg;

            template<typename Flags, typename T> static Vc_ALWAYS_INLINE void store(T *mem, VTArg x, typename Flags::EnableIfAligned               = nullptr) { _mm256_store_si256(reinterpret_cast<__m256i *>(mem), x); }
            template<typename Flags, typename T> static Vc_ALWAYS_INLINE void store(T *mem, VTArg x, typename Flags::EnableIfUnalignedNotStreaming = nullptr) { _mm256_storeu_si256(reinterpret_cast<__m256i *>(mem), x); }
            template<typename Flags, typename T> static Vc_ALWAYS_INLINE void store(T *mem, VTArg x, typename Flags::EnableIfStreaming             = nullptr) { _mm256_stream_si256(reinterpret_cast<__m256i *>(mem), x); }
            template<typename Flags, typename T> static Vc_ALWAYS_INLINE void store(T *mem, VTArg x, typename Flags::EnableIfUnalignedAndStreaming = nullptr) { AvxIntrinsics::stream_store(mem, x, setallone_si256()); }

            template<typename Flags, typename T> static Vc_ALWAYS_INLINE void store(T *mem, VTArg x, VTArg m, typename std::enable_if<!Flags::IsStreaming, void *>::type = nullptr) { _mm256_maskstore(mem, m, x); }
            template<typename Flags, typename T> static Vc_ALWAYS_INLINE void store(T *mem, VTArg x, VTArg m, typename std::enable_if< Flags::IsStreaming, void *>::type = nullptr) { AvxIntrinsics::stream_store(mem, x, m); }
        };

#define Vc_OP1(op) \
        static Vc_INTRINSIC VectorType Vc_CONST op(VTArg a) { return Vc_CAT2(_mm256_##op##_, Vc_SUFFIX)(a); }
#define Vc_OP(op) \
        static Vc_INTRINSIC VectorType Vc_CONST op(VTArg a, VTArg b) { return Vc_CAT2(op##_ , Vc_SUFFIX)(a, b); }
#define Vc_OP_(op) \
        static Vc_INTRINSIC VectorType Vc_CONST op(VTArg a, VTArg b) { return Vc_CAT2(_mm256_##op    , Vc_SUFFIX)(a, b); }
#define Vc_OPx(op, op2) \
        static Vc_INTRINSIC VectorType Vc_CONST op(VTArg a, VTArg b) { return Vc_CAT2(_mm256_##op2##_, Vc_SUFFIX)(a, b); }

        template<> struct VectorHelper<double> {
            typedef __m256d VectorType;
            typedef const VectorType VTArg;
            typedef double EntryType;
#define Vc_SUFFIX pd

            static Vc_ALWAYS_INLINE VectorType notMaskedToZero(VTArg a, __m256 mask) { return Vc_CAT2(_mm256_and_, Vc_SUFFIX)(_mm256_castps_pd(mask), a); }
            static Vc_ALWAYS_INLINE VectorType set(const double a) { return Vc_CAT2(_mm256_set1_, Vc_SUFFIX)(a); }
            static Vc_ALWAYS_INLINE VectorType set(const double a, const double b, const double c, const double d) {
                return Vc_CAT2(_mm256_set_, Vc_SUFFIX)(a, b, c, d);
            }
            static Vc_ALWAYS_INLINE VectorType zero() { return Vc_CAT2(_mm256_setzero_, Vc_SUFFIX)(); }
            static Vc_ALWAYS_INLINE VectorType one()  { return Vc_CAT2(setone_, Vc_SUFFIX)(); }// set(1.); }

            static inline void fma(VectorType &v1, VTArg v2, VTArg v3) {
#ifdef Vc_IMPL_FMA4
                v1 = _mm256_macc_pd(v1, v2, v3);
#else
                VectorType h1 = _mm256_and_pd(v1, _mm256_broadcast_sd(reinterpret_cast<const double *>(&c_general::highMaskDouble)));
                VectorType h2 = _mm256_and_pd(v2, _mm256_broadcast_sd(reinterpret_cast<const double *>(&c_general::highMaskDouble)));
#if defined(Vc_GCC) && Vc_GCC < 0x40703
                // GCC before 4.7.3 uses an incorrect optimization where it replaces the subtraction with an andnot
                // http://gcc.gnu.org/bugzilla/show_bug.cgi?id=54703
                asm("":"+x"(h1), "+x"(h2));
#endif
                const VectorType l1 = _mm256_sub_pd(v1, h1);
                const VectorType l2 = _mm256_sub_pd(v2, h2);
                const VectorType ll = mul(l1, l2);
                const VectorType lh = add(mul(l1, h2), mul(h1, l2));
                const VectorType hh = mul(h1, h2);
                // ll < lh < hh for all entries is certain
                const VectorType lh_lt_v3 = cmplt_pd(abs(lh), abs(v3)); // |lh| < |v3|
                const VectorType b = _mm256_blendv_pd(v3, lh, lh_lt_v3);
                const VectorType c = _mm256_blendv_pd(lh, v3, lh_lt_v3);
                v1 = add(add(ll, b), add(c, hh));
#endif
            }

            static Vc_INTRINSIC VectorType Vc_CONST add(VTArg a, VTArg b) { return _mm256_add_pd(a,b); }
            static Vc_INTRINSIC VectorType Vc_CONST sub(VTArg a, VTArg b) { return _mm256_sub_pd(a,b); }
            static Vc_INTRINSIC VectorType Vc_CONST mul(VTArg a, VTArg b) { return _mm256_mul_pd(a,b); }

            Vc_OP1(sqrt)
            static Vc_ALWAYS_INLINE Vc_CONST VectorType rsqrt(VTArg x) {
                return _mm256_div_pd(one(), sqrt(x));
            }
            static Vc_ALWAYS_INLINE Vc_CONST VectorType reciprocal(VTArg x) {
                return _mm256_div_pd(one(), x);
            }
            static Vc_ALWAYS_INLINE Vc_CONST VectorType abs(VTArg a) {
                return Vc_CAT2(_mm256_and_, Vc_SUFFIX)(a, setabsmask_pd());
            }

            static Vc_INTRINSIC VectorType Vc_CONST min(VTArg a, VTArg b) { return _mm256_min_pd(a, b); }
            static Vc_INTRINSIC VectorType Vc_CONST max(VTArg a, VTArg b) { return _mm256_max_pd(a, b); }
            static Vc_ALWAYS_INLINE Vc_CONST EntryType min(VTArg a) {
                __m128d b = _mm_min_pd(avx_cast<__m128d>(a), _mm256_extractf128_pd(a, 1));
                b = _mm_min_sd(b, _mm_unpackhi_pd(b, b));
                return _mm_cvtsd_f64(b);
            }
            static Vc_ALWAYS_INLINE Vc_CONST EntryType max(VTArg a) {
                __m128d b = _mm_max_pd(avx_cast<__m128d>(a), _mm256_extractf128_pd(a, 1));
                b = _mm_max_sd(b, _mm_unpackhi_pd(b, b));
                return _mm_cvtsd_f64(b);
            }
            static Vc_ALWAYS_INLINE Vc_CONST EntryType mul(VTArg a) {
                __m128d b = _mm_mul_pd(avx_cast<__m128d>(a), _mm256_extractf128_pd(a, 1));
                b = _mm_mul_sd(b, _mm_shuffle_pd(b, b, _MM_SHUFFLE2(0, 1)));
                return _mm_cvtsd_f64(b);
            }
            static Vc_ALWAYS_INLINE Vc_CONST EntryType add(VTArg a) {
                __m128d b = _mm_add_pd(avx_cast<__m128d>(a), _mm256_extractf128_pd(a, 1));
                b = _mm_hadd_pd(b, b); // or: b = _mm_add_sd(b, _mm256_shuffle_pd(b, b, _MM_SHUFFLE2(0, 1)));
                return _mm_cvtsd_f64(b);
            }
#undef Vc_SUFFIX
            static Vc_ALWAYS_INLINE Vc_CONST VectorType round(VTArg a) {
                return _mm256_round_pd(a, _MM_FROUND_NINT);
            }
        };

        template<> struct VectorHelper<float> {
            typedef float EntryType;
            typedef __m256 VectorType;
            typedef const VectorType VTArg;
#define Vc_SUFFIX ps

            static Vc_ALWAYS_INLINE Vc_CONST VectorType notMaskedToZero(VTArg a, __m256 mask) { return Vc_CAT2(_mm256_and_, Vc_SUFFIX)(mask, a); }
            static Vc_ALWAYS_INLINE Vc_CONST VectorType set(const float a) { return Vc_CAT2(_mm256_set1_, Vc_SUFFIX)(a); }
            static Vc_ALWAYS_INLINE Vc_CONST VectorType set(const float a, const float b, const float c, const float d,
                    const float e, const float f, const float g, const float h) {
                return Vc_CAT2(_mm256_set_, Vc_SUFFIX)(a, b, c, d, e, f, g, h); }
            static Vc_ALWAYS_INLINE Vc_CONST VectorType zero() { return Vc_CAT2(_mm256_setzero_, Vc_SUFFIX)(); }
            static Vc_ALWAYS_INLINE Vc_CONST VectorType one()  { return Vc_CAT2(setone_, Vc_SUFFIX)(); }// set(1.f); }
            static Vc_ALWAYS_INLINE Vc_CONST __m256 concat(__m256d a, __m256d b) { return _mm256_insertf128_ps(avx_cast<__m256>(_mm256_cvtpd_ps(a)), _mm256_cvtpd_ps(b), 1); }

            static inline void fma(VectorType &v1, VTArg v2, VTArg v3) {
#ifdef Vc_IMPL_FMA4
                v1 = _mm256_macc_ps(v1, v2, v3);
#else
                __m256d v1_0 = _mm256_cvtps_pd(lo128(v1));
                __m256d v1_1 = _mm256_cvtps_pd(hi128(v1));
                __m256d v2_0 = _mm256_cvtps_pd(lo128(v2));
                __m256d v2_1 = _mm256_cvtps_pd(hi128(v2));
                __m256d v3_0 = _mm256_cvtps_pd(lo128(v3));
                __m256d v3_1 = _mm256_cvtps_pd(hi128(v3));
                v1 = AVX::concat(
                        _mm256_cvtpd_ps(_mm256_add_pd(_mm256_mul_pd(v1_0, v2_0), v3_0)),
                        _mm256_cvtpd_ps(_mm256_add_pd(_mm256_mul_pd(v1_1, v2_1), v3_1)));
#endif
            }

            static Vc_INTRINSIC VectorType Vc_CONST add(VTArg a, VTArg b) { return _mm256_add_ps(a, b); }
            static Vc_INTRINSIC VectorType Vc_CONST sub(VTArg a, VTArg b) { return _mm256_sub_ps(a, b); }
            static Vc_INTRINSIC VectorType Vc_CONST mul(VTArg a, VTArg b) { return _mm256_mul_ps(a, b); }

            Vc_OP1(sqrt) Vc_OP1(rsqrt)
            static Vc_ALWAYS_INLINE Vc_CONST VectorType reciprocal(VTArg x) {
                return _mm256_rcp_ps(x);
            }
            static Vc_ALWAYS_INLINE Vc_CONST VectorType abs(VTArg a) {
                return Vc_CAT2(_mm256_and_, Vc_SUFFIX)(a, setabsmask_ps());
            }

            static Vc_INTRINSIC VectorType Vc_CONST min(VTArg a, VTArg b) { return _mm256_min_ps(a, b); }
            static Vc_INTRINSIC VectorType Vc_CONST max(VTArg a, VTArg b) { return _mm256_max_ps(a, b); }
            static Vc_ALWAYS_INLINE Vc_CONST EntryType min(VTArg a) {
                __m128 b = _mm_min_ps(lo128(a), hi128(a));
                b = _mm_min_ps(b, _mm_movehl_ps(b, b));   // b = min(a0, a2), min(a1, a3), min(a2, a2), min(a3, a3)
                b = _mm_min_ss(b, _mm_shuffle_ps(b, b, _MM_SHUFFLE(1, 1, 1, 1))); // b = min(a0, a1), a1, a2, a3
                return _mm_cvtss_f32(b);
            }
            static Vc_ALWAYS_INLINE Vc_CONST EntryType max(VTArg a) {
                __m128 b = _mm_max_ps(avx_cast<__m128>(a), _mm256_extractf128_ps(a, 1));
                b = _mm_max_ps(b, _mm_movehl_ps(b, b));   // b = max(a0, a2), max(a1, a3), max(a2, a2), max(a3, a3)
                b = _mm_max_ss(b, _mm_shuffle_ps(b, b, _MM_SHUFFLE(1, 1, 1, 1))); // b = max(a0, a1), a1, a2, a3
                return _mm_cvtss_f32(b);
            }
            static Vc_ALWAYS_INLINE Vc_CONST EntryType mul(VTArg a) {
                __m128 b = _mm_mul_ps(avx_cast<__m128>(a), _mm256_extractf128_ps(a, 1));
                b = _mm_mul_ps(b, _mm_shuffle_ps(b, b, _MM_SHUFFLE(0, 1, 2, 3)));
                b = _mm_mul_ss(b, _mm_shuffle_ps(b, b, _MM_SHUFFLE(3, 2, 0, 1)));
                return _mm_cvtss_f32(b);
            }
            static Vc_ALWAYS_INLINE Vc_CONST EntryType add(VTArg a) {
                __m128 b = _mm_add_ps(avx_cast<__m128>(a), _mm256_extractf128_ps(a, 1));
                b = _mm_add_ps(b, _mm_shuffle_ps(b, b, _MM_SHUFFLE(0, 1, 2, 3)));
                b = _mm_add_ss(b, _mm_shuffle_ps(b, b, _MM_SHUFFLE(3, 2, 0, 1)));
                return _mm_cvtss_f32(b);
            }
#undef Vc_SUFFIX
            static Vc_ALWAYS_INLINE Vc_CONST VectorType round(VTArg a) {
                return _mm256_round_ps(a, _MM_FROUND_NINT);
            }
        };

#undef Vc_OP1
#undef Vc_OP
#undef Vc_OP_
#undef Vc_OPx

}  // namespace AVX(2)
}  // namespace Vc

#endif // VC_AVX_VECTORHELPER_H_
