/*  This file is part of the Vc library. {{{
Copyright © 2011-2015 Matthias Kretz <kretz@kde.org>

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

#ifndef VC_AVX_SHUFFLE_H_
#define VC_AVX_SHUFFLE_H_

#include "../sse/shuffle.h"
#include "macros.h"

namespace Vc_VERSIONED_NAMESPACE
{
namespace Detail
{
template <int... Dst> struct Permutation {};
template <uint8_t... Sel> struct Mask {};

#ifdef Vc_IMPL_AVX2
template <uint8_t Sel0, uint8_t Sel1, uint8_t Sel2, uint8_t Sel3, uint8_t Sel4,
          uint8_t Sel5, uint8_t Sel6, uint8_t Sel7, uint8_t Sel8, uint8_t Sel9,
          uint8_t Sel10, uint8_t Sel11, uint8_t Sel12, uint8_t Sel13, uint8_t Sel14,
          uint8_t Sel15>
Vc_INTRINSIC Vc_CONST __m256i
blend(__m256i a, __m256i b, Mask<Sel0, Sel1, Sel2, Sel3, Sel4, Sel5, Sel6, Sel7, Sel8,
                                 Sel9, Sel10, Sel11, Sel12, Sel13, Sel14, Sel15>)
{
    static_assert((Sel0 == 0 || Sel0 == 1) && (Sel1 == 0 || Sel1 == 1) &&
                      (Sel2 == 0 || Sel2 == 1) && (Sel3 == 0 || Sel3 == 1) &&
                      (Sel4 == 0 || Sel4 == 1) && (Sel5 == 0 || Sel5 == 1) &&
                      (Sel6 == 0 || Sel6 == 1) && (Sel7 == 0 || Sel7 == 1) &&
                      (Sel8 == 0 || Sel8 == 1) && (Sel9 == 0 || Sel9 == 1) &&
                      (Sel10 == 0 || Sel10 == 1) && (Sel11 == 0 || Sel11 == 1) &&
                      (Sel12 == 0 || Sel12 == 1) && (Sel13 == 0 || Sel13 == 1) &&
                      (Sel14 == 0 || Sel14 == 1) && (Sel15 == 0 || Sel15 == 1),
                  "Selectors must be 0 or 1 to select the value from a or b");
    constexpr uint8_t mask = static_cast<uint8_t>(
        (Sel0  << 0 ) | (Sel1  << 1 ) | (Sel2  << 2 ) | (Sel3  << 3 ) |
        (Sel4  << 4 ) | (Sel5  << 5 ) | (Sel6  << 6 ) | (Sel7  << 7 ) |
        (Sel8  << 8 ) | (Sel9  << 9 ) | (Sel10 << 10) | (Sel11 << 11) |
        (Sel12 << 12) | (Sel13 << 13) | (Sel14 << 14) | (Sel15 << 15));
    return _mm256_blend_epi16(a, b, mask);
}
#endif  // Vc_IMPL_AVX2
}  // namespace Detail
namespace Mem
{
#ifdef Vc_IMPL_AVX2
        template<VecPos Dst0, VecPos Dst1, VecPos Dst2, VecPos Dst3> static Vc_ALWAYS_INLINE __m256i Vc_CONST permuteLo(__m256i x) {
            static_assert(Dst0 >= X0 && Dst1 >= X0 && Dst2 >= X0 && Dst3 >= X0, "Incorrect_Range");
            static_assert(Dst0 <= X3 && Dst1 <= X3 && Dst2 <= X3 && Dst3 <= X3, "Incorrect_Range");
            return _mm256_shufflelo_epi16(x, Dst0 + Dst1 * 4 + Dst2 * 16 + Dst3 * 64);
        }

        template<VecPos Dst0, VecPos Dst1, VecPos Dst2, VecPos Dst3> static Vc_ALWAYS_INLINE __m256i Vc_CONST permuteHi(__m256i x) {
            static_assert(Dst0 >= X4 && Dst1 >= X4 && Dst2 >= X4 && Dst3 >= X4, "Incorrect_Range");
            static_assert(Dst0 <= X7 && Dst1 <= X7 && Dst2 <= X7 && Dst3 <= X7, "Incorrect_Range");
            return _mm256_shufflehi_epi16(x, (Dst0 - X4) + (Dst1 - X4) * 4 + (Dst2 - X4) * 16 + (Dst3 - X4) * 64);
        }
#endif  // Vc_IMPL_AVX2

        template<VecPos L, VecPos H> static Vc_ALWAYS_INLINE __m256 Vc_CONST permute128(__m256 x) {
            static_assert((L >= X0 && L <= X1) || L == Const0, "Incorrect_Range");
            static_assert((H >= X0 && H <= X1) || H == Const0, "Incorrect_Range");
            return _mm256_permute2f128_ps(
                x, x, (L == Const0 ? 0x8 : L) + (H == Const0 ? 0x80 : H * (1 << 4)));
        }
        template<VecPos L, VecPos H> static Vc_ALWAYS_INLINE __m256d Vc_CONST permute128(__m256d x) {
            static_assert((L >= X0 && L <= X1) || L == Const0, "Incorrect_Range");
            static_assert((H >= X0 && H <= X1) || H == Const0, "Incorrect_Range");
            return _mm256_permute2f128_pd(
                x, x, (L == Const0 ? 0x8 : L) + (H == Const0 ? 0x80 : H * (1 << 4)));
        }
        template<VecPos L, VecPos H> static Vc_ALWAYS_INLINE __m256i Vc_CONST permute128(__m256i x) {
            static_assert((L >= X0 && L <= X1) || L == Const0, "Incorrect_Range");
            static_assert((H >= X0 && H <= X1) || H == Const0, "Incorrect_Range");
#ifdef Vc_IMPL_AVX2
            return _mm256_permute2x128_si256(
                x, x, (L == Const0 ? 0x8 : L) + (H == Const0 ? 0x80 : H * (1 << 4)));
#else
            return _mm256_permute2f128_si256(
                x, x, (L == Const0 ? 0x8 : L) + (H == Const0 ? 0x80 : H * (1 << 4)));
#endif
        }
        template<VecPos L, VecPos H> static Vc_ALWAYS_INLINE __m256 Vc_CONST shuffle128(__m256 x, __m256 y) {
            static_assert(L >= X0 && H >= X0, "Incorrect_Range");
            static_assert(L <= Y1 && H <= Y1, "Incorrect_Range");
            return _mm256_permute2f128_ps(x, y, (L < Y0 ? L : L - Y0 + 2) + (H < Y0 ? H : H - Y0 + 2) * (1 << 4));
        }
        template<VecPos L, VecPos H> static Vc_ALWAYS_INLINE __m256i Vc_CONST shuffle128(__m256i x, __m256i y) {
            static_assert(L >= X0 && H >= X0, "Incorrect_Range");
            static_assert(L <= Y1 && H <= Y1, "Incorrect_Range");
#ifdef Vc_IMPL_AVX2
            return _mm256_permute2x128_si256(
                x, y, (L < Y0 ? L : L - Y0 + 2) + (H < Y0 ? H : H - Y0 + 2) * (1 << 4));
#else
            return _mm256_permute2f128_si256(
                x, y, (L < Y0 ? L : L - Y0 + 2) + (H < Y0 ? H : H - Y0 + 2) * (1 << 4));
#endif
        }
        template<VecPos L, VecPos H> static Vc_ALWAYS_INLINE __m256d Vc_CONST shuffle128(__m256d x, __m256d y) {
            static_assert(L >= X0 && H >= X0, "Incorrect_Range");
            static_assert(L <= Y1 && H <= Y1, "Incorrect_Range");
            return _mm256_permute2f128_pd(x, y, (L < Y0 ? L : L - Y0 + 2) + (H < Y0 ? H : H - Y0 + 2) * (1 << 4));
        }
        template<VecPos Dst0, VecPos Dst1, VecPos Dst2, VecPos Dst3> static Vc_ALWAYS_INLINE __m256d Vc_CONST permute(__m256d x) {
            static_assert(Dst0 >= X0 && Dst1 >= X0 && Dst2 >= X2 && Dst3 >= X2, "Incorrect_Range");
            static_assert(Dst0 <= X1 && Dst1 <= X1 && Dst2 <= X3 && Dst3 <= X3, "Incorrect_Range");
            return _mm256_permute_pd(x, Dst0 + Dst1 * 2 + (Dst2 - X2) * 4 + (Dst3 - X2) * 8);
        }
        template<VecPos Dst0, VecPos Dst1, VecPos Dst2, VecPos Dst3> static Vc_ALWAYS_INLINE __m256 Vc_CONST permute(__m256 x) {
            static_assert(Dst0 >= X0 && Dst1 >= X0 && Dst2 >= X0 && Dst3 >= X0, "Incorrect_Range");
            static_assert(Dst0 <= X3 && Dst1 <= X3 && Dst2 <= X3 && Dst3 <= X3, "Incorrect_Range");
            return _mm256_permute_ps(x, Dst0 + Dst1 * 4 + Dst2 * 16 + Dst3 * 64);
        }
        template<VecPos Dst0, VecPos Dst1, VecPos Dst2, VecPos Dst3> static Vc_ALWAYS_INLINE __m256i Vc_CONST permute(__m256i x) {
            return _mm256_castps_si256(permute<Dst0, Dst1, Dst2, Dst3>(_mm256_castsi256_ps(x)));
        }
#ifdef Vc_IMPL_AVX2
        template<VecPos Dst0, VecPos Dst1, VecPos Dst2, VecPos Dst3> static Vc_ALWAYS_INLINE __m256i Vc_CONST permute4x64(__m256i x) {
            static_assert(Dst0 >= X0 && Dst1 >= X0 && Dst2 >= X0 && Dst3 >= X0, "Incorrect_Range");
            static_assert(Dst0 <= X3 && Dst1 <= X3 && Dst2 <= X3 && Dst3 <= X3, "Incorrect_Range");
            return _mm256_permute4x64_epi64(x, Dst0 + Dst1 * 4 + Dst2 * 16 + Dst3 * 64);
        }
#endif  // Vc_IMPL_AVX2
        template<VecPos Dst0, VecPos Dst1, VecPos Dst2, VecPos Dst3> static Vc_ALWAYS_INLINE __m256d Vc_CONST shuffle(__m256d x, __m256d y) {
            static_assert(Dst0 >= X0 && Dst1 >= Y0 && Dst2 >= X2 && Dst3 >= Y2, "Incorrect_Range");
            static_assert(Dst0 <= X1 && Dst1 <= Y1 && Dst2 <= X3 && Dst3 <= Y3, "Incorrect_Range");
            return _mm256_shuffle_pd(x, y, Dst0 + (Dst1 - Y0) * 2 + (Dst2 - X2) * 4 + (Dst3 - Y2) * 8);
        }
        template<VecPos Dst0, VecPos Dst1, VecPos Dst2, VecPos Dst3> static Vc_ALWAYS_INLINE __m256 Vc_CONST shuffle(__m256 x, __m256 y) {
            static_assert(Dst0 >= X0 && Dst1 >= X0 && Dst2 >= Y0 && Dst3 >= Y0, "Incorrect_Range");
            static_assert(Dst0 <= X3 && Dst1 <= X3 && Dst2 <= Y3 && Dst3 <= Y3, "Incorrect_Range");
            return _mm256_shuffle_ps(x, y, Dst0 + Dst1 * 4 + (Dst2 - Y0) * 16 + (Dst3 - Y0) * 64);
        }
        template<VecPos Dst0, VecPos Dst1, VecPos Dst2, VecPos Dst3, VecPos Dst4, VecPos Dst5, VecPos Dst6, VecPos Dst7>
        static Vc_ALWAYS_INLINE __m256 Vc_CONST blend(__m256 x, __m256 y) {
            static_assert(Dst0 == X0 || Dst0 == Y0, "Incorrect_Range");
            static_assert(Dst1 == X1 || Dst1 == Y1, "Incorrect_Range");
            static_assert(Dst2 == X2 || Dst2 == Y2, "Incorrect_Range");
            static_assert(Dst3 == X3 || Dst3 == Y3, "Incorrect_Range");
            static_assert(Dst4 == X4 || Dst4 == Y4, "Incorrect_Range");
            static_assert(Dst5 == X5 || Dst5 == Y5, "Incorrect_Range");
            static_assert(Dst6 == X6 || Dst6 == Y6, "Incorrect_Range");
            static_assert(Dst7 == X7 || Dst7 == Y7, "Incorrect_Range");
            return _mm256_blend_ps(x, y,
                    (Dst0 / Y0) *  1 + (Dst1 / Y1) *  2 +
                    (Dst2 / Y2) *  4 + (Dst3 / Y3) *  8 +
                    (Dst4 / Y4) * 16 + (Dst5 / Y5) * 32 +
                    (Dst6 / Y6) * 64 + (Dst7 / Y7) *128
                    );
        }
        template<VecPos Dst0, VecPos Dst1, VecPos Dst2, VecPos Dst3, VecPos Dst4, VecPos Dst5, VecPos Dst6, VecPos Dst7>
        static Vc_ALWAYS_INLINE __m256i Vc_CONST blend(__m256i x, __m256i y) {
            return _mm256_castps_si256(blend<Dst0, Dst1, Dst2, Dst3, Dst4, Dst5, Dst6, Dst7>(_mm256_castsi256_ps(x), _mm256_castsi256_ps(y)));
        }
        template<VecPos Dst> struct ScaleForBlend { enum { Value = Dst >= X4 ? Dst - X4 + Y0 : Dst }; };
        template<VecPos Dst0, VecPos Dst1, VecPos Dst2, VecPos Dst3, VecPos Dst4, VecPos Dst5, VecPos Dst6, VecPos Dst7>
        static Vc_ALWAYS_INLINE __m256 Vc_CONST permute(__m256 x) {
            static_assert(Dst0 >= X0 && Dst0 <= X7, "Incorrect_Range");
            static_assert(Dst1 >= X0 && Dst1 <= X7, "Incorrect_Range");
            static_assert(Dst2 >= X0 && Dst2 <= X7, "Incorrect_Range");
            static_assert(Dst3 >= X0 && Dst3 <= X7, "Incorrect_Range");
            static_assert(Dst4 >= X0 && Dst4 <= X7, "Incorrect_Range");
            static_assert(Dst5 >= X0 && Dst5 <= X7, "Incorrect_Range");
            static_assert(Dst6 >= X0 && Dst6 <= X7, "Incorrect_Range");
            static_assert(Dst7 >= X0 && Dst7 <= X7, "Incorrect_Range");
            if (Dst0 + X4 == Dst4 && Dst1 + X4 == Dst5 && Dst2 + X4 == Dst6 && Dst3 + X4 == Dst7) {
                return permute<Dst0, Dst1, Dst2, Dst3>(x);
            }
            const __m128 loIn = _mm256_castps256_ps128(x);
            const __m128 hiIn = _mm256_extractf128_ps(x, 1);
            __m128 lo, hi;

            if (Dst0 < X4 && Dst1 < X4 && Dst2 < X4 && Dst3 < X4) {
                lo = _mm_permute_ps(loIn, Dst0 + Dst1 * 4 + Dst2 * 16 + Dst3 * 64);
            } else if (Dst0 >= X4 && Dst1 >= X4 && Dst2 >= X4 && Dst3 >= X4) {
                lo = _mm_permute_ps(hiIn, Dst0 + Dst1 * 4 + Dst2 * 16 + Dst3 * 64);
            } else if (Dst0 < X4 && Dst1 < X4 && Dst2 >= X4 && Dst3 >= X4) {
                lo = shuffle<Dst0, Dst1, Dst2 - X4 + Y0, Dst3 - X4 + Y0>(loIn, hiIn);
            } else if (Dst0 >= X4 && Dst1 >= X4 && Dst2 < X4 && Dst3 < X4) {
                lo = shuffle<Dst0 - X4, Dst1 - X4, Dst2 + Y0, Dst3 + Y0>(hiIn, loIn);
            } else if (Dst0 == X0 && Dst1 == X4 && Dst2 == X1 && Dst3 == X5) {
                lo = _mm_unpacklo_ps(loIn, hiIn);
            } else if (Dst0 == X4 && Dst1 == X0 && Dst2 == X5 && Dst3 == X1) {
                lo = _mm_unpacklo_ps(hiIn, loIn);
            } else if (Dst0 == X2 && Dst1 == X6 && Dst2 == X3 && Dst3 == X7) {
                lo = _mm_unpackhi_ps(loIn, hiIn);
            } else if (Dst0 == X6 && Dst1 == X2 && Dst2 == X7 && Dst3 == X3) {
                lo = _mm_unpackhi_ps(hiIn, loIn);
            } else if (Dst0 % X4 == 0 && Dst1 % X4 == 1 && Dst2 % X4 == 2 && Dst3 % X4 == 3) {
                lo = blend<ScaleForBlend<Dst0>::Value, ScaleForBlend<Dst1>::Value,
                   ScaleForBlend<Dst2>::Value, ScaleForBlend<Dst3>::Value>(loIn, hiIn);
            }

            if (Dst4 >= X4 && Dst5 >= X4 && Dst6 >= X4 && Dst7 >= X4) {
                hi = _mm_permute_ps(hiIn, (Dst4 - X4) + (Dst5 - X4) * 4 + (Dst6 - X4) * 16 + (Dst7 - X4) * 64);
            } else if (Dst4 < X4 && Dst5 < X4 && Dst6 < X4 && Dst7 < X4) {
                hi = _mm_permute_ps(loIn, (Dst4 - X4) + (Dst5 - X4) * 4 + (Dst6 - X4) * 16 + (Dst7 - X4) * 64);
            } else if (Dst4 < X4 && Dst5 < X4 && Dst6 >= X4 && Dst7 >= X4) {
                hi = shuffle<Dst4, Dst5, Dst6 - X4 + Y0, Dst7 - X4 + Y0>(loIn, hiIn);
            } else if (Dst4 >= X4 && Dst5 >= X4 && Dst6 < X4 && Dst7 < X4) {
                hi = shuffle<Dst4 - X4, Dst5 - X4, Dst6 + Y0, Dst7 + Y0>(hiIn, loIn);
            } else if (Dst4 == X0 && Dst5 == X4 && Dst6 == X1 && Dst7 == X5) {
                hi = _mm_unpacklo_ps(loIn, hiIn);
            } else if (Dst4 == X4 && Dst5 == X0 && Dst6 == X5 && Dst7 == X1) {
                hi = _mm_unpacklo_ps(hiIn, loIn);
            } else if (Dst4 == X2 && Dst5 == X6 && Dst6 == X3 && Dst7 == X7) {
                hi = _mm_unpackhi_ps(loIn, hiIn);
            } else if (Dst4 == X6 && Dst5 == X2 && Dst6 == X7 && Dst7 == X3) {
                hi = _mm_unpackhi_ps(hiIn, loIn);
            } else if (Dst4 % X4 == 0 && Dst5 % X4 == 1 && Dst6 % X4 == 2 && Dst7 % X4 == 3) {
                hi = blend<ScaleForBlend<Dst4>::Value, ScaleForBlend<Dst5>::Value,
                   ScaleForBlend<Dst6>::Value, ScaleForBlend<Dst7>::Value>(loIn, hiIn);
            }

            return _mm256_insertf128_ps(_mm256_castps128_ps256(lo), hi, 1);
        }
}  // namespace Mem
}  // namespace Vc

    // little endian has the lo bits on the right and high bits on the left
    // with vectors this becomes greatly confusing:
    // Mem: abcd
    // Reg: dcba
    //
    // The shuffles and permutes above use memory ordering. The ones below use register ordering:
namespace Vc_VERSIONED_NAMESPACE
{
namespace Reg
{
        template<VecPos H, VecPos L> static Vc_ALWAYS_INLINE __m256 Vc_CONST permute128(__m256 x, __m256 y) {
            static_assert(L >= X0 && H >= X0, "Incorrect_Range");
            static_assert(L <= Y1 && H <= Y1, "Incorrect_Range");
            return _mm256_permute2f128_ps(x, y, (L < Y0 ? L : L - Y0 + 2) + (H < Y0 ? H : H - Y0 + 2) * (1 << 4));
        }
        template<VecPos H, VecPos L> static Vc_ALWAYS_INLINE __m256i Vc_CONST permute128(__m256i x, __m256i y) {
            static_assert(L >= X0 && H >= X0, "Incorrect_Range");
            static_assert(L <= Y1 && H <= Y1, "Incorrect_Range");
#ifdef Vc_IMPL_AVX2
            return _mm256_permute2x128_si256(
                x, y, (L < Y0 ? L : L - Y0 + 2) + (H < Y0 ? H : H - Y0 + 2) * (1 << 4));
#else
            return _mm256_permute2f128_si256(
                x, y, (L < Y0 ? L : L - Y0 + 2) + (H < Y0 ? H : H - Y0 + 2) * (1 << 4));
#endif
        }
        template<VecPos H, VecPos L> static Vc_ALWAYS_INLINE __m256d Vc_CONST permute128(__m256d x, __m256d y) {
            static_assert(L >= X0 && H >= X0, "Incorrect_Range");
            static_assert(L <= Y1 && H <= Y1, "Incorrect_Range");
            return _mm256_permute2f128_pd(x, y, (L < Y0 ? L : L - Y0 + 2) + (H < Y0 ? H : H - Y0 + 2) * (1 << 4));
        }
        template<VecPos Dst3, VecPos Dst2, VecPos Dst1, VecPos Dst0> static Vc_ALWAYS_INLINE __m256d Vc_CONST permute(__m256d x) {
            static_assert(Dst0 >= X0 && Dst1 >= X0 && Dst2 >= X2 && Dst3 >= X2, "Incorrect_Range");
            static_assert(Dst0 <= X1 && Dst1 <= X1 && Dst2 <= X3 && Dst3 <= X3, "Incorrect_Range");
            return _mm256_permute_pd(x, Dst0 + Dst1 * 2 + (Dst2 - X2) * 4 + (Dst3 - X2) * 8);
        }
        template<VecPos Dst3, VecPos Dst2, VecPos Dst1, VecPos Dst0> static Vc_ALWAYS_INLINE __m256 Vc_CONST permute(__m256 x) {
            static_assert(Dst0 >= X0 && Dst1 >= X0 && Dst2 >= X0 && Dst3 >= X0, "Incorrect_Range");
            static_assert(Dst0 <= X3 && Dst1 <= X3 && Dst2 <= X3 && Dst3 <= X3, "Incorrect_Range");
            return _mm256_permute_ps(x, Dst0 + Dst1 * 4 + Dst2 * 16 + Dst3 * 64);
        }
        template<VecPos Dst1, VecPos Dst0> static Vc_ALWAYS_INLINE __m128d Vc_CONST permute(__m128d x) {
            static_assert(Dst0 >= X0 && Dst1 >= X0, "Incorrect_Range");
            static_assert(Dst0 <= X1 && Dst1 <= X1, "Incorrect_Range");
            return _mm_permute_pd(x, Dst0 + Dst1 * 2);
        }
        template<VecPos Dst3, VecPos Dst2, VecPos Dst1, VecPos Dst0> static Vc_ALWAYS_INLINE __m128 Vc_CONST permute(__m128 x) {
            static_assert(Dst0 >= X0 && Dst1 >= X0 && Dst2 >= X0 && Dst3 >= X0, "Incorrect_Range");
            static_assert(Dst0 <= X3 && Dst1 <= X3 && Dst2 <= X3 && Dst3 <= X3, "Incorrect_Range");
            return _mm_permute_ps(x, Dst0 + Dst1 * 4 + Dst2 * 16 + Dst3 * 64);
        }
        template<VecPos Dst3, VecPos Dst2, VecPos Dst1, VecPos Dst0> static Vc_ALWAYS_INLINE __m256d Vc_CONST shuffle(__m256d x, __m256d y) {
            static_assert(Dst0 >= X0 && Dst1 >= Y0 && Dst2 >= X2 && Dst3 >= Y2, "Incorrect_Range");
            static_assert(Dst0 <= X1 && Dst1 <= Y1 && Dst2 <= X3 && Dst3 <= Y3, "Incorrect_Range");
            return _mm256_shuffle_pd(x, y, Dst0 + (Dst1 - Y0) * 2 + (Dst2 - X2) * 4 + (Dst3 - Y2) * 8);
        }
        template<VecPos Dst3, VecPos Dst2, VecPos Dst1, VecPos Dst0> static Vc_ALWAYS_INLINE __m256 Vc_CONST shuffle(__m256 x, __m256 y) {
            static_assert(Dst0 >= X0 && Dst1 >= X0 && Dst2 >= Y0 && Dst3 >= Y0, "Incorrect_Range");
            static_assert(Dst0 <= X3 && Dst1 <= X3 && Dst2 <= Y3 && Dst3 <= Y3, "Incorrect_Range");
            return _mm256_shuffle_ps(x, y, Dst0 + Dst1 * 4 + (Dst2 - Y0) * 16 + (Dst3 - Y0) * 64);
        }
}  // namespace Reg
}  // namespace Vc

#endif // VC_AVX_SHUFFLE_H_
