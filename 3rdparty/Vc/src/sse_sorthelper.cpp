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

#include <Vc/sse/vector.h>
#include <Vc/sse/macros.h>

namespace Vc_VERSIONED_NAMESPACE
{
namespace Detail
{
template <>
Vc_CONST SSE::short_v sorted<CurrentImplementation::current()>(SSE::short_v x_)
{
    __m128i lo, hi, y, x = x_.data();
    // sort pairs
    y = Mem::permute<X1, X0, X3, X2, X5, X4, X7, X6>(x);
    lo = _mm_min_epi16(x, y);
    hi = _mm_max_epi16(x, y);
    x = SSE::blend_epi16<0xaa>(lo, hi);

    // merge left and right quads
    y = Mem::permute<X3, X2, X1, X0, X7, X6, X5, X4>(x);
    lo = _mm_min_epi16(x, y);
    hi = _mm_max_epi16(x, y);
    x = SSE::blend_epi16<0xcc>(lo, hi);
    y = _mm_srli_si128(x, 2);
    lo = _mm_min_epi16(x, y);
    hi = _mm_max_epi16(x, y);
    x = SSE::blend_epi16<0xaa>(lo, _mm_slli_si128(hi, 2));

    // merge quads into octs
    y = _mm_shuffle_epi32(x, _MM_SHUFFLE(1, 0, 3, 2));
    y = _mm_shufflelo_epi16(y, _MM_SHUFFLE(0, 1, 2, 3));
    lo = _mm_min_epi16(x, y);
    hi = _mm_max_epi16(x, y);

    x = _mm_unpacklo_epi16(lo, hi);
    y = _mm_srli_si128(x, 8);
    lo = _mm_min_epi16(x, y);
    hi = _mm_max_epi16(x, y);

    x = _mm_unpacklo_epi16(lo, hi);
    y = _mm_srli_si128(x, 8);
    lo = _mm_min_epi16(x, y);
    hi = _mm_max_epi16(x, y);

    return _mm_unpacklo_epi16(lo, hi);
}

template <>
Vc_CONST SSE::ushort_v sorted<CurrentImplementation::current()>(SSE::ushort_v x_)
{
    __m128i lo, hi, y, x = x_.data();
    // sort pairs
    y = Mem::permute<X1, X0, X3, X2, X5, X4, X7, X6>(x);
    lo = SSE::min_epu16(x, y);
    hi = SSE::max_epu16(x, y);
    x = SSE::blend_epi16<0xaa>(lo, hi);

    // merge left and right quads
    y = Mem::permute<X3, X2, X1, X0, X7, X6, X5, X4>(x);
    lo = SSE::min_epu16(x, y);
    hi = SSE::max_epu16(x, y);
    x = SSE::blend_epi16<0xcc>(lo, hi);
    y = _mm_srli_si128(x, 2);
    lo = SSE::min_epu16(x, y);
    hi = SSE::max_epu16(x, y);
    x = SSE::blend_epi16<0xaa>(lo, _mm_slli_si128(hi, 2));

    // merge quads into octs
    y = _mm_shuffle_epi32(x, _MM_SHUFFLE(1, 0, 3, 2));
    y = _mm_shufflelo_epi16(y, _MM_SHUFFLE(0, 1, 2, 3));
    lo = SSE::min_epu16(x, y);
    hi = SSE::max_epu16(x, y);

    x = _mm_unpacklo_epi16(lo, hi);
    y = _mm_srli_si128(x, 8);
    lo = SSE::min_epu16(x, y);
    hi = SSE::max_epu16(x, y);

    x = _mm_unpacklo_epi16(lo, hi);
    y = _mm_srli_si128(x, 8);
    lo = SSE::min_epu16(x, y);
    hi = SSE::max_epu16(x, y);

    return _mm_unpacklo_epi16(lo, hi);
}

template <> Vc_CONST SSE::int_v sorted<CurrentImplementation::current()>(SSE::int_v x_)
{
    __m128i x = x_.data();
    /*
    // in 16,67% of the cases the merge can be replaced by an append

    // x = [a b c d]
    // y = [c d a b]
    __m128i y = _mm_shuffle_epi32(x, _MM_SHUFFLE(1, 0, 3, 2));
    __m128i l = SSE::min_epi32(x, y); // min[ac bd ac bd]
    __m128i h = SSE::max_epi32(x, y); // max[ac bd ac bd]
    if (IS_UNLIKELY(_mm_cvtsi128_si32(h) <= l[1])) { // l[0] < h[0] < l[1] < h[1]
        return _mm_unpacklo_epi32(l, h);
    }
    // h[0] > l[1]
    */

    // sort pairs
    __m128i y = _mm_shuffle_epi32(x, _MM_SHUFFLE(2, 3, 0, 1));
    __m128i l = SSE::min_epi32(x, y);
    __m128i h = SSE::max_epi32(x, y);
    x = _mm_unpacklo_epi32(l, h);
    y = _mm_unpackhi_epi32(h, l);

    // sort quads
    l = SSE::min_epi32(x, y);
    h = SSE::max_epi32(x, y);
    x = _mm_unpacklo_epi32(l, h);
    y = _mm_unpackhi_epi64(x, x);

    l = SSE::min_epi32(x, y);
    h = SSE::max_epi32(x, y);
    return _mm_unpacklo_epi32(l, h);
}

template <> Vc_CONST SSE::uint_v sorted<CurrentImplementation::current()>(SSE::uint_v x_)
{
    __m128i x = x_.data();
    __m128i y = _mm_shuffle_epi32(x, _MM_SHUFFLE(2, 3, 0, 1));
    __m128i l = SSE::min_epu32(x, y);
    __m128i h = SSE::max_epu32(x, y);
    x = _mm_unpacklo_epi32(l, h);
    y = _mm_unpackhi_epi32(h, l);

    // sort quads
    l = SSE::min_epu32(x, y);
    h = SSE::max_epu32(x, y);
    x = _mm_unpacklo_epi32(l, h);
    y = _mm_unpackhi_epi64(x, x);

    l = SSE::min_epu32(x, y);
    h = SSE::max_epu32(x, y);
    return _mm_unpacklo_epi32(l, h);
}

template <>
Vc_CONST SSE::float_v sorted<CurrentImplementation::current()>(SSE::float_v x_)
{
    __m128 x = x_.data();
    __m128 y = _mm_shuffle_ps(x, x, _MM_SHUFFLE(2, 3, 0, 1));
    __m128 l = _mm_min_ps(x, y);
    __m128 h = _mm_max_ps(x, y);
    x = _mm_unpacklo_ps(l, h);
    y = _mm_unpackhi_ps(h, l);

    l = _mm_min_ps(x, y);
    h = _mm_max_ps(x, y);
    x = _mm_unpacklo_ps(l, h);
    y = _mm_movehl_ps(x, x);

    l = _mm_min_ps(x, y);
    h = _mm_max_ps(x, y);
    return _mm_unpacklo_ps(l, h);
    // X         __m128 k = _mm_cmpgt_ps(x, y);
    // X         k = _mm_shuffle_ps(k, k, _MM_SHUFFLE(2, 2, 0, 0));
    // X         x = SSE::blendv_ps(x, y, k);
    // X         y = _mm_shuffle_ps(x, x, _MM_SHUFFLE(1, 0, 3, 2));
    // X         k = _mm_cmpgt_ps(x, y);
    // X         k = _mm_shuffle_ps(k, k, _MM_SHUFFLE(1, 0, 1, 0));
    // X         x = SSE::blendv_ps(x, y, k);
    // X         y = _mm_shuffle_ps(x, x, _MM_SHUFFLE(3, 1, 2, 0));
    // X         k = _mm_cmpgt_ps(x, y);
    // X         k = _mm_shuffle_ps(k, k, _MM_SHUFFLE(0, 1, 1, 0));
    // X         return SSE::blendv_ps(x, y, k);
}

}  // namespace Detail
}  // namespace Vc

// vim: foldmethod=marker
