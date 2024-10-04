/*  This file is part of the Vc library. {{{
Copyright © 2013-2015 Matthias Kretz <kretz@kde.org>

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

#include "macros.h"

namespace Vc_VERSIONED_NAMESPACE
{
namespace Detail
{
/*mask_count{{{*/
template<> Vc_INTRINSIC Vc_CONST int mask_count<2>(__m128i k)
{
    int mask = _mm_movemask_pd(_mm_castsi128_pd(k));
    return (mask & 1) + (mask >> 1);
}

template<> Vc_INTRINSIC Vc_CONST int mask_count<4>(__m128i k)
{
#ifdef Vc_IMPL_POPCNT
    return _mm_popcnt_u32(_mm_movemask_ps(_mm_castsi128_ps(k)));
#else
    auto x = _mm_srli_epi32(k, 31);
    x = _mm_add_epi32(x, _mm_shuffle_epi32(x, _MM_SHUFFLE(0, 1, 2, 3)));
    x = _mm_add_epi32(x, _mm_shufflelo_epi16(x, _MM_SHUFFLE(1, 0, 3, 2)));
    return _mm_cvtsi128_si32(x);
#endif
}

template<> Vc_INTRINSIC Vc_CONST int mask_count<8>(__m128i k)
{
#ifdef Vc_IMPL_POPCNT
    return _mm_popcnt_u32(_mm_movemask_epi8(k)) / 2;
#else
    auto x = _mm_srli_epi16(k, 15);
    x = _mm_add_epi16(x, _mm_shuffle_epi32(x, _MM_SHUFFLE(0, 1, 2, 3)));
    x = _mm_add_epi16(x, _mm_shufflelo_epi16(x, _MM_SHUFFLE(0, 1, 2, 3)));
    x = _mm_add_epi16(x, _mm_shufflelo_epi16(x, _MM_SHUFFLE(2, 3, 0, 1)));
    return _mm_extract_epi16(x, 0);
#endif
}

template<> Vc_INTRINSIC Vc_CONST int mask_count<16>(__m128i k)
{
    return Detail::popcnt16(_mm_movemask_epi8(k));
}
/*}}}*/
// mask_to_int/*{{{*/
template<> Vc_INTRINSIC Vc_CONST int mask_to_int<2>(__m128i k)
{
    return _mm_movemask_pd(_mm_castsi128_pd(k));
}
template<> Vc_INTRINSIC Vc_CONST int mask_to_int<4>(__m128i k)
{
    return _mm_movemask_ps(_mm_castsi128_ps(k));
}
template<> Vc_INTRINSIC Vc_CONST int mask_to_int<8>(__m128i k)
{
    return _mm_movemask_epi8(_mm_packs_epi16(k, _mm_setzero_si128()));
}
template<> Vc_INTRINSIC Vc_CONST int mask_to_int<16>(__m128i k)
{
    return _mm_movemask_epi8(k);
}
/*}}}*/
// mask_store/*{{{*/
template <size_t> Vc_ALWAYS_INLINE void mask_store(__m128i k, bool *mem);
template <> Vc_ALWAYS_INLINE void mask_store<16>(__m128i k, bool *mem)
{
    _mm_store_si128(reinterpret_cast<__m128i *>(mem), _mm_and_si128(k, _mm_set1_epi8(1)));
}
template <> Vc_ALWAYS_INLINE void mask_store<8>(__m128i k, bool *mem)
{
    k = _mm_srli_epi16(k, 15);
    const auto k2 = _mm_packs_epi16(k, _mm_setzero_si128());
#ifdef __x86_64__
    *aliasing_cast<int64_t>(mem) = _mm_cvtsi128_si64(k2);
#else
    _mm_store_sd(aliasing_cast<double>(mem), _mm_castsi128_pd(k2));
#endif
}
template <> Vc_ALWAYS_INLINE void mask_store<4>(__m128i k, bool *mem)
{
    *aliasing_cast<int32_t>(mem) = _mm_cvtsi128_si32(
        _mm_packs_epi16(_mm_srli_epi16(_mm_packs_epi32(k, _mm_setzero_si128()), 15),
                        _mm_setzero_si128()));
}
template <> Vc_ALWAYS_INLINE void mask_store<2>(__m128i k, bool *mem)
{
    mem[0] = -SseIntrinsics::extract_epi32<1>(k);
    mem[1] = -SseIntrinsics::extract_epi32<3>(k);
}
/*}}}*/
// mask_load/*{{{*/
template<size_t> Vc_ALWAYS_INLINE __m128 mask_load(const bool *mem);
template<> Vc_ALWAYS_INLINE __m128 mask_load<16>(const bool *mem)
{
    return sse_cast<__m128>(_mm_cmpgt_epi8(
        _mm_load_si128(reinterpret_cast<const __m128i *>(mem)), _mm_setzero_si128()));
}
template<> Vc_ALWAYS_INLINE __m128 mask_load<8>(const bool *mem)
{
#ifdef __x86_64__
    __m128i k = _mm_cvtsi64_si128(*reinterpret_cast<const int64_t *>(mem));
#else
    __m128i k = _mm_castpd_si128(_mm_load_sd(reinterpret_cast<const double *>(mem)));
#endif
    return sse_cast<__m128>(_mm_cmpgt_epi16(_mm_unpacklo_epi8(k, k), _mm_setzero_si128()));
}
template<> Vc_ALWAYS_INLINE __m128 mask_load<4>(const bool *mem)
{
    __m128i k = _mm_cvtsi32_si128(*reinterpret_cast<const int *>(mem));
    k = _mm_cmpgt_epi16(_mm_unpacklo_epi8(k, k), _mm_setzero_si128());
    return sse_cast<__m128>(_mm_unpacklo_epi16(k, k));
}
template<> Vc_ALWAYS_INLINE __m128 mask_load<2>(const bool *mem)
{
    return sse_cast<__m128>(
        _mm_set_epi32(-int(mem[1]), -int(mem[1]), -int(mem[0]), -int(mem[0])));
}
/*}}}*/
// is_equal{{{
template <> Vc_INTRINSIC Vc_CONST bool is_equal<2>(__m128 k1, __m128 k2)
{
    return _mm_movemask_pd(_mm_castps_pd(k1)) == _mm_movemask_pd(_mm_castps_pd(k2));
}
template <> Vc_INTRINSIC Vc_CONST bool is_not_equal<2>(__m128 k1, __m128 k2)
{
    return _mm_movemask_pd(_mm_castps_pd(k1)) != _mm_movemask_pd(_mm_castps_pd(k2));
}

template <> Vc_INTRINSIC Vc_CONST bool is_equal<4>(__m128 k1, __m128 k2)
{
    return _mm_movemask_ps(k1) == _mm_movemask_ps(k2);
}
template <> Vc_INTRINSIC Vc_CONST bool is_not_equal<4>(__m128 k1, __m128 k2)
{
    return _mm_movemask_ps(k1) != _mm_movemask_ps(k2);
}

template <> Vc_INTRINSIC Vc_CONST bool is_equal<8>(__m128 k1, __m128 k2)
{
    return _mm_movemask_epi8(_mm_castps_si128(k1)) ==
           _mm_movemask_epi8(_mm_castps_si128(k2));
}
template <> Vc_INTRINSIC Vc_CONST bool is_not_equal<8>(__m128 k1, __m128 k2)
{
    return _mm_movemask_epi8(_mm_castps_si128(k1)) !=
           _mm_movemask_epi8(_mm_castps_si128(k2));
}

template <> Vc_INTRINSIC Vc_CONST bool is_equal<16>(__m128 k1, __m128 k2)
{
    return _mm_movemask_epi8(_mm_castps_si128(k1)) ==
           _mm_movemask_epi8(_mm_castps_si128(k2));
}
template <> Vc_INTRINSIC Vc_CONST bool is_not_equal<16>(__m128 k1, __m128 k2)
{
    return _mm_movemask_epi8(_mm_castps_si128(k1)) !=
           _mm_movemask_epi8(_mm_castps_si128(k2));
}

// }}}
}  // namespace Detail

template<> Vc_ALWAYS_INLINE void SSE::double_m::store(bool *mem) const
{
    *aliasing_cast<uint16_t>(mem) = _mm_movemask_epi8(dataI()) & 0x0101;
}
template<typename T> Vc_ALWAYS_INLINE void Mask<T, VectorAbi::Sse>::store(bool *mem) const
{
    Detail::mask_store<Size>(dataI(), mem);
}
template<> Vc_ALWAYS_INLINE void SSE::double_m::load(const bool *mem)
{
    d.set(0, MaskBool(mem[0]));
    d.set(1, MaskBool(mem[1]));
}
template <typename T> Vc_ALWAYS_INLINE void Mask<T, VectorAbi::Sse>::load(const bool *mem)
{
    d.v() = sse_cast<VectorType>(Detail::mask_load<Size>(mem));
}

// get / operator[] {{{1
template <>
Vc_INTRINSIC Vc_PURE bool SSE::short_m::get(const SSE::short_m &m, int index) noexcept
{
    return m.shiftMask() & (1 << 2 * index);
}
template <>
Vc_INTRINSIC Vc_PURE bool SSE::ushort_m::get(const SSE::ushort_m &m, int index) noexcept
{
    return m.shiftMask() & (1 << 2 * index);
}

// firstOne {{{1
template<typename T> Vc_ALWAYS_INLINE Vc_PURE int Mask<T, VectorAbi::Sse>::firstOne() const
{
    const int mask = toInt();
#ifdef _MSC_VER
    unsigned long bit;
    _BitScanForward(&bit, mask);
#else
    int bit;
    __asm__("bsf %1,%0" : "=&r"(bit) : "r"(mask));
#endif
    return bit;
}

// generate {{{1
template <typename M, typename G>
Vc_INTRINSIC M generate_impl(G &&gen, std::integral_constant<int, 2>)
{
    return _mm_set_epi64x(gen(1) ? 0xffffffffffffffffull : 0,
                          gen(0) ? 0xffffffffffffffffull : 0);
}
template <typename M, typename G>
Vc_INTRINSIC M generate_impl(G &&gen, std::integral_constant<int, 4>)
{
    return _mm_setr_epi32(gen(0) ? 0xfffffffful : 0, gen(1) ? 0xfffffffful : 0,
                          gen(2) ? 0xfffffffful : 0, gen(3) ? 0xfffffffful : 0);
}
template <typename M, typename G>
Vc_INTRINSIC M generate_impl(G &&gen, std::integral_constant<int, 8>)
{
    return _mm_setr_epi16(gen(0) ? 0xffffu : 0, gen(1) ? 0xffffu : 0,
                          gen(2) ? 0xffffu : 0, gen(3) ? 0xffffu : 0,
                          gen(4) ? 0xffffu : 0, gen(5) ? 0xffffu : 0,
                          gen(6) ? 0xffffu : 0, gen(7) ? 0xffffu : 0);
}
template <typename T>
template <typename G>
Vc_INTRINSIC Mask<T, VectorAbi::Sse> Mask<T, VectorAbi::Sse>::generate(G &&gen)
{
    return generate_impl<Mask<T, VectorAbi::Sse>>(std::forward<G>(gen),
                                  std::integral_constant<int, Size>());
}
// shifted {{{1
template <typename T> Vc_INTRINSIC Vc_PURE Mask<T, VectorAbi::Sse> Mask<T, VectorAbi::Sse>::shifted(int amount) const
{
    switch (amount * int(sizeof(VectorEntryType))) {
    case   0: return *this;
    case   1: return Detail::shifted<  1>(dataI());
    case   2: return Detail::shifted<  2>(dataI());
    case   3: return Detail::shifted<  3>(dataI());
    case   4: return Detail::shifted<  4>(dataI());
    case   5: return Detail::shifted<  5>(dataI());
    case   6: return Detail::shifted<  6>(dataI());
    case   7: return Detail::shifted<  7>(dataI());
    case   8: return Detail::shifted<  8>(dataI());
    case   9: return Detail::shifted<  9>(dataI());
    case  10: return Detail::shifted< 10>(dataI());
    case  11: return Detail::shifted< 11>(dataI());
    case  12: return Detail::shifted< 12>(dataI());
    case  13: return Detail::shifted< 13>(dataI());
    case  14: return Detail::shifted< 14>(dataI());
    case  15: return Detail::shifted< 15>(dataI());
    case  16: return Detail::shifted< 16>(dataI());
    case  -1: return Detail::shifted< -1>(dataI());
    case  -2: return Detail::shifted< -2>(dataI());
    case  -3: return Detail::shifted< -3>(dataI());
    case  -4: return Detail::shifted< -4>(dataI());
    case  -5: return Detail::shifted< -5>(dataI());
    case  -6: return Detail::shifted< -6>(dataI());
    case  -7: return Detail::shifted< -7>(dataI());
    case  -8: return Detail::shifted< -8>(dataI());
    case  -9: return Detail::shifted< -9>(dataI());
    case -10: return Detail::shifted<-10>(dataI());
    case -11: return Detail::shifted<-11>(dataI());
    case -12: return Detail::shifted<-12>(dataI());
    case -13: return Detail::shifted<-13>(dataI());
    case -14: return Detail::shifted<-14>(dataI());
    case -15: return Detail::shifted<-15>(dataI());
    case -16: return Detail::shifted<-16>(dataI());
    }
    return Zero();
}
// }}}1

}

// vim: foldmethod=marker
