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

#include <Vc/avx/vector.h>
#include <Vc/avx/debug.h>
#include <Vc/avx/macros.h>

namespace Vc_VERSIONED_NAMESPACE
{
namespace Detail
{
#ifdef Vc_IMPL_AVX2
template <>
Vc_CONST AVX2::short_v sorted<CurrentImplementation::current()>(AVX2::short_v x_)
{
    // ab cd ef gh ij kl mn op
    // ↓↑ ↓↑ ↓↑ ↓↑ ↓↑ ↓↑ ↓↑ ↓↑
    // ⎮⎝ ⎠⎮ ⎮⎝ ⎠⎮ ⎮⎝ ⎠⎮ ⎮⎝ ⎠⎮
    // ⎮ ╳ ⎮ ⎮ ╳ ⎮ ⎮ ╳ ⎮ ⎮ ╳ ⎮
    // ⎮⎛ ⎞⎮ ⎮⎛ ⎞⎮ ⎮⎛ ⎞⎮ ⎮⎛ ⎞⎮
    // <> <> <> <> <> <> <> <>
    // ↓↑ ↓↑ ↓↑ ↓↑ ↓↑ ↓↑ ↓↑ ↓↑
    // ⎮⎝ ⎠⎮ ⎮⎝ ⎠⎮ ⎮⎝ ⎠⎮ ⎮⎝ ⎠⎮
    // ⎮ o ⎮ ⎮ o ⎮ ⎮ o ⎮ ⎮ o ⎮
    // ⎮↓ ↑⎮ ⎮↓ ↑⎮ ⎮↓ ↑⎮ ⎮↓ ↑⎮
    // 01 23 01 23 01 23 01 23
    // ⎮⎮ ⎮⎮   ╳   ⎮⎮ ⎮⎮   ╳
    // 01 23 32 10 01 23 32 10
    // ⎮⎝ ⎮⎝ ⎠⎮ ⎠⎮ ⎮⎝ ⎮⎝ ⎠⎮ ⎠⎮
    // ⎮ ╲⎮ ╳ ⎮╱ ⎮ ⎮ ╲⎮ ╳ ⎮╱ ⎮
    // ⎮  ╲╱ ╲╱  ⎮ ⎮  ╲╱ ╲╱  ⎮
    // ⎮  ╱╲ ╱╲  ⎮ ⎮  ╱╲ ╱╲  ⎮
    // ⎮ ╱⎮ ╳ ⎮╲ ⎮ ⎮ ╱⎮ ╳ ⎮╲ ⎮
    // ⎮⎛ ⎮⎛ ⎞⎮ ⎞⎮ ⎮⎛ ⎮⎛ ⎞⎮ ⎞⎮
    // <> <> <> <> <> <> <> <>
    // ↓↑ ↓↑ ↓↑ ↓↑ ↓↑ ↓↑ ↓↑ ↓↑
    // ⎮⎝ ⎠⎮ ⎮⎝ ⎠⎮ ⎮⎝ ⎠⎮ ⎮⎝ ⎠⎮
    // ⎮ ╳ ⎮ ⎮ ╳ ⎮ ⎮ ╳ ⎮ ⎮ ╳ ⎮
    // ⎮⎛ ⎞⎮ ⎮⎛ ⎞⎮ ⎮⎛ ⎞⎮ ⎮⎛ ⎞⎮
    // <> <> <> <> <> <> <> <>
    // ↓↑ ↓↑ ↓↑ ↓↑ ↓↑ ↓↑ ↓↑ ↓↑
    // ⎮⎝ ⎠⎮ ⎮⎝ ⎠⎮ ⎮⎝ ⎠⎮ ⎮⎝ ⎠⎮
    // ⎮ o ⎮ ⎮ o ⎮ ⎮ o ⎮ ⎮ o ⎮
    // ⎮↓ ↑⎮ ⎮↓ ↑⎮ ⎮↓ ↑⎮ ⎮↓ ↑⎮
    // 01 23 01 23 01 23 01 23

    // sort pairs (one min/max)
    auto x = AVX::lo128(x_.data());
    auto y = AVX::hi128(x_.data());
    // Vc_DEBUG << "xy: " << AVX::addType<short>(x) << AVX::addType<short>(y);
    auto l = _mm_min_epi16(x, y);
    auto h = _mm_max_epi16(x, y);
    // Vc_DEBUG << "lh: " << AVX::addType<short>(l) << AVX::addType<short>(h);

    // merge left & right quads (two min/max)
    x = _mm_unpacklo_epi16(l, h);
    y = _mm_unpackhi_epi16(h, l);
    // Vc_DEBUG << "8x2 sorted xy: " << AVX::addType<short>(x) << AVX::addType<short>(y);
    l = _mm_min_epi16(x, y);
    h = _mm_max_epi16(x, y);
    // Vc_DEBUG << "lh: " << AVX::addType<short>(l) << AVX::addType<short>(h);
    x = Mem::permuteLo<X1, X0, X3, X2>(Mem::blend<X0, Y1, X2, Y3, X4, Y5, X6, Y7>(l, h));
    y = Mem::permuteHi<X5, X4, X7, X6>(Mem::blend<X0, Y1, X2, Y3, X4, Y5, X6, Y7>(h, l));
    // Vc_DEBUG << "xy: " << AVX::addType<short>(x) << AVX::addType<short>(y);
    l = _mm_min_epi16(x, y);
    h = _mm_max_epi16(x, y);
    // Vc_DEBUG << "lh: " << AVX::addType<short>(l) << AVX::addType<short>(h);

    // merge quads into octs (three min/max)
    x = _mm_unpacklo_epi16(h, l);
    y = _mm_unpackhi_epi16(l, h);
    // Vc_DEBUG << "4x4 sorted xy: " << AVX::addType<short>(x) << AVX::addType<short>(y);
    l = _mm_min_epi16(x, y);
    h = _mm_max_epi16(x, y);
    // Vc_DEBUG << "lh: " << AVX::addType<short>(l) << AVX::addType<short>(h);
    x = Mem::permuteLo<X2, X3, X0, X1>(Mem::blend<X0, X1, Y2, Y3, X4, X5, Y6, Y7>(h, l));
    y = Mem::permuteHi<X6, X7, X4, X5>(Mem::blend<X0, X1, Y2, Y3, X4, X5, Y6, Y7>(l, h));
    // Vc_DEBUG << "xy: " << AVX::addType<short>(x) << AVX::addType<short>(y);
    l = _mm_min_epi16(x, y);
    h = _mm_max_epi16(x, y);
    // Vc_DEBUG << "lh: " << AVX::addType<short>(l) << AVX::addType<short>(h);
    x = Mem::permuteHi<X5, X4, X7, X6>(Mem::blend<X0, Y1, X2, Y3, X4, Y5, X6, Y7>(l, h));
    y = Mem::permuteLo<X1, X0, X3, X2>(Mem::blend<X0, Y1, X2, Y3, X4, Y5, X6, Y7>(h, l));
    // Vc_DEBUG << "xy: " << AVX::addType<short>(x) << AVX::addType<short>(y);
    l = _mm_min_epi16(x, y);
    h = _mm_max_epi16(x, y);
    // Vc_DEBUG << "lh: " << AVX::addType<short>(l) << AVX::addType<short>(h) << " done?";

    // merge octs into hexa (four min/max)
    x = _mm_unpacklo_epi16(l, h);
    y = _mm_unpackhi_epi16(h, l);
    // Vc_DEBUG << "2x8 sorted xy: " << AVX::addType<short>(x) << AVX::addType<short>(y);
    l = _mm_min_epi16(x, y);
    h = _mm_max_epi16(x, y);
    // Vc_DEBUG << "lh: " << AVX::addType<short>(l) << AVX::addType<short>(h);
    x = _mm_unpacklo_epi64(l, h);
    y = _mm_unpackhi_epi64(l, h);
    // Vc_DEBUG << "xy: " << AVX::addType<short>(x) << AVX::addType<short>(y);
    l = _mm_min_epi16(x, y);
    h = _mm_max_epi16(x, y);
    // Vc_DEBUG << "lh: " << AVX::addType<short>(l) << AVX::addType<short>(h);
    x = _mm_castps_si128(Mem::permute<X1, X0, X3, X2>(Mem::blend<X0, Y1, X2, Y3>(_mm_castsi128_ps(h), _mm_castsi128_ps(l))));
    y = _mm_castps_si128(Mem::blend<X0, Y1, X2, Y3>(_mm_castsi128_ps(l), _mm_castsi128_ps(h)));
    // Vc_DEBUG << "xy: " << AVX::addType<short>(x) << AVX::addType<short>(y);
    l = _mm_min_epi16(x, y);
    h = _mm_max_epi16(x, y);
    // Vc_DEBUG << "lh: " << AVX::addType<short>(l) << AVX::addType<short>(h);
    x = Mem::blend<X0, Y1, X2, Y3, X4, Y5, X6, Y7>(l, h);
    y = Mem::permuteLo<X1, X0, X3, X2>(
        Mem::permuteHi<X5, X4, X7, X6>(Mem::blend<X0, Y1, X2, Y3, X4, Y5, X6, Y7>(h, l)));
    // Vc_DEBUG << "xy: " << AVX::addType<short>(x) << AVX::addType<short>(y);
    l = _mm_min_epi16(x, y);
    h = _mm_max_epi16(x, y);
    // Vc_DEBUG << "lh: " << AVX::addType<short>(l) << AVX::addType<short>(h);
    x = _mm_unpacklo_epi16(l, h);
    y = _mm_unpackhi_epi16(l, h);
    return AVX::concat(x, y);
}

template <>
Vc_CONST AVX2::ushort_v sorted<CurrentImplementation::current()>(AVX2::ushort_v x_)
{
    // sort pairs (one min/max)
    auto x = AVX::lo128(x_.data());
    auto y = AVX::hi128(x_.data());
    // Vc_DEBUG << "xy: " << AVX::addType<short>(x) << AVX::addType<short>(y);
    auto l = _mm_min_epu16(x, y);
    auto h = _mm_max_epu16(x, y);
    // Vc_DEBUG << "lh: " << AVX::addType<short>(l) << AVX::addType<short>(h);

    // merge left & right quads (two min/max)
    x = _mm_unpacklo_epi16(l, h);
    y = _mm_unpackhi_epi16(h, l);
    // Vc_DEBUG << "8x2 sorted xy: " << AVX::addType<short>(x) << AVX::addType<short>(y);
    l = _mm_min_epu16(x, y);
    h = _mm_max_epu16(x, y);
    // Vc_DEBUG << "lh: " << AVX::addType<short>(l) << AVX::addType<short>(h);
    x = Mem::permuteLo<X1, X0, X3, X2>(Mem::blend<X0, Y1, X2, Y3, X4, Y5, X6, Y7>(l, h));
    y = Mem::permuteHi<X5, X4, X7, X6>(Mem::blend<X0, Y1, X2, Y3, X4, Y5, X6, Y7>(h, l));
    // Vc_DEBUG << "xy: " << AVX::addType<short>(x) << AVX::addType<short>(y);
    l = _mm_min_epu16(x, y);
    h = _mm_max_epu16(x, y);
    // Vc_DEBUG << "lh: " << AVX::addType<short>(l) << AVX::addType<short>(h);

    // merge quads into octs (three min/max)
    x = _mm_unpacklo_epi16(h, l);
    y = _mm_unpackhi_epi16(l, h);
    // Vc_DEBUG << "4x4 sorted xy: " << AVX::addType<short>(x) << AVX::addType<short>(y);
    l = _mm_min_epu16(x, y);
    h = _mm_max_epu16(x, y);
    // Vc_DEBUG << "lh: " << AVX::addType<short>(l) << AVX::addType<short>(h);
    x = Mem::permuteLo<X2, X3, X0, X1>(Mem::blend<X0, X1, Y2, Y3, X4, X5, Y6, Y7>(h, l));
    y = Mem::permuteHi<X6, X7, X4, X5>(Mem::blend<X0, X1, Y2, Y3, X4, X5, Y6, Y7>(l, h));
    // Vc_DEBUG << "xy: " << AVX::addType<short>(x) << AVX::addType<short>(y);
    l = _mm_min_epu16(x, y);
    h = _mm_max_epu16(x, y);
    // Vc_DEBUG << "lh: " << AVX::addType<short>(l) << AVX::addType<short>(h);
    x = Mem::permuteHi<X5, X4, X7, X6>(Mem::blend<X0, Y1, X2, Y3, X4, Y5, X6, Y7>(l, h));
    y = Mem::permuteLo<X1, X0, X3, X2>(Mem::blend<X0, Y1, X2, Y3, X4, Y5, X6, Y7>(h, l));
    // Vc_DEBUG << "xy: " << AVX::addType<short>(x) << AVX::addType<short>(y);
    l = _mm_min_epu16(x, y);
    h = _mm_max_epu16(x, y);
    // Vc_DEBUG << "lh: " << AVX::addType<short>(l) << AVX::addType<short>(h) << " done?";

    // merge octs into hexa (four min/max)
    x = _mm_unpacklo_epi16(l, h);
    y = _mm_unpackhi_epi16(h, l);
    // Vc_DEBUG << "2x8 sorted xy: " << AVX::addType<short>(x) << AVX::addType<short>(y);
    l = _mm_min_epu16(x, y);
    h = _mm_max_epu16(x, y);
    // Vc_DEBUG << "lh: " << AVX::addType<short>(l) << AVX::addType<short>(h);
    x = _mm_unpacklo_epi64(l, h);
    y = _mm_unpackhi_epi64(l, h);
    // Vc_DEBUG << "xy: " << AVX::addType<short>(x) << AVX::addType<short>(y);
    l = _mm_min_epu16(x, y);
    h = _mm_max_epu16(x, y);
    // Vc_DEBUG << "lh: " << AVX::addType<short>(l) << AVX::addType<short>(h);
    x = _mm_castps_si128(Mem::permute<X1, X0, X3, X2>(Mem::blend<X0, Y1, X2, Y3>(_mm_castsi128_ps(h), _mm_castsi128_ps(l))));
    y = _mm_castps_si128(Mem::blend<X0, Y1, X2, Y3>(_mm_castsi128_ps(l), _mm_castsi128_ps(h)));
    // Vc_DEBUG << "xy: " << AVX::addType<short>(x) << AVX::addType<short>(y);
    l = _mm_min_epu16(x, y);
    h = _mm_max_epu16(x, y);
    // Vc_DEBUG << "lh: " << AVX::addType<short>(l) << AVX::addType<short>(h);
    x = Mem::blend<X0, Y1, X2, Y3, X4, Y5, X6, Y7>(l, h);
    y = Mem::permuteLo<X1, X0, X3, X2>(
        Mem::permuteHi<X5, X4, X7, X6>(Mem::blend<X0, Y1, X2, Y3, X4, Y5, X6, Y7>(h, l)));
    // Vc_DEBUG << "xy: " << AVX::addType<short>(x) << AVX::addType<short>(y);
    l = _mm_min_epu16(x, y);
    h = _mm_max_epu16(x, y);
    // Vc_DEBUG << "lh: " << AVX::addType<short>(l) << AVX::addType<short>(h);
    x = _mm_unpacklo_epi16(l, h);
    y = _mm_unpackhi_epi16(l, h);
    return AVX::concat(x, y);
}

template <> Vc_CONST AVX2::int_v sorted<CurrentImplementation::current()>(AVX2::int_v x_)
{
    using namespace AVX;
    const __m256i hgfedcba = x_.data();
    const __m128i hgfe = hi128(hgfedcba);
    const __m128i dcba = lo128(hgfedcba);
    __m128i l = _mm_min_epi32(hgfe, dcba); // ↓hd ↓gc ↓fb ↓ea
    __m128i h = _mm_max_epi32(hgfe, dcba); // ↑hd ↑gc ↑fb ↑ea

    __m128i x = _mm_unpacklo_epi32(l, h); // ↑fb ↓fb ↑ea ↓ea
    __m128i y = _mm_unpackhi_epi32(l, h); // ↑hd ↓hd ↑gc ↓gc

    l = _mm_min_epi32(x, y); // ↓(↑fb,↑hd) ↓hfdb ↓(↑ea,↑gc) ↓geca
    h = _mm_max_epi32(x, y); // ↑hfdb ↑(↓fb,↓hd) ↑geca ↑(↓ea,↓gc)

    x = _mm_min_epi32(l, Reg::permute<X2, X2, X0, X0>(h)); // 2(hfdb) 1(hfdb) 2(geca) 1(geca)
    y = _mm_max_epi32(h, Reg::permute<X3, X3, X1, X1>(l)); // 4(hfdb) 3(hfdb) 4(geca) 3(geca)

    __m128i b = Reg::shuffle<Y0, Y1, X0, X1>(y, x); // b3 <= b2 <= b1 <= b0
    __m128i a = _mm_unpackhi_epi64(x, y);           // a3 >= a2 >= a1 >= a0

    // _mm_extract_epi32 may return an unsigned int, breaking these comparisons.
    if (Vc_IS_UNLIKELY(static_cast<int>(_mm_extract_epi32(x, 2)) >= static_cast<int>(_mm_extract_epi32(y, 1)))) {
        return concat(Reg::permute<X0, X1, X2, X3>(b), a);
    } else if (Vc_IS_UNLIKELY(static_cast<int>(_mm_extract_epi32(x, 0)) >= static_cast<int>(_mm_extract_epi32(y, 3)))) {
        return concat(a, Reg::permute<X0, X1, X2, X3>(b));
    }

    // merge
    l = _mm_min_epi32(a, b); // ↓a3b3 ↓a2b2 ↓a1b1 ↓a0b0
    h = _mm_max_epi32(a, b); // ↑a3b3 ↑a2b2 ↑a1b1 ↑a0b0

    a = _mm_unpacklo_epi32(l, h); // ↑a1b1 ↓a1b1 ↑a0b0 ↓a0b0
    b = _mm_unpackhi_epi32(l, h); // ↑a3b3 ↓a3b3 ↑a2b2 ↓a2b2
    l = _mm_min_epi32(a, b);      // ↓(↑a1b1,↑a3b3) ↓a1b3 ↓(↑a0b0,↑a2b2) ↓a0b2
    h = _mm_max_epi32(a, b);      // ↑a3b1 ↑(↓a1b1,↓a3b3) ↑a2b0 ↑(↓a0b0,↓a2b2)

    a = _mm_unpacklo_epi32(l, h); // ↑a2b0 ↓(↑a0b0,↑a2b2) ↑(↓a0b0,↓a2b2) ↓a0b2
    b = _mm_unpackhi_epi32(l, h); // ↑a3b1 ↓(↑a1b1,↑a3b3) ↑(↓a1b1,↓a3b3) ↓a1b3
    l = _mm_min_epi32(a, b); // ↓(↑a2b0,↑a3b1) ↓(↑a0b0,↑a2b2,↑a1b1,↑a3b3) ↓(↑(↓a0b0,↓a2b2) ↑(↓a1b1,↓a3b3)) ↓a0b3
    h = _mm_max_epi32(a, b); // ↑a3b0 ↑(↓(↑a0b0,↑a2b2) ↓(↑a1b1,↑a3b3)) ↑(↓a0b0,↓a2b2,↓a1b1,↓a3b3) ↑(↓a0b2,↓a1b3)

    return concat(_mm_unpacklo_epi32(l, h), _mm_unpackhi_epi32(l, h));
}

template <>
Vc_CONST AVX2::uint_v sorted<CurrentImplementation::current()>(AVX2::uint_v x_)
{
    using namespace AVX;
    const __m256i hgfedcba = x_.data();
    const __m128i hgfe = hi128(hgfedcba);
    const __m128i dcba = lo128(hgfedcba);
    __m128i l = _mm_min_epu32(hgfe, dcba); // ↓hd ↓gc ↓fb ↓ea
    __m128i h = _mm_max_epu32(hgfe, dcba); // ↑hd ↑gc ↑fb ↑ea

    __m128i x = _mm_unpacklo_epi32(l, h); // ↑fb ↓fb ↑ea ↓ea
    __m128i y = _mm_unpackhi_epi32(l, h); // ↑hd ↓hd ↑gc ↓gc

    l = _mm_min_epu32(x, y); // ↓(↑fb,↑hd) ↓hfdb ↓(↑ea,↑gc) ↓geca
    h = _mm_max_epu32(x, y); // ↑hfdb ↑(↓fb,↓hd) ↑geca ↑(↓ea,↓gc)

    x = _mm_min_epu32(l, Reg::permute<X2, X2, X0, X0>(h)); // 2(hfdb) 1(hfdb) 2(geca) 1(geca)
    y = _mm_max_epu32(h, Reg::permute<X3, X3, X1, X1>(l)); // 4(hfdb) 3(hfdb) 4(geca) 3(geca)

    __m128i b = Reg::shuffle<Y0, Y1, X0, X1>(y, x); // b3 <= b2 <= b1 <= b0
    __m128i a = _mm_unpackhi_epi64(x, y);           // a3 >= a2 >= a1 >= a0

    if (Vc_IS_UNLIKELY(extract_epu32<2>(x) >= extract_epu32<1>(y))) {
        return concat(Reg::permute<X0, X1, X2, X3>(b), a);
    } else if (Vc_IS_UNLIKELY(extract_epu32<0>(x) >= extract_epu32<3>(y))) {
        return concat(a, Reg::permute<X0, X1, X2, X3>(b));
    }

    // merge
    l = _mm_min_epu32(a, b); // ↓a3b3 ↓a2b2 ↓a1b1 ↓a0b0
    h = _mm_max_epu32(a, b); // ↑a3b3 ↑a2b2 ↑a1b1 ↑a0b0

    a = _mm_unpacklo_epi32(l, h); // ↑a1b1 ↓a1b1 ↑a0b0 ↓a0b0
    b = _mm_unpackhi_epi32(l, h); // ↑a3b3 ↓a3b3 ↑a2b2 ↓a2b2
    l = _mm_min_epu32(a, b);      // ↓(↑a1b1,↑a3b3) ↓a1b3 ↓(↑a0b0,↑a2b2) ↓a0b2
    h = _mm_max_epu32(a, b);      // ↑a3b1 ↑(↓a1b1,↓a3b3) ↑a2b0 ↑(↓a0b0,↓a2b2)

    a = _mm_unpacklo_epi32(l, h); // ↑a2b0 ↓(↑a0b0,↑a2b2) ↑(↓a0b0,↓a2b2) ↓a0b2
    b = _mm_unpackhi_epi32(l, h); // ↑a3b1 ↓(↑a1b1,↑a3b3) ↑(↓a1b1,↓a3b3) ↓a1b3
    l = _mm_min_epu32(a, b); // ↓(↑a2b0,↑a3b1) ↓(↑a0b0,↑a2b2,↑a1b1,↑a3b3) ↓(↑(↓a0b0,↓a2b2) ↑(↓a1b1,↓a3b3)) ↓a0b3
    h = _mm_max_epu32(a, b); // ↑a3b0 ↑(↓(↑a0b0,↑a2b2) ↓(↑a1b1,↑a3b3)) ↑(↓a0b0,↓a2b2,↓a1b1,↓a3b3) ↑(↓a0b2,↓a1b3)

    return concat(_mm_unpacklo_epi32(l, h), _mm_unpackhi_epi32(l, h));
}
#endif  // AVX2

template <>
Vc_CONST AVX2::float_v sorted<CurrentImplementation::current()>(AVX2::float_v x_)
{
    __m256 hgfedcba = x_.data();
    const __m128 hgfe = AVX::hi128(hgfedcba);
    const __m128 dcba = AVX::lo128(hgfedcba);
    __m128 l = _mm_min_ps(hgfe, dcba); // ↓hd ↓gc ↓fb ↓ea
    __m128 h = _mm_max_ps(hgfe, dcba); // ↑hd ↑gc ↑fb ↑ea

    __m128 x = _mm_unpacklo_ps(l, h); // ↑fb ↓fb ↑ea ↓ea
    __m128 y = _mm_unpackhi_ps(l, h); // ↑hd ↓hd ↑gc ↓gc

    l = _mm_min_ps(x, y); // ↓(↑fb,↑hd) ↓hfdb ↓(↑ea,↑gc) ↓geca
    h = _mm_max_ps(x, y); // ↑hfdb ↑(↓fb,↓hd) ↑geca ↑(↓ea,↓gc)

    x = _mm_min_ps(l, Reg::permute<X2, X2, X0, X0>(h)); // 2(hfdb) 1(hfdb) 2(geca) 1(geca)
    y = _mm_max_ps(h, Reg::permute<X3, X3, X1, X1>(l)); // 4(hfdb) 3(hfdb) 4(geca) 3(geca)

    __m128 a = _mm_castpd_ps(_mm_unpackhi_pd(_mm_castps_pd(x), _mm_castps_pd(y))); // a3 >= a2 >= a1 >= a0
    __m128 b = Reg::shuffle<Y0, Y1, X0, X1>(y, x); // b3 <= b2 <= b1 <= b0

    // merge
    l = _mm_min_ps(a, b); // ↓a3b3 ↓a2b2 ↓a1b1 ↓a0b0
    h = _mm_max_ps(a, b); // ↑a3b3 ↑a2b2 ↑a1b1 ↑a0b0

    a = _mm_unpacklo_ps(l, h); // ↑a1b1 ↓a1b1 ↑a0b0 ↓a0b0
    b = _mm_unpackhi_ps(l, h); // ↑a3b3 ↓a3b3 ↑a2b2 ↓a2b2
    l = _mm_min_ps(a, b);      // ↓(↑a1b1,↑a3b3) ↓a1b3 ↓(↑a0b0,↑a2b2) ↓a0b2
    h = _mm_max_ps(a, b);      // ↑a3b1 ↑(↓a1b1,↓a3b3) ↑a2b0 ↑(↓a0b0,↓a2b2)

    a = _mm_unpacklo_ps(l, h); // ↑a2b0 ↓(↑a0b0,↑a2b2) ↑(↓a0b0,↓a2b2) ↓a0b2
    b = _mm_unpackhi_ps(l, h); // ↑a3b1 ↓(↑a1b1,↑a3b3) ↑(↓a1b1,↓a3b3) ↓a1b3
    l = _mm_min_ps(a, b); // ↓(↑a2b0,↑a3b1) ↓(↑a0b0,↑a2b2,↑a1b1,↑a3b3) ↓(↑(↓a0b0,↓a2b2),↑(↓a1b1,↓a3b3)) ↓a0b3
    h = _mm_max_ps(a, b); // ↑a3b0 ↑(↓(↑a0b0,↑a2b2),↓(↑a1b1,↑a3b3)) ↑(↓a0b0,↓a2b2,↓a1b1,↓a3b3) ↑(↓a0b2,↓a1b3)

    return AVX::concat(_mm_unpacklo_ps(l, h), _mm_unpackhi_ps(l, h));
}

#if 0
template<> void SortHelper<double>::sort(__m256d &Vc_RESTRICT x, __m256d &Vc_RESTRICT y)
{
    __m256d l = _mm256_min_pd(x, y); // ↓x3y3 ↓x2y2 ↓x1y1 ↓x0y0
    __m256d h = _mm256_max_pd(x, y); // ↑x3y3 ↑x2y2 ↑x1y1 ↑x0y0
    x = _mm256_unpacklo_pd(l, h); // ↑x2y2 ↓x2y2 ↑x0y0 ↓x0y0
    y = _mm256_unpackhi_pd(l, h); // ↑x3y3 ↓x3y3 ↑x1y1 ↓x1y1
    l = _mm256_min_pd(x, y); // ↓(↑x2y2,↑x3y3) ↓x3x2y3y2 ↓(↑x0y0,↑x1y1) ↓x1x0y1y0
    h = _mm256_max_pd(x, y); // ↑x3x2y3y2 ↑(↓x2y2,↓x3y3) ↑x1x0y1y0 ↑(↓x0y0,↓x1y1)
    x = _mm256_unpacklo_pd(l, h); // ↑(↓x2y2,↓x3y3) ↓x3x2y3y2 ↑(↓x0y0,↓x1y1) ↓x1x0y1y0
    y = _mm256_unpackhi_pd(h, l); // ↓(↑x2y2,↑x3y3) ↑x3x2y3y2 ↓(↑x0y0,↑x1y1) ↑x1x0y1y0
    l = _mm256_min_pd(x, y); // ↓(↑(↓x2y2,↓x3y3) ↓(↑x2y2,↑x3y3)) ↓x3x2y3y2 ↓(↑(↓x0y0,↓x1y1) ↓(↑x0y0,↑x1y1)) ↓x1x0y1y0
    h = _mm256_max_pd(x, y); // ↑(↑(↓x2y2,↓x3y3) ↓(↑x2y2,↑x3y3)) ↑x3x2y3y2 ↑(↑(↓x0y0,↓x1y1) ↓(↑x0y0,↑x1y1)) ↑x1x0y1y0
    __m256d a = Reg::permute<X2, X3, X1, X0>(Reg::permute128<X0, X1>(h, h)); // h0 h1 h3 h2
    __m256d b = Reg::permute<X2, X3, X1, X0>(l);                             // l2 l3 l1 l0

    // a3 >= a2 >= b1 >= b0
    // b3 <= b2 <= a1 <= a0

    // merge
    l = _mm256_min_pd(a, b); // ↓a3b3 ↓a2b2 ↓a1b1 ↓a0b0
    h = _mm256_min_pd(a, b); // ↑a3b3 ↑a2b2 ↑a1b1 ↑a0b0

    x = _mm256_unpacklo_pd(l, h); // ↑a2b2 ↓a2b2 ↑a0b0 ↓a0b0
    y = _mm256_unpackhi_pd(l, h); // ↑a3b3 ↓a3b3 ↑a1b1 ↓a1b1
    l = _mm256_min_pd(x, y);      // ↓(↑a2b2,↑a3b3) ↓a2b3 ↓(↑a0b0,↑a1b1) ↓a1b0
    h = _mm256_min_pd(x, y);      // ↑a3b2 ↑(↓a2b2,↓a3b3) ↑a0b1 ↑(↓a0b0,↓a1b1)

    x = Reg::permute128<Y0, X0>(l, h); // ↑a0b1 ↑(↓a0b0,↓a1b1) ↓(↑a0b0,↑a1b1) ↓a1b0
    y = Reg::permute128<Y1, X1>(l, h); // ↑a3b2 ↑(↓a2b2,↓a3b3) ↓(↑a2b2,↑a3b3) ↓a2b3
    l = _mm256_min_pd(x, y);      // ↓(↑a0b1,↑a3b2) ↓(↑(↓a0b0,↓a1b1) ↑(↓a2b2,↓a3b3)) ↓(↑a0b0,↑a1b1,↑a2b2,↑a3b3) ↓b0b3
    h = _mm256_min_pd(x, y);      // ↑a0a3 ↑(↓a0b0,↓a1b1,↓a2b2,↓a3b3) ↑(↓(↑a0b0,↑a1b1) ↓(↑a2b2,↑a3b3)) ↑(↓a1b0,↓a2b3)

    x = _mm256_unpacklo_pd(l, h); // h2 l2 h0 l0
    y = _mm256_unpackhi_pd(l, h); // h3 l3 h1 l1
}
#endif
template <>
Vc_CONST AVX2::double_v sorted<CurrentImplementation::current()>(AVX2::double_v x_)
{
    __m256d dcba = x_.data();
    /*
     * to find the second largest number find
     * max(min(max(ab),max(cd)), min(max(ad),max(bc)))
     *  or
     * max(max(min(ab),min(cd)), min(max(ab),max(cd)))
     *
    const __m256d adcb = avx_cast<__m256d>(AVX::concat(_mm_alignr_epi8(avx_cast<__m128i>(dc), avx_cast<__m128i>(ba), 8), _mm_alignr_epi8(avx_cast<__m128i>(ba), avx_cast<__m128i>(dc), 8)));
    const __m256d l = _mm256_min_pd(dcba, adcb); // min(ad cd bc ab)
    const __m256d h = _mm256_max_pd(dcba, adcb); // max(ad cd bc ab)
    // max(h3, h1)
    // max(min(h0,h2), min(h3,h1))
    // min(max(l0,l2), max(l3,l1))
    // min(l3, l1)

    const __m256d ll = _mm256_min_pd(h, Reg::permute128<X0, X1>(h, h)); // min(h3h1 h2h0 h1h3 h0h2)
    //const __m256d hh = _mm256_max_pd(h3 ll1_3 l1 l0, h1 ll0_2 l3 l2);
    const __m256d hh = _mm256_max_pd(
            Reg::permute128<X1, Y0>(_mm256_unpackhi_pd(ll, h), l),
            Reg::permute128<X0, Y1>(_mm256_blend_pd(h ll, 0x1), l));
    _mm256_min_pd(hh0, hh1
     */

    //////////////////////////////////////////////////////////////////////////////////
    // max(max(ac), max(bd))
    // max(max(min(ac),min(bd)), min(max(ac),max(bd)))
    // min(max(min(ac),min(bd)), min(max(ac),max(bd)))
    // min(min(ac), min(bd))
    __m128d l = _mm_min_pd(AVX::lo128(dcba), AVX::hi128(dcba)); // min(bd) min(ac)
    __m128d h = _mm_max_pd(AVX::lo128(dcba), AVX::hi128(dcba)); // max(bd) max(ac)
    __m128d h0_l0 = _mm_unpacklo_pd(l, h);
    __m128d h1_l1 = _mm_unpackhi_pd(l, h);
    l = _mm_min_pd(h0_l0, h1_l1);
    h = _mm_max_pd(h0_l0, h1_l1);
    return AVX::concat(
        _mm_min_pd(l, Reg::permute<X0, X0>(h)),
        _mm_max_pd(h, Reg::permute<X1, X1>(l))
            );
    // extract: 1 cycle
    // min/max: 4 cycles
    // unpacklo/hi: 2 cycles
    // min/max: 4 cycles
    // permute: 1 cycle
    // min/max: 4 cycles
    // insert:  1 cycle
    // ----------------------
    // total:   17 cycles

    /*
    __m256d cdab = Reg::permute<X2, X3, X0, X1>(dcba);
    __m256d l = _mm256_min_pd(dcba, cdab);
    __m256d h = _mm256_max_pd(dcba, cdab);
    __m256d maxmin_ba = Reg::permute128<X0, Y0>(l, h);
    __m256d maxmin_dc = Reg::permute128<X1, Y1>(l, h);

    l = _mm256_min_pd(maxmin_ba, maxmin_dc);
    h = _mm256_max_pd(maxmin_ba, maxmin_dc);

    return _mm256_blend_pd(h, l, 0x55);
    */

    /*
    // a b c d
    // b a d c
    // sort pairs
    __m256d y, l, h;
    __m128d l2, h2;
    y = shuffle<X1, Y0, X3, Y2>(x, x);
    l = _mm256_min_pd(x, y); // min[ab ab cd cd]
    h = _mm256_max_pd(x, y); // max[ab ab cd cd]

    // 1 of 2 is at [0]
    // 1 of 4 is at [1]
    // 1 of 4 is at [2]
    // 1 of 2 is at [3]

    // don't be fooled by unpack here. It works differently for AVX pd than for SSE ps
    x = _mm256_unpacklo_pd(l, h); // l_ab h_ab l_cd h_cd
    l2 = _mm_min_pd(AVX::lo128(x), AVX::hi128(x)); // l_abcd l(h_ab hcd)
    h2 = _mm_max_pd(AVX::lo128(x), AVX::hi128(x)); // h(l_ab l_cd) h_abcd

    // either it is:
    return AVX::concat(l2, h2);
    // or:
    // AVX::concat(_mm_unpacklo_pd(l2, h2), _mm_unpackhi_pd(l2, h2));

    // I'd like to have four useful compares
    const __m128d dc = AVX::hi128(dcba);
    const __m128d ba = AVX::lo128(dcba);
    const __m256d adcb = avx_cast<__m256d>(AVX::concat(_mm_alignr_epi8(avx_cast<__m128i>(dc), avx_cast<__m128i>(ba), 8), _mm_alignr_epi8(avx_cast<__m128i>(ba), avx_cast<__m128i>(dc), 8)));

    const int extraCmp = _mm_movemask_pd(_mm_cmpgt_pd(dc, ba));
    // 0x0: d <= b && c <= a
    // 0x1: d <= b && c >  a
    // 0x2: d >  b && c <= a
    // 0x3: d >  b && c >  a

    switch (_mm256_movemask_pd(_mm256_cmpgt_pd(dcba, adcb))) {
    // impossible: 0x0, 0xf
    case 0x1: // a <= b && b <= c && c <= d && d >  a
        // abcd
        return Reg::permute<X2, X3, X0, X1>(Reg::permute<X0, X1>(dcba, dcba));
    case 0x2: // a <= b && b <= c && c >  d && d <= a
        // dabc
        return Reg::permute<X2, X3, X0, X1>(adcb);
    case 0x3: // a <= b && b <= c && c >  d && d >  a
        // a[bd]c
        if (extraCmp & 2) {
            // abdc
            return Reg::permute<X2, X3, X1, X0>(Reg::permute<X0, X1>(dcba, dcba));
        } else {
            // adbc
            return Reg::permute<X3, X2, X0, X1>(adcb);
        }
    case 0x4: // a <= b && b >  c && c <= d && d <= a
        // cdab;
        return Reg::permute<X2, X3, X0, X1>(dcba);
    case 0x5: // a <= b && b >  c && c <= d && d >  a
        // [ac] < [bd]
        switch (extraCmp) {
        case 0x0: // d <= b && c <= a
            // cadb
            return shuffle<>(dcba, bcda);
        case 0x1: // d <= b && c >  a
        case 0x2: // d >  b && c <= a
        case 0x3: // d >  b && c >  a
        }
    case 0x6: // a <= b && b >  c && c >  d && d <= a
        // d[ac]b
    case 0x7: // a <= b && b >  c && c >  d && d >  a
        // adcb;
        return permute<X1, X0, X3, X2>(permute128<X1, X0>(bcda, bcda));
    case 0x8: // a >  b && b <= c && c <= d && d <= a
        return bcda;
    case 0x9: // a >  b && b <= c && c <= d && d >  a
        // b[ac]d;
    case 0xa: // a >  b && b <= c && c >  d && d <= a
        // [ac] > [bd]
    case 0xb: // a >  b && b <= c && c >  d && d >  a
        // badc;
        return permute128<X1, X0>(dcba);
    case 0xc: // a >  b && b >  c && c <= d && d <= a
        // c[bd]a;
    case 0xd: // a >  b && b >  c && c <= d && d >  a
        // cbad;
        return permute<X1, X0, X3, X2>(bcda);
    case 0xe: // a >  b && b >  c && c >  d && d <= a
        return dcba;
    }
    */
}

}  // namespace Detail
}  // namespace Vc

// vim: foldmethod=marker
