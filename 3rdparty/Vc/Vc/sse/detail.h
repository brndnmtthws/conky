/*  This file is part of the Vc library. {{{
Copyright © 2015 Matthias Kretz <kretz@kde.org>

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

#ifndef VC_SSE_DETAIL_H_
#define VC_SSE_DETAIL_H_

#include "casts.h"
#ifdef Vc_IMPL_AVX
#include "../avx/intrinsics.h"
#endif
#include "vectorhelper.h"

#include "macros.h"

namespace Vc_VERSIONED_NAMESPACE
{
namespace Detail
{
// (converting) load functions {{{1
template <typename V, typename DstT> struct LoadTag
{
};

// when_(un)aligned{{{2
class when_aligned
{
public:
    template <typename F> constexpr when_aligned(F, typename F::EnableIfAligned = nullptr)
    {
    }
};

class when_unaligned
{
public:
    template <typename F>
    constexpr when_unaligned(F, typename F::EnableIfUnaligned = nullptr)
    {
    }
};

class when_streaming
{
public:
    template <typename F>
    constexpr when_streaming(F, typename F::EnableIfStreaming = nullptr)
    {
    }
};

// load16{{{2
Vc_INTRINSIC __m128 load16(const float *mem, when_aligned)
{
    return _mm_load_ps(mem);
}
Vc_INTRINSIC __m128 load16(const float *mem, when_unaligned)
{
    return _mm_loadu_ps(mem);
}
Vc_INTRINSIC __m128 load16(const float *mem, when_streaming)
{
    return SseIntrinsics::_mm_stream_load(mem);
}
Vc_INTRINSIC __m128d load16(const double *mem, when_aligned)
{
    return _mm_load_pd(mem);
}
Vc_INTRINSIC __m128d load16(const double *mem, when_unaligned)
{
    return _mm_loadu_pd(mem);
}
Vc_INTRINSIC __m128d load16(const double *mem, when_streaming)
{
    return SseIntrinsics::_mm_stream_load(mem);
}
template <class T> Vc_INTRINSIC __m128i load16(const T *mem, when_aligned)
{
    static_assert(std::is_integral<T>::value, "load16<T> is only intended for integral T");
    return _mm_load_si128(reinterpret_cast<const __m128i *>(mem));
}
template <class T> Vc_INTRINSIC __m128i load16(const T *mem, when_unaligned)
{
    static_assert(std::is_integral<T>::value, "load16<T> is only intended for integral T");
    return _mm_loadu_si128(reinterpret_cast<const __m128i *>(mem));
}
template <class T> Vc_INTRINSIC __m128i load16(const T *mem, when_streaming)
{
    static_assert(std::is_integral<T>::value, "load16<T> is only intended for integral T");
    return SseIntrinsics::_mm_stream_load(mem);
}

// MSVC workarounds{{{2
#ifdef Vc_MSVC
// work around: "fatal error C1001: An internal error has occurred in the compiler."
template <typename V, typename DstT, typename F>
Vc_INTRINSIC __m128d load(const double *mem, F f,
                          enable_if<(std::is_same<DstT, double>::value &&
                                     std::is_same<V, __m128d>::value)> = nullarg)
{
    return load16(mem, f);
}

template <typename V, typename DstT, typename F>
Vc_INTRINSIC __m128 load(const float *mem, F f,
                         enable_if<(std::is_same<DstT, float>::value &&
                                    std::is_same<V, __m128>::value)> = nullarg)
{
    return load16(mem, f);
}

template <typename V, typename DstT, typename F>
Vc_INTRINSIC __m128i load(const uint *mem, F f,
                          enable_if<(std::is_same<DstT, uint>::value &&
                                     std::is_same<V, __m128i>::value)> = nullarg)
{
    return load16(mem, f);
}

template <typename V, typename DstT, typename F>
Vc_INTRINSIC __m128i load(const int *mem, F f,
                          enable_if<(std::is_same<DstT, int>::value &&
                                     std::is_same<V, __m128i>::value)> = nullarg)
{
    return load16(mem, f);
}

template <typename V, typename DstT, typename F>
Vc_INTRINSIC __m128i load(const short *mem, F f,
                          enable_if<(std::is_same<DstT, short>::value &&
                                     std::is_same<V, __m128i>::value)> = nullarg)
{
    return load16(mem, f);
}

template <typename V, typename DstT, typename F>
Vc_INTRINSIC __m128i load(const ushort *mem, F f,
                          enable_if<(std::is_same<DstT, ushort>::value &&
                                     std::is_same<V, __m128i>::value)> = nullarg)
{
    return load16(mem, f);
}
#endif  // Vc_MSVC

// generic load{{{2
template <typename V, typename DstT, typename SrcT, typename Flags,
          typename = enable_if<
#ifdef Vc_MSVC
              !std::is_same<DstT, SrcT>::value &&
#endif
              (!std::is_integral<DstT>::value || !std::is_integral<SrcT>::value ||
               sizeof(DstT) >= sizeof(SrcT))>>
Vc_INTRINSIC V load(const SrcT *mem, Flags flags)
{
    return load(mem, flags, LoadTag<V, DstT>());
}

// no conversion load from any T {{{2
template <typename V, typename T, typename Flags>
Vc_INTRINSIC V
    load(const T *mem, Flags, LoadTag<V, T>, enable_if<sizeof(V) == 16> = nullarg)
{
    return SSE::VectorHelper<V>::template load<Flags>(mem);
}

// short {{{2
template <typename Flags>
Vc_INTRINSIC __m128i load(const ushort *mem, Flags, LoadTag<__m128i, short>)
{
    return SSE::VectorHelper<__m128i>::load<Flags>(mem);
}
template <typename Flags>
Vc_INTRINSIC __m128i load(const uchar *mem, Flags, LoadTag<__m128i, short>)
{
    // the only available streaming load loads 16 bytes - twice as much as we need =>
    // can't use it, or we risk an out-of-bounds read and an unaligned load exception
    return SSE::cvtepu8_epi16(_mm_loadl_epi64(reinterpret_cast<const __m128i *>(mem)));
}
template <typename Flags>
Vc_INTRINSIC __m128i load(const schar *mem, Flags, LoadTag<__m128i, short>)
{
    // the only available streaming load loads 16 bytes - twice as much as we need =>
    // can't use it, or we risk an out-of-bounds read and an unaligned load exception
    return SSE::cvtepi8_epi16(_mm_loadl_epi64(reinterpret_cast<const __m128i *>(mem)));
}

// ushort {{{2
template <typename Flags>
Vc_INTRINSIC __m128i load(const uchar *mem, Flags, LoadTag<__m128i, ushort>)
{
    // the only available streaming load loads 16 bytes - twice as much as we need =>
    // can't use it, or we risk an out-of-bounds read and an unaligned load exception
    return SSE::cvtepu8_epi16(_mm_loadl_epi64(reinterpret_cast<const __m128i *>(mem)));
}

// int {{{2
template <typename Flags>
Vc_INTRINSIC __m128i load(const uint *mem, Flags, LoadTag<__m128i, int>)
{
    return SSE::VectorHelper<__m128i>::load<Flags>(mem);
}
// no difference between streaming and alignment, because the
// 32/64 bit loads are not available as streaming loads, and can always be unaligned
template <typename Flags>
Vc_INTRINSIC __m128i load(const ushort *mem, Flags, LoadTag<__m128i, int>)
{
    return SSE::cvtepu16_epi32(_mm_loadl_epi64(reinterpret_cast<const __m128i *>(mem)));
}
template <typename Flags>
Vc_INTRINSIC __m128i load(const short *mem, Flags, LoadTag<__m128i, int>)
{
    return SSE::cvtepi16_epi32(_mm_loadl_epi64(reinterpret_cast<const __m128i *>(mem)));
}
template <typename Flags>
Vc_INTRINSIC __m128i load(const uchar *mem, Flags, LoadTag<__m128i, int>)
{
    return SSE::cvtepu8_epi32(_mm_cvtsi32_si128(*aliasing_cast<int>(mem)));
}
template <typename Flags>
Vc_INTRINSIC __m128i load(const schar *mem, Flags, LoadTag<__m128i, int>)
{
    return SSE::cvtepi8_epi32(_mm_cvtsi32_si128(*aliasing_cast<int>(mem)));
}

// uint {{{2
template <typename Flags>
Vc_INTRINSIC __m128i load(const ushort *mem, Flags, LoadTag<__m128i, uint>)
{
    return SSE::cvtepu16_epi32(_mm_loadl_epi64(reinterpret_cast<const __m128i *>(mem)));
}
template <typename Flags>
Vc_INTRINSIC __m128i load(const uchar *mem, Flags, LoadTag<__m128i, uint>)
{
    return SSE::cvtepu8_epi32(_mm_cvtsi32_si128(*aliasing_cast<int>(mem)));
}

// double {{{2
template <typename Flags>
Vc_INTRINSIC __m128d load(const float *mem, Flags, LoadTag<__m128d, double>)
{
    return SSE::convert<float, double>(
        _mm_loadl_pi(_mm_setzero_ps(), reinterpret_cast<const __m64 *>(mem)));
}
template <typename Flags>
Vc_INTRINSIC __m128d load(const uint *mem, Flags, LoadTag<__m128d, double>)
{
    return SSE::convert<uint, double>(
        _mm_loadl_epi64(reinterpret_cast<const __m128i *>(mem)));
}
template <typename Flags>
Vc_INTRINSIC __m128d load(const int *mem, Flags, LoadTag<__m128d, double>)
{
    return SSE::convert<int, double>(
        _mm_loadl_epi64(reinterpret_cast<const __m128i *>(mem)));
}
template <typename Flags>
Vc_INTRINSIC __m128d load(const ushort *mem, Flags, LoadTag<__m128d, double>)
{
    return SSE::convert<ushort, double>(
        _mm_cvtsi32_si128(*aliasing_cast<int>(mem)));
}
template <typename Flags>
Vc_INTRINSIC __m128d load(const short *mem, Flags, LoadTag<__m128d, double>)
{
    return SSE::convert<short, double>(
        _mm_cvtsi32_si128(*aliasing_cast<int>(mem)));
}
template <typename Flags>
Vc_INTRINSIC __m128d load(const uchar *mem, Flags, LoadTag<__m128d, double>)
{
    return SSE::convert<uchar, double>(
        _mm_set1_epi16(*aliasing_cast<short>(mem)));
}
template <typename Flags>
Vc_INTRINSIC __m128d load(const schar *mem, Flags, LoadTag<__m128d, double>)
{
    return SSE::convert<char, double>(
        _mm_set1_epi16(*aliasing_cast<short>(mem)));
}

// float {{{2
template <typename Flags>
Vc_INTRINSIC __m128 load(const double *mem, Flags, LoadTag<__m128, float>)
{
#ifdef Vc_IMPL_AVX
    if (Flags::IsUnaligned) {
        return _mm256_cvtpd_ps(_mm256_loadu_pd(mem));
    } else if (Flags::IsStreaming) {
        return _mm256_cvtpd_ps(AvxIntrinsics::stream_load<__m256d>(mem));
    } else {  // Flags::IsAligned
        return _mm256_cvtpd_ps(_mm256_load_pd(mem));
    }
#else
    return _mm_movelh_ps(_mm_cvtpd_ps(SSE::VectorHelper<__m128d>::load<Flags>(&mem[0])),
                         _mm_cvtpd_ps(SSE::VectorHelper<__m128d>::load<Flags>(&mem[2])));
#endif
}
template <typename Flags>
Vc_INTRINSIC __m128 load(const uint *mem, Flags f, LoadTag<__m128, float>)
{
    return SSE::convert<uint, float>(load<__m128i, uint>(mem, f));
}
template <typename T, typename Flags,
          typename = enable_if<!std::is_same<T, float>::value>>
Vc_INTRINSIC __m128 load(const T *mem, Flags f, LoadTag<__m128, float>)
{
    return _mm_cvtepi32_ps(load<__m128i, int>(mem, f));
}

// shifted{{{1
template <int amount, typename T>
Vc_INTRINSIC Vc_CONST enable_if<amount == 0, T> shifted(T k)
{
    return k;
}
template <int amount, typename T>
Vc_INTRINSIC Vc_CONST enable_if<(sizeof(T) == 16 && amount > 0), T> shifted(T k)
{
    return _mm_srli_si128(k, amount);
}
template <int amount, typename T>
Vc_INTRINSIC Vc_CONST enable_if<(sizeof(T) == 16 && amount < 0), T> shifted(T k)
{
    return _mm_slli_si128(k, -amount);
}

// IndexesFromZero{{{1
template <typename T, int Size> Vc_INTRINSIC Vc_CONST const T *IndexesFromZero()
{
    if (Size == 4) {
        return reinterpret_cast<const T *>(SSE::_IndexesFromZero4);
    } else if (Size == 8) {
        return reinterpret_cast<const T *>(SSE::_IndexesFromZero8);
    } else if (Size == 16) {
        return reinterpret_cast<const T *>(SSE::_IndexesFromZero16);
    }
    return 0;
}

// popcnt{{{1
Vc_INTRINSIC Vc_CONST unsigned int popcnt4(unsigned int n)
{
#ifdef Vc_IMPL_POPCNT
    return _mm_popcnt_u32(n);
#else
    n = (n & 0x5U) + ((n >> 1) & 0x5U);
    n = (n & 0x3U) + ((n >> 2) & 0x3U);
    return n;
#endif
}
Vc_INTRINSIC Vc_CONST unsigned int popcnt8(unsigned int n)
{
#ifdef Vc_IMPL_POPCNT
    return _mm_popcnt_u32(n);
#else
    n = (n & 0x55U) + ((n >> 1) & 0x55U);
    n = (n & 0x33U) + ((n >> 2) & 0x33U);
    n = (n & 0x0fU) + ((n >> 4) & 0x0fU);
    return n;
#endif
}
Vc_INTRINSIC Vc_CONST unsigned int popcnt16(unsigned int n)
{
#ifdef Vc_IMPL_POPCNT
    return _mm_popcnt_u32(n);
#else
    n = (n & 0x5555U) + ((n >> 1) & 0x5555U);
    n = (n & 0x3333U) + ((n >> 2) & 0x3333U);
    n = (n & 0x0f0fU) + ((n >> 4) & 0x0f0fU);
    n = (n & 0x00ffU) + ((n >> 8) & 0x00ffU);
    return n;
#endif
}
Vc_INTRINSIC Vc_CONST unsigned int popcnt32(unsigned int n)
{
#ifdef Vc_IMPL_POPCNT
    return _mm_popcnt_u32(n);
#else
    n = (n & 0x55555555U) + ((n >> 1) & 0x55555555U);
    n = (n & 0x33333333U) + ((n >> 2) & 0x33333333U);
    n = (n & 0x0f0f0f0fU) + ((n >> 4) & 0x0f0f0f0fU);
    n = (n & 0x00ff00ffU) + ((n >> 8) & 0x00ff00ffU);
    n = (n & 0x0000ffffU) + ((n >>16) & 0x0000ffffU);
    return n;
#endif
}

// mask_cast{{{1
template<size_t From, size_t To, typename R> Vc_INTRINSIC Vc_CONST R mask_cast(__m128i k)
{
    static_assert(From == To, "Incorrect mask cast.");
    static_assert(std::is_same<R, __m128>::value, "Incorrect mask cast.");
    return SSE::sse_cast<__m128>(k);
}

template<> Vc_INTRINSIC Vc_CONST __m128 mask_cast<2, 4, __m128>(__m128i k)
{
    return SSE::sse_cast<__m128>(_mm_packs_epi16(k, _mm_setzero_si128()));
}
template<> Vc_INTRINSIC Vc_CONST __m128 mask_cast<2, 8, __m128>(__m128i k)
{
    return SSE::sse_cast<__m128>(
        _mm_packs_epi16(_mm_packs_epi16(k, _mm_setzero_si128()), _mm_setzero_si128()));
}

template<> Vc_INTRINSIC Vc_CONST __m128 mask_cast<4, 2, __m128>(__m128i k)
{
    return SSE::sse_cast<__m128>(_mm_unpacklo_epi32(k, k));
}
template<> Vc_INTRINSIC Vc_CONST __m128 mask_cast<4, 8, __m128>(__m128i k)
{
    return SSE::sse_cast<__m128>(_mm_packs_epi16(k, _mm_setzero_si128()));
}

template<> Vc_INTRINSIC Vc_CONST __m128 mask_cast<8, 2, __m128>(__m128i k)
{
    const auto tmp = _mm_unpacklo_epi16(k, k);
    return SSE::sse_cast<__m128>(_mm_unpacklo_epi32(tmp, tmp));
}
template<> Vc_INTRINSIC Vc_CONST __m128 mask_cast<8, 4, __m128>(__m128i k)
{
    return SSE::sse_cast<__m128>(_mm_unpacklo_epi16(k, k));
}

template<> Vc_INTRINSIC Vc_CONST __m128 mask_cast<16, 8, __m128>(__m128i k)
{
    return SSE::sse_cast<__m128>(_mm_unpacklo_epi8(k, k));
}
template<> Vc_INTRINSIC Vc_CONST __m128 mask_cast<16, 4, __m128>(__m128i k)
{
    const auto tmp = SSE::sse_cast<__m128i>(mask_cast<16, 8, __m128>(k));
    return SSE::sse_cast<__m128>(_mm_unpacklo_epi16(tmp, tmp));
}
template<> Vc_INTRINSIC Vc_CONST __m128 mask_cast<16, 2, __m128>(__m128i k)
{
    const auto tmp = SSE::sse_cast<__m128i>(mask_cast<16, 4, __m128>(k));
    return SSE::sse_cast<__m128>(_mm_unpacklo_epi32(tmp, tmp));
}

// allone{{{1
template <typename V> Vc_INTRINSIC_L Vc_CONST_L V allone() Vc_INTRINSIC_R Vc_CONST_R;
template<> Vc_INTRINSIC Vc_CONST __m128  allone<__m128 >() { return SSE::_mm_setallone_ps(); }
template<> Vc_INTRINSIC Vc_CONST __m128i allone<__m128i>() { return SSE::_mm_setallone_si128(); }
template<> Vc_INTRINSIC Vc_CONST __m128d allone<__m128d>() { return SSE::_mm_setallone_pd(); }

// zero{{{1
template <typename V> inline V zero();
template<> Vc_INTRINSIC Vc_CONST __m128  zero<__m128 >() { return _mm_setzero_ps(); }
template<> Vc_INTRINSIC Vc_CONST __m128i zero<__m128i>() { return _mm_setzero_si128(); }
template<> Vc_INTRINSIC Vc_CONST __m128d zero<__m128d>() { return _mm_setzero_pd(); }

// negate{{{1
Vc_ALWAYS_INLINE Vc_CONST __m128 negate(__m128 v, std::integral_constant<std::size_t, 4>)
{
    return _mm_xor_ps(v, SSE::_mm_setsignmask_ps());
}
Vc_ALWAYS_INLINE Vc_CONST __m128d negate(__m128d v, std::integral_constant<std::size_t, 8>)
{
    return _mm_xor_pd(v, SSE::_mm_setsignmask_pd());
}
Vc_ALWAYS_INLINE Vc_CONST __m128i negate(__m128i v, std::integral_constant<std::size_t, 4>)
{
#ifdef Vc_IMPL_SSSE3
    return _mm_sign_epi32(v, allone<__m128i>());
#else
    return _mm_sub_epi32(_mm_setzero_si128(), v);
#endif
}
Vc_ALWAYS_INLINE Vc_CONST __m128i negate(__m128i v, std::integral_constant<std::size_t, 2>)
{
#ifdef Vc_IMPL_SSSE3
    return _mm_sign_epi16(v, allone<__m128i>());
#else
    return _mm_sub_epi16(_mm_setzero_si128(), v);
#endif
}

// xor_{{{1
Vc_INTRINSIC __m128 xor_(__m128 a, __m128 b) { return _mm_xor_ps(a, b); }
Vc_INTRINSIC __m128d xor_(__m128d a, __m128d b) { return _mm_xor_pd(a, b); }
Vc_INTRINSIC __m128i xor_(__m128i a, __m128i b) { return _mm_xor_si128(a, b); }

// or_{{{1
Vc_INTRINSIC __m128 or_(__m128 a, __m128 b) { return _mm_or_ps(a, b); }
Vc_INTRINSIC __m128d or_(__m128d a, __m128d b) { return _mm_or_pd(a, b); }
Vc_INTRINSIC __m128i or_(__m128i a, __m128i b) { return _mm_or_si128(a, b); }

// and_{{{1
Vc_INTRINSIC __m128 and_(__m128 a, __m128 b) { return _mm_and_ps(a, b); }
Vc_INTRINSIC __m128d and_(__m128d a, __m128d b) { return _mm_and_pd(a, b); }
Vc_INTRINSIC __m128i and_(__m128i a, __m128i b) { return _mm_and_si128(a, b); }

// andnot_{{{1
Vc_INTRINSIC __m128 andnot_(__m128 a, __m128 b) { return _mm_andnot_ps(a, b); }
Vc_INTRINSIC __m128d andnot_(__m128d a, __m128d b) { return _mm_andnot_pd(a, b); }
Vc_INTRINSIC __m128i andnot_(__m128i a, __m128i b) { return _mm_andnot_si128(a, b); }

// not_{{{1
Vc_INTRINSIC __m128  not_(__m128  a) { return andnot_(a, allone<__m128 >()); }
Vc_INTRINSIC __m128d not_(__m128d a) { return andnot_(a, allone<__m128d>()); }
Vc_INTRINSIC __m128i not_(__m128i a) { return andnot_(a, allone<__m128i>()); }

// add{{{1
Vc_INTRINSIC __m128  add(__m128  a, __m128  b,  float) { return _mm_add_ps(a, b); }
Vc_INTRINSIC __m128d add(__m128d a, __m128d b, double) { return _mm_add_pd(a, b); }
Vc_INTRINSIC __m128i add(__m128i a, __m128i b,    int) { return _mm_add_epi32(a, b); }
Vc_INTRINSIC __m128i add(__m128i a, __m128i b,   uint) { return _mm_add_epi32(a, b); }
Vc_INTRINSIC __m128i add(__m128i a, __m128i b,  short) { return _mm_add_epi16(a, b); }
Vc_INTRINSIC __m128i add(__m128i a, __m128i b, ushort) { return _mm_add_epi16(a, b); }
Vc_INTRINSIC __m128i add(__m128i a, __m128i b,  schar) { return _mm_add_epi8 (a, b); }
Vc_INTRINSIC __m128i add(__m128i a, __m128i b,  uchar) { return _mm_add_epi8 (a, b); }

// sub{{{1
Vc_INTRINSIC __m128  sub(__m128  a, __m128  b,  float) { return _mm_sub_ps(a, b); }
Vc_INTRINSIC __m128d sub(__m128d a, __m128d b, double) { return _mm_sub_pd(a, b); }
Vc_INTRINSIC __m128i sub(__m128i a, __m128i b,    int) { return _mm_sub_epi32(a, b); }
Vc_INTRINSIC __m128i sub(__m128i a, __m128i b,   uint) { return _mm_sub_epi32(a, b); }
Vc_INTRINSIC __m128i sub(__m128i a, __m128i b,  short) { return _mm_sub_epi16(a, b); }
Vc_INTRINSIC __m128i sub(__m128i a, __m128i b, ushort) { return _mm_sub_epi16(a, b); }
Vc_INTRINSIC __m128i sub(__m128i a, __m128i b,  schar) { return _mm_sub_epi8 (a, b); }
Vc_INTRINSIC __m128i sub(__m128i a, __m128i b,  uchar) { return _mm_sub_epi8 (a, b); }

// mul{{{1
Vc_INTRINSIC __m128  mul(__m128  a, __m128  b,  float) { return _mm_mul_ps(a, b); }
Vc_INTRINSIC __m128d mul(__m128d a, __m128d b, double) { return _mm_mul_pd(a, b); }
Vc_INTRINSIC __m128i mul(__m128i a, __m128i b,    int) {
#ifdef Vc_IMPL_SSE4_1
    return _mm_mullo_epi32(a, b);
#else
    const __m128i aShift = _mm_srli_si128(a, 4);
    const __m128i ab02 = _mm_mul_epu32(a, b);  // [a0 * b0, a2 * b2]
    const __m128i bShift = _mm_srli_si128(b, 4);
    const __m128i ab13 = _mm_mul_epu32(aShift, bShift);  // [a1 * b1, a3 * b3]
    return _mm_unpacklo_epi32(_mm_shuffle_epi32(ab02, 8), _mm_shuffle_epi32(ab13, 8));
#endif
}
Vc_INTRINSIC __m128i mul(__m128i a, __m128i b,   uint) { return mul(a, b, int()); }
Vc_INTRINSIC __m128i mul(__m128i a, __m128i b,  short) { return _mm_mullo_epi16(a, b); }
Vc_INTRINSIC __m128i mul(__m128i a, __m128i b, ushort) { return _mm_mullo_epi16(a, b); }
Vc_INTRINSIC __m128i mul(__m128i a, __m128i b,  schar) {
#ifdef Vc_USE_BUILTIN_VECTOR_TYPES
    using B = Common::BuiltinType<schar, 16>;
    const auto x = aliasing_cast<B>(a) * aliasing_cast<B>(b);
    return reinterpret_cast<const __m128i &>(x);
#else
    return or_(
        and_(_mm_mullo_epi16(a, b), _mm_slli_epi16(allone<__m128i>(), 8)),
        _mm_slli_epi16(_mm_mullo_epi16(_mm_srli_si128(a, 1), _mm_srli_si128(b, 1)), 8));
#endif
}
Vc_INTRINSIC __m128i mul(__m128i a, __m128i b,  uchar) {
#ifdef Vc_USE_BUILTIN_VECTOR_TYPES
    using B = Common::BuiltinType<uchar, 16>;
    const auto x = aliasing_cast<B>(a) * aliasing_cast<B>(b);
    return reinterpret_cast<const __m128i &>(x);
#else
    return or_(
        and_(_mm_mullo_epi16(a, b), _mm_slli_epi16(allone<__m128i>(), 8)),
        _mm_slli_epi16(_mm_mullo_epi16(_mm_srli_si128(a, 1), _mm_srli_si128(b, 1)), 8));
#endif
}

// div{{{1
Vc_INTRINSIC __m128  div(__m128  a, __m128  b,  float) { return _mm_div_ps(a, b); }
Vc_INTRINSIC __m128d div(__m128d a, __m128d b, double) { return _mm_div_pd(a, b); }

// TODO: fma{{{1
//Vc_INTRINSIC __m128  fma(__m128  a, __m128  b, __m128  c,  float) { return _mm_mul_ps(a, b); }
//Vc_INTRINSIC __m128d fma(__m128d a, __m128d b, __m128d c, double) { return _mm_mul_pd(a, b); }
//Vc_INTRINSIC __m128i fma(__m128i a, __m128i b, __m128i c,    int) { }
//Vc_INTRINSIC __m128i fma(__m128i a, __m128i b, __m128i c,   uint) { return fma(a, b, int()); }
//Vc_INTRINSIC __m128i fma(__m128i a, __m128i b, __m128i c,  short) { return _mm_mullo_epi16(a, b); }
//Vc_INTRINSIC __m128i fma(__m128i a, __m128i b, __m128i c, ushort) { return _mm_mullo_epi16(a, b); }
//Vc_INTRINSIC __m128i fma(__m128i a, __m128i b, __m128i c,  schar) { }
//Vc_INTRINSIC __m128i fma(__m128i a, __m128i b, __m128i c,  uchar) { }

// min{{{1
Vc_INTRINSIC __m128  min(__m128  a, __m128  b,  float) { return _mm_min_ps(a, b); }
Vc_INTRINSIC __m128d min(__m128d a, __m128d b, double) { return _mm_min_pd(a, b); }
Vc_INTRINSIC __m128i min(__m128i a, __m128i b,    int) { return SSE::min_epi32(a, b); }
Vc_INTRINSIC __m128i min(__m128i a, __m128i b,   uint) { return SSE::min_epu32(a, b); }
Vc_INTRINSIC __m128i min(__m128i a, __m128i b,  short) { return _mm_min_epi16(a, b); }
Vc_INTRINSIC __m128i min(__m128i a, __m128i b, ushort) { return SSE::min_epu16(a, b); }
Vc_INTRINSIC __m128i min(__m128i a, __m128i b,  schar) { return SSE::min_epi8 (a, b); }
Vc_INTRINSIC __m128i min(__m128i a, __m128i b,  uchar) { return _mm_min_epu8 (a, b); }

// max{{{1
Vc_INTRINSIC __m128  max(__m128  a, __m128  b,  float) { return _mm_max_ps(a, b); }
Vc_INTRINSIC __m128d max(__m128d a, __m128d b, double) { return _mm_max_pd(a, b); }
Vc_INTRINSIC __m128i max(__m128i a, __m128i b,    int) { return SSE::max_epi32(a, b); }
Vc_INTRINSIC __m128i max(__m128i a, __m128i b,   uint) { return SSE::max_epu32(a, b); }
Vc_INTRINSIC __m128i max(__m128i a, __m128i b,  short) { return _mm_max_epi16(a, b); }
Vc_INTRINSIC __m128i max(__m128i a, __m128i b, ushort) { return SSE::max_epu16(a, b); }
Vc_INTRINSIC __m128i max(__m128i a, __m128i b,  schar) { return SSE::max_epi8 (a, b); }
Vc_INTRINSIC __m128i max(__m128i a, __m128i b,  uchar) { return _mm_max_epu8 (a, b); }

// horizontal add{{{1
Vc_INTRINSIC  float add(__m128  a,  float) {
    a = _mm_add_ps(a, _mm_movehl_ps(a, a));
    a = _mm_add_ss(a, _mm_shuffle_ps(a, a, _MM_SHUFFLE(1, 1, 1, 1)));
    return _mm_cvtss_f32(a);
}
Vc_INTRINSIC double add(__m128d a, double) {
    a = _mm_add_sd(a, _mm_unpackhi_pd(a, a));
    return _mm_cvtsd_f64(a);
}
Vc_INTRINSIC    int add(__m128i a,    int) {
    a = add(a, _mm_srli_si128(a, 8), int());
    a = add(a, _mm_srli_si128(a, 4), int());
    return _mm_cvtsi128_si32(a);
}
Vc_INTRINSIC   uint add(__m128i a,   uint) { return add(a, int()); }
Vc_INTRINSIC  short add(__m128i a,  short) {
    a = add(a, _mm_srli_si128(a, 8), short());
    a = add(a, _mm_srli_si128(a, 4), short());
    a = add(a, _mm_srli_si128(a, 2), short());
    return _mm_cvtsi128_si32(a);  // & 0xffff is implicit
}
Vc_INTRINSIC ushort add(__m128i a, ushort) { return add(a, short()); }
Vc_INTRINSIC  schar add(__m128i a,  schar) {
    a = add(a, _mm_srli_si128(a, 8), schar());
    a = add(a, _mm_srli_si128(a, 4), schar());
    a = add(a, _mm_srli_si128(a, 2), schar());
    a = add(a, _mm_srli_si128(a, 1), schar());
    return _mm_cvtsi128_si32(a);  // & 0xff is implicit
}
Vc_INTRINSIC  uchar add(__m128i a,  uchar) { return add(a, schar()); }

// horizontal mul{{{1
Vc_INTRINSIC  float mul(__m128  a,  float) {
    a = _mm_mul_ps(a, _mm_movehl_ps(a, a));
    a = _mm_mul_ss(a, _mm_shuffle_ps(a, a, _MM_SHUFFLE(1, 1, 1, 1)));
    return _mm_cvtss_f32(a);
}
Vc_INTRINSIC double mul(__m128d a, double) {
    a = _mm_mul_sd(a, _mm_unpackhi_pd(a, a));
    return _mm_cvtsd_f64(a);
}
Vc_INTRINSIC    int mul(__m128i a,    int) {
    a = mul(a, _mm_srli_si128(a, 8), int());
    a = mul(a, _mm_srli_si128(a, 4), int());
    return _mm_cvtsi128_si32(a);
}
Vc_INTRINSIC   uint mul(__m128i a,   uint) { return mul(a, int()); }
Vc_INTRINSIC  short mul(__m128i a,  short) {
    a = mul(a, _mm_srli_si128(a, 8), short());
    a = mul(a, _mm_srli_si128(a, 4), short());
    a = mul(a, _mm_srli_si128(a, 2), short());
    return _mm_cvtsi128_si32(a);  // & 0xffff is implicit
}
Vc_INTRINSIC ushort mul(__m128i a, ushort) { return mul(a, short()); }
Vc_INTRINSIC  schar mul(__m128i a,  schar) {
    // convert to two short vectors, multiply them and then do horizontal reduction
    const __m128i s0 = _mm_srai_epi16(a, 1);
    const __m128i s1 = Detail::and_(a, _mm_set1_epi32(0x0f0f0f0f));
    return mul(mul(s0, s1, short()), short());
}
Vc_INTRINSIC  uchar mul(__m128i a,  uchar) { return mul(a, schar()); }

// horizontal min{{{1
Vc_INTRINSIC  float min(__m128  a,  float) {
    a = _mm_min_ps(a, _mm_movehl_ps(a, a));
    a = _mm_min_ss(a, _mm_shuffle_ps(a, a, _MM_SHUFFLE(1, 1, 1, 1)));
    return _mm_cvtss_f32(a);
}
Vc_INTRINSIC double min(__m128d a, double) {
    a = _mm_min_sd(a, _mm_unpackhi_pd(a, a));
    return _mm_cvtsd_f64(a);
}
Vc_INTRINSIC    int min(__m128i a,    int) {
    a = min(a, _mm_shuffle_epi32(a, _MM_SHUFFLE(1, 0, 3, 2)), int());
    a = min(a, _mm_shufflelo_epi16(a, _MM_SHUFFLE(1, 0, 3, 2)), int());
    return _mm_cvtsi128_si32(a);
}
Vc_INTRINSIC   uint min(__m128i a,   uint) {
    a = min(a, _mm_shuffle_epi32(a, _MM_SHUFFLE(1, 0, 3, 2)), uint());
    a = min(a, _mm_shufflelo_epi16(a, _MM_SHUFFLE(1, 0, 3, 2)), uint());
    return _mm_cvtsi128_si32(a);
}
Vc_INTRINSIC  short min(__m128i a,  short) {
    a = min(a, _mm_shuffle_epi32(a, _MM_SHUFFLE(1, 0, 3, 2)), short());
    a = min(a, _mm_shufflelo_epi16(a, _MM_SHUFFLE(1, 0, 3, 2)), short());
    a = min(a, _mm_shufflelo_epi16(a, _MM_SHUFFLE(1, 1, 1, 1)), short());
    return _mm_cvtsi128_si32(a);  // & 0xffff is implicit
}
Vc_INTRINSIC ushort min(__m128i a, ushort) {
    a = min(a, _mm_shuffle_epi32(a, _MM_SHUFFLE(1, 0, 3, 2)), ushort());
    a = min(a, _mm_shufflelo_epi16(a, _MM_SHUFFLE(1, 0, 3, 2)), ushort());
    a = min(a, _mm_shufflelo_epi16(a, _MM_SHUFFLE(1, 1, 1, 1)), ushort());
    return _mm_cvtsi128_si32(a);  // & 0xffff is implicit
}
Vc_INTRINSIC  schar min(__m128i a,  schar) {
    a = min(a, _mm_shuffle_epi32(a, _MM_SHUFFLE(1, 0, 3, 2)), schar());
    a = min(a, _mm_shufflelo_epi16(a, _MM_SHUFFLE(1, 0, 3, 2)), schar());
    a = min(a, _mm_shufflelo_epi16(a, _MM_SHUFFLE(1, 1, 1, 1)), schar());
    return std::min(schar(_mm_cvtsi128_si32(a) >> 8), schar(_mm_cvtsi128_si32(a)));
}
Vc_INTRINSIC  uchar min(__m128i a,  uchar) {
    a = min(a, _mm_shuffle_epi32(a, _MM_SHUFFLE(1, 0, 3, 2)), uchar());
    a = min(a, _mm_shufflelo_epi16(a, _MM_SHUFFLE(1, 0, 3, 2)), uchar());
    a = min(a, _mm_shufflelo_epi16(a, _MM_SHUFFLE(1, 1, 1, 1)), uchar());
    return std::min((_mm_cvtsi128_si32(a) >> 8) & 0xff, _mm_cvtsi128_si32(a) & 0xff);
}

// horizontal max{{{1
Vc_INTRINSIC  float max(__m128  a,  float) {
    a = _mm_max_ps(a, _mm_movehl_ps(a, a));
    a = _mm_max_ss(a, _mm_shuffle_ps(a, a, _MM_SHUFFLE(1, 1, 1, 1)));
    return _mm_cvtss_f32(a);
}
Vc_INTRINSIC double max(__m128d a, double) {
    a = _mm_max_sd(a, _mm_unpackhi_pd(a, a));
    return _mm_cvtsd_f64(a);
}
Vc_INTRINSIC    int max(__m128i a,    int) {
    a = max(a, _mm_shuffle_epi32(a, _MM_SHUFFLE(1, 0, 3, 2)), int());
    a = max(a, _mm_shufflelo_epi16(a, _MM_SHUFFLE(1, 0, 3, 2)), int());
    return _mm_cvtsi128_si32(a);
}
Vc_INTRINSIC   uint max(__m128i a,   uint) {
    a = max(a, _mm_shuffle_epi32(a, _MM_SHUFFLE(1, 0, 3, 2)), uint());
    a = max(a, _mm_shufflelo_epi16(a, _MM_SHUFFLE(1, 0, 3, 2)), uint());
    return _mm_cvtsi128_si32(a);
}
Vc_INTRINSIC  short max(__m128i a,  short) {
    a = max(a, _mm_shuffle_epi32(a, _MM_SHUFFLE(1, 0, 3, 2)), short());
    a = max(a, _mm_shufflelo_epi16(a, _MM_SHUFFLE(1, 0, 3, 2)), short());
    a = max(a, _mm_shufflelo_epi16(a, _MM_SHUFFLE(1, 1, 1, 1)), short());
    return _mm_cvtsi128_si32(a);  // & 0xffff is implicit
}
Vc_INTRINSIC ushort max(__m128i a, ushort) {
    a = max(a, _mm_shuffle_epi32(a, _MM_SHUFFLE(1, 0, 3, 2)), ushort());
    a = max(a, _mm_shufflelo_epi16(a, _MM_SHUFFLE(1, 0, 3, 2)), ushort());
    a = max(a, _mm_shufflelo_epi16(a, _MM_SHUFFLE(1, 1, 1, 1)), ushort());
    return _mm_cvtsi128_si32(a);  // & 0xffff is implicit
}
Vc_INTRINSIC  schar max(__m128i a,  schar) {
    a = max(a, _mm_shuffle_epi32(a, _MM_SHUFFLE(1, 0, 3, 2)), schar());
    a = max(a, _mm_shufflelo_epi16(a, _MM_SHUFFLE(1, 0, 3, 2)), schar());
    a = max(a, _mm_shufflelo_epi16(a, _MM_SHUFFLE(1, 1, 1, 1)), schar());
    return std::max(schar(_mm_cvtsi128_si32(a) >> 8), schar(_mm_cvtsi128_si32(a)));
}
Vc_INTRINSIC  uchar max(__m128i a,  uchar) {
    a = max(a, _mm_shuffle_epi32(a, _MM_SHUFFLE(1, 0, 3, 2)), uchar());
    a = max(a, _mm_shufflelo_epi16(a, _MM_SHUFFLE(1, 0, 3, 2)), uchar());
    a = max(a, _mm_shufflelo_epi16(a, _MM_SHUFFLE(1, 1, 1, 1)), uchar());
    return std::max((_mm_cvtsi128_si32(a) >> 8) & 0xff, _mm_cvtsi128_si32(a) & 0xff);
}

// sorted{{{1
template <Vc::Implementation, typename T>
Vc_CONST_L SSE::Vector<T> Vc_VDECL sorted(SSE::Vector<T> x) Vc_CONST_R;
template <typename T> Vc_INTRINSIC Vc_CONST SSE::Vector<T> sorted(SSE::Vector<T> x)
{
    static_assert(!CurrentImplementation::is(ScalarImpl),
                  "Detail::sorted can only be instantiated if a non-Scalar "
                  "implementation is selected.");
    return sorted < CurrentImplementation::is_between(SSE2Impl, SSSE3Impl)
               ? SSE2Impl
               : CurrentImplementation::is_between(SSE41Impl, SSE42Impl)
                     ? SSE41Impl
                     : CurrentImplementation::current() > (x);
}

// sanitize{{{1
template <typename V> constexpr int sanitize(int n)
{
    return (n >= int(sizeof(V)) || n <= -int(sizeof(V))) ? 0 : n;
}

// rotated{{{1
template <typename T, size_t N, typename V>
static Vc_INTRINSIC Vc_CONST enable_if<(sizeof(V) == 16), V> rotated(V v, int amount)
{
    using namespace SSE;
    switch (static_cast<unsigned int>(amount) % N) {
    case 0:
        return v;
    case 1:
        return sse_cast<V>(_mm_alignr_epi8(v, v, sanitize<V>(1 * sizeof(T))));
    case 2:
        return sse_cast<V>(_mm_alignr_epi8(v, v, sanitize<V>(2 * sizeof(T))));
    case 3:
        return sse_cast<V>(_mm_alignr_epi8(v, v, sanitize<V>(3 * sizeof(T))));
    case 4:
        return sse_cast<V>(_mm_alignr_epi8(v, v, sanitize<V>(4 * sizeof(T))));
    case 5:
        return sse_cast<V>(_mm_alignr_epi8(v, v, sanitize<V>(5 * sizeof(T))));
    case 6:
        return sse_cast<V>(_mm_alignr_epi8(v, v, sanitize<V>(6 * sizeof(T))));
    case 7:
        return sse_cast<V>(_mm_alignr_epi8(v, v, sanitize<V>(7 * sizeof(T))));
    }
    return sse_cast<V>(_mm_setzero_si128());
}

//InterleaveImpl{{{1
template<typename V, size_t Size, size_t VSize> struct InterleaveImpl;
template<typename V> struct InterleaveImpl<V, 8, 16> {
    template<typename I> static inline void interleave(typename V::EntryType *const data, const I &i,/*{{{*/
            const typename V::AsArg v0, const typename V::AsArg v1)
    {
        const __m128i tmp0 = _mm_unpacklo_epi16(v0.data(), v1.data());
        const __m128i tmp1 = _mm_unpackhi_epi16(v0.data(), v1.data());
#ifdef __x86_64__
        const long long tmp00 = _mm_cvtsi128_si64(tmp0);
        const long long tmp01 = _mm_cvtsi128_si64(_mm_unpackhi_epi64(tmp0, tmp0));
        const long long tmp10 = _mm_cvtsi128_si64(tmp1);
        const long long tmp11 = _mm_cvtsi128_si64(_mm_unpackhi_epi64(tmp1, tmp1));
        aliasing_cast<int>(data[i[0]]) = tmp00;
        aliasing_cast<int>(data[i[1]]) = tmp00 >> 32;
        aliasing_cast<int>(data[i[2]]) = tmp01;
        aliasing_cast<int>(data[i[3]]) = tmp01 >> 32;
        aliasing_cast<int>(data[i[4]]) = tmp10;
        aliasing_cast<int>(data[i[5]]) = tmp10 >> 32;
        aliasing_cast<int>(data[i[6]]) = tmp11;
        aliasing_cast<int>(data[i[7]]) = tmp11 >> 32;
#elif defined(Vc_IMPL_SSE4_1)
        using namespace SseIntrinsics;
        aliasing_cast<int>(data[i[0]]) = _mm_cvtsi128_si32(tmp0);
        aliasing_cast<int>(data[i[1]]) = extract_epi32<1>(tmp0);
        aliasing_cast<int>(data[i[2]]) = extract_epi32<2>(tmp0);
        aliasing_cast<int>(data[i[3]]) = extract_epi32<3>(tmp0);
        aliasing_cast<int>(data[i[4]]) = _mm_cvtsi128_si32(tmp1);
        aliasing_cast<int>(data[i[5]]) = extract_epi32<1>(tmp1);
        aliasing_cast<int>(data[i[6]]) = extract_epi32<2>(tmp1);
        aliasing_cast<int>(data[i[7]]) = extract_epi32<3>(tmp1);
#else
        aliasing_cast<int>(data[i[0]]) = _mm_cvtsi128_si32(tmp0);
        aliasing_cast<int>(data[i[1]]) = _mm_cvtsi128_si32(_mm_srli_si128(tmp0, 4));
        aliasing_cast<int>(data[i[2]]) = _mm_cvtsi128_si32(_mm_srli_si128(tmp0, 8));
        aliasing_cast<int>(data[i[3]]) = _mm_cvtsi128_si32(_mm_srli_si128(tmp0, 12));
        aliasing_cast<int>(data[i[4]]) = _mm_cvtsi128_si32(tmp1);
        aliasing_cast<int>(data[i[5]]) = _mm_cvtsi128_si32(_mm_srli_si128(tmp1, 4));
        aliasing_cast<int>(data[i[6]]) = _mm_cvtsi128_si32(_mm_srli_si128(tmp1, 8));
        aliasing_cast<int>(data[i[7]]) = _mm_cvtsi128_si32(_mm_srli_si128(tmp1, 12));
#endif
    }/*}}}*/
    static inline void interleave(typename V::EntryType *const data, const Common::SuccessiveEntries<2> &i,/*{{{*/
            const typename V::AsArg v0, const typename V::AsArg v1)
    {
        const __m128i tmp0 = _mm_unpacklo_epi16(v0.data(), v1.data());
        const __m128i tmp1 = _mm_unpackhi_epi16(v0.data(), v1.data());
        V(tmp0).store(&data[i[0]], Vc::Unaligned);
        V(tmp1).store(&data[i[4]], Vc::Unaligned);
    }/*}}}*/
    template<typename I> static inline void interleave(typename V::EntryType *const data, const I &i,/*{{{*/
            const typename V::AsArg v0, const typename V::AsArg v1, const typename V::AsArg v2)
    {
#if defined Vc_USE_MASKMOV_SCATTER && !defined Vc_MSVC
        // MSVC fails to compile the MMX intrinsics
        const __m64 mask = _mm_set_pi16(0, -1, -1, -1);
        const __m128i tmp0 = _mm_unpacklo_epi16(v0.data(), v2.data());
        const __m128i tmp1 = _mm_unpackhi_epi16(v0.data(), v2.data());
        const __m128i tmp2 = _mm_unpacklo_epi16(v1.data(), v1.data());
        const __m128i tmp3 = _mm_unpackhi_epi16(v1.data(), v1.data());

        const __m128i tmp4 = _mm_unpacklo_epi16(tmp0, tmp2);
        const __m128i tmp5 = _mm_unpackhi_epi16(tmp0, tmp2);
        const __m128i tmp6 = _mm_unpacklo_epi16(tmp1, tmp3);
        const __m128i tmp7 = _mm_unpackhi_epi16(tmp1, tmp3);

        _mm_maskmove_si64(_mm_movepi64_pi64(tmp4), mask, reinterpret_cast<char *>(&data[i[0]]));
        _mm_maskmove_si64(_mm_movepi64_pi64(_mm_srli_si128(tmp4, 8)), mask, reinterpret_cast<char *>(&data[i[1]]));
        _mm_maskmove_si64(_mm_movepi64_pi64(tmp5), mask, reinterpret_cast<char *>(&data[i[2]]));
        _mm_maskmove_si64(_mm_movepi64_pi64(_mm_srli_si128(tmp5, 8)), mask, reinterpret_cast<char *>(&data[i[3]]));
        _mm_maskmove_si64(_mm_movepi64_pi64(tmp6), mask, reinterpret_cast<char *>(&data[i[4]]));
        _mm_maskmove_si64(_mm_movepi64_pi64(_mm_srli_si128(tmp6, 8)), mask, reinterpret_cast<char *>(&data[i[5]]));
        _mm_maskmove_si64(_mm_movepi64_pi64(tmp7), mask, reinterpret_cast<char *>(&data[i[6]]));
        _mm_maskmove_si64(_mm_movepi64_pi64(_mm_srli_si128(tmp7, 8)), mask, reinterpret_cast<char *>(&data[i[7]]));
        _mm_empty();
#else
        interleave(data, i, v0, v1);
        v2.scatter(data + 2, i);
#endif
    }/*}}}*/
    template<typename I> static inline void interleave(typename V::EntryType *const data, const I &i,/*{{{*/
            const typename V::AsArg v0, const typename V::AsArg v1,
            const typename V::AsArg v2, const typename V::AsArg v3)
    {
        const __m128i tmp0 = _mm_unpacklo_epi16(v0.data(), v2.data());
        const __m128i tmp1 = _mm_unpackhi_epi16(v0.data(), v2.data());
        const __m128i tmp2 = _mm_unpacklo_epi16(v1.data(), v3.data());
        const __m128i tmp3 = _mm_unpackhi_epi16(v1.data(), v3.data());

        const __m128i tmp4 = _mm_unpacklo_epi16(tmp0, tmp2);
        const __m128i tmp5 = _mm_unpackhi_epi16(tmp0, tmp2);
        const __m128i tmp6 = _mm_unpacklo_epi16(tmp1, tmp3);
        const __m128i tmp7 = _mm_unpackhi_epi16(tmp1, tmp3);

        _mm_storel_epi64(reinterpret_cast<__m128i *>(&data[i[0]]), tmp4);
        _mm_storel_epi64(reinterpret_cast<__m128i *>(&data[i[2]]), tmp5);
        _mm_storel_epi64(reinterpret_cast<__m128i *>(&data[i[4]]), tmp6);
        _mm_storel_epi64(reinterpret_cast<__m128i *>(&data[i[6]]), tmp7);
        _mm_storeh_pi(reinterpret_cast<__m64 *>(&data[i[1]]), _mm_castsi128_ps(tmp4));
        _mm_storeh_pi(reinterpret_cast<__m64 *>(&data[i[3]]), _mm_castsi128_ps(tmp5));
        _mm_storeh_pi(reinterpret_cast<__m64 *>(&data[i[5]]), _mm_castsi128_ps(tmp6));
        _mm_storeh_pi(reinterpret_cast<__m64 *>(&data[i[7]]), _mm_castsi128_ps(tmp7));
    }/*}}}*/
    static inline void interleave(typename V::EntryType *const data, const Common::SuccessiveEntries<4> &i,/*{{{*/
            const typename V::AsArg v0, const typename V::AsArg v1,
            const typename V::AsArg v2, const typename V::AsArg v3)
    {
        const __m128i tmp0 = _mm_unpacklo_epi16(v0.data(), v2.data());
        const __m128i tmp1 = _mm_unpackhi_epi16(v0.data(), v2.data());
        const __m128i tmp2 = _mm_unpacklo_epi16(v1.data(), v3.data());
        const __m128i tmp3 = _mm_unpackhi_epi16(v1.data(), v3.data());

        const __m128i tmp4 = _mm_unpacklo_epi16(tmp0, tmp2);
        const __m128i tmp5 = _mm_unpackhi_epi16(tmp0, tmp2);
        const __m128i tmp6 = _mm_unpacklo_epi16(tmp1, tmp3);
        const __m128i tmp7 = _mm_unpackhi_epi16(tmp1, tmp3);

        V(tmp4).store(&data[i[0]], ::Vc::Unaligned);
        V(tmp5).store(&data[i[2]], ::Vc::Unaligned);
        V(tmp6).store(&data[i[4]], ::Vc::Unaligned);
        V(tmp7).store(&data[i[6]], ::Vc::Unaligned);
    }/*}}}*/
    template <typename I>  // interleave 5 args{{{2
    static inline void interleave(typename V::EntryType *const data, const I &i,
                                  const typename V::AsArg v0, const typename V::AsArg v1,
                                  const typename V::AsArg v2, const typename V::AsArg v3,
                                  const typename V::AsArg v4)
    {
        interleave(data, i, v0, v1, v2, v3);
        v4.scatter(data + 4, i);
    }
    template <typename I>  // interleave 6 args{{{2
    static inline void interleave(typename V::EntryType *const data, const I &i,
                                  const typename V::AsArg v0, const typename V::AsArg v1,
                                  const typename V::AsArg v2, const typename V::AsArg v3,
                                  const typename V::AsArg v4, const typename V::AsArg v5)
    {
        interleave(data, i, v0, v1, v2, v3);
        interleave(data + 4, i, v4, v5);
    }
    template <typename I>  // interleave 7 args{{{2
    static inline void interleave(typename V::EntryType *const data, const I &i,
                                  const typename V::AsArg v0, const typename V::AsArg v1,
                                  const typename V::AsArg v2, const typename V::AsArg v3,
                                  const typename V::AsArg v4, const typename V::AsArg v5,
                                  const typename V::AsArg v6)
    {
        interleave(data, i, v0, v1, v2, v3);
        interleave(data + 4, i, v4, v5, v6);
    }
    template <typename I>  // interleave 8 args{{{2
    static inline void interleave(typename V::EntryType *const data, const I &i,
                                  const typename V::AsArg v0, const typename V::AsArg v1,
                                  const typename V::AsArg v2, const typename V::AsArg v3,
                                  const typename V::AsArg v4, const typename V::AsArg v5,
                                  const typename V::AsArg v6, const typename V::AsArg v7)
    {
        interleave(data, i, v0, v1, v2, v3);
        interleave(data + 4, i, v4, v5, v6, v7);
    }
    //}}}2
    template<typename I> static inline void deinterleave(typename V::EntryType const *const data, /*{{{*/
            const I &i, V &v0, V &v1)
    {
        const __m128i a = _mm_cvtsi32_si128(*aliasing_cast<int>(&data[i[0]]));
        const __m128i b = _mm_cvtsi32_si128(*aliasing_cast<int>(&data[i[1]]));
        const __m128i c = _mm_cvtsi32_si128(*aliasing_cast<int>(&data[i[2]]));
        const __m128i d = _mm_cvtsi32_si128(*aliasing_cast<int>(&data[i[3]]));
        const __m128i e = _mm_cvtsi32_si128(*aliasing_cast<int>(&data[i[4]]));
        const __m128i f = _mm_cvtsi32_si128(*aliasing_cast<int>(&data[i[5]]));
        const __m128i g = _mm_cvtsi32_si128(*aliasing_cast<int>(&data[i[6]]));
        const __m128i h = _mm_cvtsi32_si128(*aliasing_cast<int>(&data[i[7]]));

        const __m128i tmp2  = _mm_unpacklo_epi16(a, e); // a0 a4 b0 b4 c0 c4 d0 d4
        const __m128i tmp3  = _mm_unpacklo_epi16(c, g); // a2 a6 b2 b6 c2 c6 d2 d6
        const __m128i tmp4  = _mm_unpacklo_epi16(b, f); // a1 a5 b1 b5 c1 c5 d1 d5
        const __m128i tmp5  = _mm_unpacklo_epi16(d, h); // a3 a7 b3 b7 c3 c7 d3 d7

        const __m128i tmp0  = _mm_unpacklo_epi16(tmp2, tmp3); // a0 a2 a4 a6 b0 b2 b4 b6
        const __m128i tmp1  = _mm_unpacklo_epi16(tmp4, tmp5); // a1 a3 a5 a7 b1 b3 b5 b7

        v0.data() = _mm_unpacklo_epi16(tmp0, tmp1);
        v1.data() = _mm_unpackhi_epi16(tmp0, tmp1);
    }/*}}}*/
    template<typename I> static inline void deinterleave(typename V::EntryType const *const data,/*{{{*/
            const I &i, V &v0, V &v1, V &v2)
    {
        const __m128i a = _mm_loadl_epi64(reinterpret_cast<const __m128i *>(&data[i[0]]));
        const __m128i b = _mm_loadl_epi64(reinterpret_cast<const __m128i *>(&data[i[1]]));
        const __m128i c = _mm_loadl_epi64(reinterpret_cast<const __m128i *>(&data[i[2]]));
        const __m128i d = _mm_loadl_epi64(reinterpret_cast<const __m128i *>(&data[i[3]]));
        const __m128i e = _mm_loadl_epi64(reinterpret_cast<const __m128i *>(&data[i[4]]));
        const __m128i f = _mm_loadl_epi64(reinterpret_cast<const __m128i *>(&data[i[5]]));
        const __m128i g = _mm_loadl_epi64(reinterpret_cast<const __m128i *>(&data[i[6]]));
        const __m128i h = _mm_loadl_epi64(reinterpret_cast<const __m128i *>(&data[i[7]]));

        const __m128i tmp2  = _mm_unpacklo_epi16(a, e); // a0 a4 b0 b4 c0 c4 d0 d4
        const __m128i tmp4  = _mm_unpacklo_epi16(b, f); // a1 a5 b1 b5 c1 c5 d1 d5
        const __m128i tmp3  = _mm_unpacklo_epi16(c, g); // a2 a6 b2 b6 c2 c6 d2 d6
        const __m128i tmp5  = _mm_unpacklo_epi16(d, h); // a3 a7 b3 b7 c3 c7 d3 d7

        const __m128i tmp0  = _mm_unpacklo_epi16(tmp2, tmp3); // a0 a2 a4 a6 b0 b2 b4 b6
        const __m128i tmp1  = _mm_unpacklo_epi16(tmp4, tmp5); // a1 a3 a5 a7 b1 b3 b5 b7
        const __m128i tmp6  = _mm_unpackhi_epi16(tmp2, tmp3); // c0 c2 c4 c6 d0 d2 d4 d6
        const __m128i tmp7  = _mm_unpackhi_epi16(tmp4, tmp5); // c1 c3 c5 c7 d1 d3 d5 d7

        v0.data() = _mm_unpacklo_epi16(tmp0, tmp1);
        v1.data() = _mm_unpackhi_epi16(tmp0, tmp1);
        v2.data() = _mm_unpacklo_epi16(tmp6, tmp7);
    }/*}}}*/
    template<typename I> static inline void deinterleave(typename V::EntryType const *const data,/*{{{*/
            const I &i, V &v0, V &v1, V &v2, V &v3)
    {
        const __m128i a = _mm_loadl_epi64(reinterpret_cast<const __m128i *>(&data[i[0]]));
        const __m128i b = _mm_loadl_epi64(reinterpret_cast<const __m128i *>(&data[i[1]]));
        const __m128i c = _mm_loadl_epi64(reinterpret_cast<const __m128i *>(&data[i[2]]));
        const __m128i d = _mm_loadl_epi64(reinterpret_cast<const __m128i *>(&data[i[3]]));
        const __m128i e = _mm_loadl_epi64(reinterpret_cast<const __m128i *>(&data[i[4]]));
        const __m128i f = _mm_loadl_epi64(reinterpret_cast<const __m128i *>(&data[i[5]]));
        const __m128i g = _mm_loadl_epi64(reinterpret_cast<const __m128i *>(&data[i[6]]));
        const __m128i h = _mm_loadl_epi64(reinterpret_cast<const __m128i *>(&data[i[7]]));

        const __m128i tmp2  = _mm_unpacklo_epi16(a, e); // a0 a4 b0 b4 c0 c4 d0 d4
        const __m128i tmp4  = _mm_unpacklo_epi16(b, f); // a1 a5 b1 b5 c1 c5 d1 d5
        const __m128i tmp3  = _mm_unpacklo_epi16(c, g); // a2 a6 b2 b6 c2 c6 d2 d6
        const __m128i tmp5  = _mm_unpacklo_epi16(d, h); // a3 a7 b3 b7 c3 c7 d3 d7

        const __m128i tmp0  = _mm_unpacklo_epi16(tmp2, tmp3); // a0 a2 a4 a6 b0 b2 b4 b6
        const __m128i tmp1  = _mm_unpacklo_epi16(tmp4, tmp5); // a1 a3 a5 a7 b1 b3 b5 b7
        const __m128i tmp6  = _mm_unpackhi_epi16(tmp2, tmp3); // c0 c2 c4 c6 d0 d2 d4 d6
        const __m128i tmp7  = _mm_unpackhi_epi16(tmp4, tmp5); // c1 c3 c5 c7 d1 d3 d5 d7

        v0.data() = _mm_unpacklo_epi16(tmp0, tmp1);
        v1.data() = _mm_unpackhi_epi16(tmp0, tmp1);
        v2.data() = _mm_unpacklo_epi16(tmp6, tmp7);
        v3.data() = _mm_unpackhi_epi16(tmp6, tmp7);
    }/*}}}*/
    template<typename I> static inline void deinterleave(typename V::EntryType const *const data,/*{{{*/
            const I &i, V &v0, V &v1, V &v2, V &v3, V &v4)
    {
        const __m128i a = _mm_loadu_si128(reinterpret_cast<const __m128i *>(&data[i[0]]));
        const __m128i b = _mm_loadu_si128(reinterpret_cast<const __m128i *>(&data[i[1]]));
        const __m128i c = _mm_loadu_si128(reinterpret_cast<const __m128i *>(&data[i[2]]));
        const __m128i d = _mm_loadu_si128(reinterpret_cast<const __m128i *>(&data[i[3]]));
        const __m128i e = _mm_loadu_si128(reinterpret_cast<const __m128i *>(&data[i[4]]));
        const __m128i f = _mm_loadu_si128(reinterpret_cast<const __m128i *>(&data[i[5]]));
        const __m128i g = _mm_loadu_si128(reinterpret_cast<const __m128i *>(&data[i[6]]));
        const __m128i h = _mm_loadu_si128(reinterpret_cast<const __m128i *>(&data[i[7]]));

        const __m128i tmp2  = _mm_unpacklo_epi16(a, e); // a0 a4 b0 b4 c0 c4 d0 d4
        const __m128i tmp4  = _mm_unpacklo_epi16(b, f); // a1 a5 b1 b5 c1 c5 d1 d5
        const __m128i tmp3  = _mm_unpacklo_epi16(c, g); // a2 a6 b2 b6 c2 c6 d2 d6
        const __m128i tmp5  = _mm_unpacklo_epi16(d, h); // a3 a7 b3 b7 c3 c7 d3 d7
        const __m128i tmp10 = _mm_unpackhi_epi16(a, e); // e0 e4 f0 f4 g0 g4 h0 h4
        const __m128i tmp11 = _mm_unpackhi_epi16(c, g); // e1 e5 f1 f5 g1 g5 h1 h5
        const __m128i tmp12 = _mm_unpackhi_epi16(b, f); // e2 e6 f2 f6 g2 g6 h2 h6
        const __m128i tmp13 = _mm_unpackhi_epi16(d, h); // e3 e7 f3 f7 g3 g7 h3 h7

        const __m128i tmp0  = _mm_unpacklo_epi16(tmp2, tmp3); // a0 a2 a4 a6 b0 b2 b4 b6
        const __m128i tmp1  = _mm_unpacklo_epi16(tmp4, tmp5); // a1 a3 a5 a7 b1 b3 b5 b7
        const __m128i tmp6  = _mm_unpackhi_epi16(tmp2, tmp3); // c0 c2 c4 c6 d0 d2 d4 d6
        const __m128i tmp7  = _mm_unpackhi_epi16(tmp4, tmp5); // c1 c3 c5 c7 d1 d3 d5 d7
        const __m128i tmp8  = _mm_unpacklo_epi16(tmp10, tmp11); // e0 e2 e4 e6 f0 f2 f4 f6
        const __m128i tmp9  = _mm_unpacklo_epi16(tmp12, tmp13); // e1 e3 e5 e7 f1 f3 f5 f7

        v0.data() = _mm_unpacklo_epi16(tmp0, tmp1);
        v1.data() = _mm_unpackhi_epi16(tmp0, tmp1);
        v2.data() = _mm_unpacklo_epi16(tmp6, tmp7);
        v3.data() = _mm_unpackhi_epi16(tmp6, tmp7);
        v4.data() = _mm_unpacklo_epi16(tmp8, tmp9);
    }/*}}}*/
    template<typename I> static inline void deinterleave(typename V::EntryType const *const data,/*{{{*/
            const I &i, V &v0, V &v1, V &v2, V &v3, V &v4, V &v5)
    {
        const __m128i a = _mm_loadu_si128(reinterpret_cast<const __m128i *>(&data[i[0]]));
        const __m128i b = _mm_loadu_si128(reinterpret_cast<const __m128i *>(&data[i[1]]));
        const __m128i c = _mm_loadu_si128(reinterpret_cast<const __m128i *>(&data[i[2]]));
        const __m128i d = _mm_loadu_si128(reinterpret_cast<const __m128i *>(&data[i[3]]));
        const __m128i e = _mm_loadu_si128(reinterpret_cast<const __m128i *>(&data[i[4]]));
        const __m128i f = _mm_loadu_si128(reinterpret_cast<const __m128i *>(&data[i[5]]));
        const __m128i g = _mm_loadu_si128(reinterpret_cast<const __m128i *>(&data[i[6]]));
        const __m128i h = _mm_loadu_si128(reinterpret_cast<const __m128i *>(&data[i[7]]));

        const __m128i tmp2  = _mm_unpacklo_epi16(a, e); // a0 a4 b0 b4 c0 c4 d0 d4
        const __m128i tmp4  = _mm_unpacklo_epi16(b, f); // a1 a5 b1 b5 c1 c5 d1 d5
        const __m128i tmp3  = _mm_unpacklo_epi16(c, g); // a2 a6 b2 b6 c2 c6 d2 d6
        const __m128i tmp5  = _mm_unpacklo_epi16(d, h); // a3 a7 b3 b7 c3 c7 d3 d7
        const __m128i tmp10 = _mm_unpackhi_epi16(a, e); // e0 e4 f0 f4 g0 g4 h0 h4
        const __m128i tmp11 = _mm_unpackhi_epi16(c, g); // e1 e5 f1 f5 g1 g5 h1 h5
        const __m128i tmp12 = _mm_unpackhi_epi16(b, f); // e2 e6 f2 f6 g2 g6 h2 h6
        const __m128i tmp13 = _mm_unpackhi_epi16(d, h); // e3 e7 f3 f7 g3 g7 h3 h7

        const __m128i tmp0  = _mm_unpacklo_epi16(tmp2, tmp3); // a0 a2 a4 a6 b0 b2 b4 b6
        const __m128i tmp1  = _mm_unpacklo_epi16(tmp4, tmp5); // a1 a3 a5 a7 b1 b3 b5 b7
        const __m128i tmp6  = _mm_unpackhi_epi16(tmp2, tmp3); // c0 c2 c4 c6 d0 d2 d4 d6
        const __m128i tmp7  = _mm_unpackhi_epi16(tmp4, tmp5); // c1 c3 c5 c7 d1 d3 d5 d7
        const __m128i tmp8  = _mm_unpacklo_epi16(tmp10, tmp11); // e0 e2 e4 e6 f0 f2 f4 f6
        const __m128i tmp9  = _mm_unpacklo_epi16(tmp12, tmp13); // e1 e3 e5 e7 f1 f3 f5 f7

        v0.data() = _mm_unpacklo_epi16(tmp0, tmp1);
        v1.data() = _mm_unpackhi_epi16(tmp0, tmp1);
        v2.data() = _mm_unpacklo_epi16(tmp6, tmp7);
        v3.data() = _mm_unpackhi_epi16(tmp6, tmp7);
        v4.data() = _mm_unpacklo_epi16(tmp8, tmp9);
        v5.data() = _mm_unpackhi_epi16(tmp8, tmp9);
    }/*}}}*/
    template<typename I> static inline void deinterleave(typename V::EntryType const *const data,/*{{{*/
            const I &i, V &v0, V &v1, V &v2, V &v3, V &v4, V &v5, V &v6)
    {
        const __m128i a = _mm_loadu_si128(reinterpret_cast<const __m128i *>(&data[i[0]]));
        const __m128i b = _mm_loadu_si128(reinterpret_cast<const __m128i *>(&data[i[1]]));
        const __m128i c = _mm_loadu_si128(reinterpret_cast<const __m128i *>(&data[i[2]]));
        const __m128i d = _mm_loadu_si128(reinterpret_cast<const __m128i *>(&data[i[3]]));
        const __m128i e = _mm_loadu_si128(reinterpret_cast<const __m128i *>(&data[i[4]]));
        const __m128i f = _mm_loadu_si128(reinterpret_cast<const __m128i *>(&data[i[5]]));
        const __m128i g = _mm_loadu_si128(reinterpret_cast<const __m128i *>(&data[i[6]]));
        const __m128i h = _mm_loadu_si128(reinterpret_cast<const __m128i *>(&data[i[7]]));

        const __m128i tmp2  = _mm_unpacklo_epi16(a, e); // a0 a4 b0 b4 c0 c4 d0 d4
        const __m128i tmp4  = _mm_unpacklo_epi16(b, f); // a1 a5 b1 b5 c1 c5 d1 d5
        const __m128i tmp3  = _mm_unpacklo_epi16(c, g); // a2 a6 b2 b6 c2 c6 d2 d6
        const __m128i tmp5  = _mm_unpacklo_epi16(d, h); // a3 a7 b3 b7 c3 c7 d3 d7
        const __m128i tmp10 = _mm_unpackhi_epi16(a, e); // e0 e4 f0 f4 g0 g4 h0 h4
        const __m128i tmp11 = _mm_unpackhi_epi16(c, g); // e1 e5 f1 f5 g1 g5 h1 h5
        const __m128i tmp12 = _mm_unpackhi_epi16(b, f); // e2 e6 f2 f6 g2 g6 h2 h6
        const __m128i tmp13 = _mm_unpackhi_epi16(d, h); // e3 e7 f3 f7 g3 g7 h3 h7

        const __m128i tmp0  = _mm_unpacklo_epi16(tmp2, tmp3); // a0 a2 a4 a6 b0 b2 b4 b6
        const __m128i tmp1  = _mm_unpacklo_epi16(tmp4, tmp5); // a1 a3 a5 a7 b1 b3 b5 b7
        const __m128i tmp6  = _mm_unpackhi_epi16(tmp2, tmp3); // c0 c2 c4 c6 d0 d2 d4 d6
        const __m128i tmp7  = _mm_unpackhi_epi16(tmp4, tmp5); // c1 c3 c5 c7 d1 d3 d5 d7
        const __m128i tmp8  = _mm_unpacklo_epi16(tmp10, tmp11); // e0 e2 e4 e6 f0 f2 f4 f6
        const __m128i tmp9  = _mm_unpacklo_epi16(tmp12, tmp13); // e1 e3 e5 e7 f1 f3 f5 f7
        const __m128i tmp14 = _mm_unpackhi_epi16(tmp10, tmp11); // g0 g2 g4 g6 h0 h2 h4 h6
        const __m128i tmp15 = _mm_unpackhi_epi16(tmp12, tmp13); // g1 g3 g5 g7 h1 h3 h5 h7

        v0.data() = _mm_unpacklo_epi16(tmp0, tmp1);
        v1.data() = _mm_unpackhi_epi16(tmp0, tmp1);
        v2.data() = _mm_unpacklo_epi16(tmp6, tmp7);
        v3.data() = _mm_unpackhi_epi16(tmp6, tmp7);
        v4.data() = _mm_unpacklo_epi16(tmp8, tmp9);
        v5.data() = _mm_unpackhi_epi16(tmp8, tmp9);
        v6.data() = _mm_unpacklo_epi16(tmp14, tmp15);
    }/*}}}*/
    template<typename I> static inline void deinterleave(typename V::EntryType const *const data,/*{{{*/
            const I &i, V &v0, V &v1, V &v2, V &v3, V &v4, V &v5, V &v6, V &v7)
    {
        const __m128i a = _mm_loadu_si128(reinterpret_cast<const __m128i *>(&data[i[0]]));
        const __m128i b = _mm_loadu_si128(reinterpret_cast<const __m128i *>(&data[i[1]]));
        const __m128i c = _mm_loadu_si128(reinterpret_cast<const __m128i *>(&data[i[2]]));
        const __m128i d = _mm_loadu_si128(reinterpret_cast<const __m128i *>(&data[i[3]]));
        const __m128i e = _mm_loadu_si128(reinterpret_cast<const __m128i *>(&data[i[4]]));
        const __m128i f = _mm_loadu_si128(reinterpret_cast<const __m128i *>(&data[i[5]]));
        const __m128i g = _mm_loadu_si128(reinterpret_cast<const __m128i *>(&data[i[6]]));
        const __m128i h = _mm_loadu_si128(reinterpret_cast<const __m128i *>(&data[i[7]]));

        const __m128i tmp2  = _mm_unpacklo_epi16(a, e); // a0 a4 b0 b4 c0 c4 d0 d4
        const __m128i tmp4  = _mm_unpacklo_epi16(b, f); // a1 a5 b1 b5 c1 c5 d1 d5
        const __m128i tmp3  = _mm_unpacklo_epi16(c, g); // a2 a6 b2 b6 c2 c6 d2 d6
        const __m128i tmp5  = _mm_unpacklo_epi16(d, h); // a3 a7 b3 b7 c3 c7 d3 d7
        const __m128i tmp10 = _mm_unpackhi_epi16(a, e); // e0 e4 f0 f4 g0 g4 h0 h4
        const __m128i tmp11 = _mm_unpackhi_epi16(c, g); // e1 e5 f1 f5 g1 g5 h1 h5
        const __m128i tmp12 = _mm_unpackhi_epi16(b, f); // e2 e6 f2 f6 g2 g6 h2 h6
        const __m128i tmp13 = _mm_unpackhi_epi16(d, h); // e3 e7 f3 f7 g3 g7 h3 h7

        const __m128i tmp0  = _mm_unpacklo_epi16(tmp2, tmp3); // a0 a2 a4 a6 b0 b2 b4 b6
        const __m128i tmp1  = _mm_unpacklo_epi16(tmp4, tmp5); // a1 a3 a5 a7 b1 b3 b5 b7
        const __m128i tmp6  = _mm_unpackhi_epi16(tmp2, tmp3); // c0 c2 c4 c6 d0 d2 d4 d6
        const __m128i tmp7  = _mm_unpackhi_epi16(tmp4, tmp5); // c1 c3 c5 c7 d1 d3 d5 d7
        const __m128i tmp8  = _mm_unpacklo_epi16(tmp10, tmp11); // e0 e2 e4 e6 f0 f2 f4 f6
        const __m128i tmp9  = _mm_unpacklo_epi16(tmp12, tmp13); // e1 e3 e5 e7 f1 f3 f5 f7
        const __m128i tmp14 = _mm_unpackhi_epi16(tmp10, tmp11); // g0 g2 g4 g6 h0 h2 h4 h6
        const __m128i tmp15 = _mm_unpackhi_epi16(tmp12, tmp13); // g1 g3 g5 g7 h1 h3 h5 h7

        v0.data() = _mm_unpacklo_epi16(tmp0, tmp1);
        v1.data() = _mm_unpackhi_epi16(tmp0, tmp1);
        v2.data() = _mm_unpacklo_epi16(tmp6, tmp7);
        v3.data() = _mm_unpackhi_epi16(tmp6, tmp7);
        v4.data() = _mm_unpacklo_epi16(tmp8, tmp9);
        v5.data() = _mm_unpackhi_epi16(tmp8, tmp9);
        v6.data() = _mm_unpacklo_epi16(tmp14, tmp15);
        v7.data() = _mm_unpackhi_epi16(tmp14, tmp15);
    }/*}}}*/
};
template<typename V> struct InterleaveImpl<V, 4, 16> {
    static inline void interleave(typename V::EntryType *const data, const Common::SuccessiveEntries<2> &i,/*{{{*/
            const typename V::AsArg v0, const typename V::AsArg v1)
    {
        const __m128 tmp0 = _mm_unpacklo_ps(SSE::sse_cast<__m128>(v0.data()),SSE::sse_cast<__m128>(v1.data()));
        const __m128 tmp1 = _mm_unpackhi_ps(SSE::sse_cast<__m128>(v0.data()),SSE::sse_cast<__m128>(v1.data()));
        _mm_storeu_ps(aliasing_cast<float>(&data[i[0]]), tmp0);
        _mm_storeu_ps(aliasing_cast<float>(&data[i[2]]), tmp1);
    }/*}}}*/
    template <typename I>  // interleave 2 args {{{2
    static inline void interleave(typename V::EntryType *const data, const I &i,
                                  const typename V::AsArg v0, const typename V::AsArg v1)
    {
        const __m128 tmp0 = _mm_unpacklo_ps(SSE::sse_cast<__m128>(v0.data()),SSE::sse_cast<__m128>(v1.data()));
        const __m128 tmp1 = _mm_unpackhi_ps(SSE::sse_cast<__m128>(v0.data()),SSE::sse_cast<__m128>(v1.data()));
        _mm_storel_pi(reinterpret_cast<__m64 *>(&data[i[0]]), tmp0);
        _mm_storeh_pi(reinterpret_cast<__m64 *>(&data[i[1]]), tmp0);
        _mm_storel_pi(reinterpret_cast<__m64 *>(&data[i[2]]), tmp1);
        _mm_storeh_pi(reinterpret_cast<__m64 *>(&data[i[3]]), tmp1);
    }
    template <typename I>  // interleave 3 args {{{2
    static inline void interleave(typename V::EntryType *const data, const I &i,
                                  const typename V::AsArg v0, const typename V::AsArg v1,
                                  const typename V::AsArg v2)
    {
#ifdef Vc_USE_MASKMOV_SCATTER
        const __m128 tmp0 = _mm_unpacklo_ps(SSE::sse_cast<__m128>(v0.data()), SSE::sse_cast<__m128>(v1.data()));
        const __m128 tmp1 = _mm_unpackhi_ps(SSE::sse_cast<__m128>(v0.data()), SSE::sse_cast<__m128>(v1.data()));
        const __m128 tmp2 = _mm_unpacklo_ps(SSE::sse_cast<__m128>(v2.data()), SSE::sse_cast<__m128>(v2.data()));
        const __m128 tmp3 = _mm_unpackhi_ps(SSE::sse_cast<__m128>(v2.data()), SSE::sse_cast<__m128>(v2.data()));
        const __m128i mask = _mm_set_epi32(0, -1, -1, -1);
        _mm_maskmoveu_si128(_mm_castps_si128(_mm_movelh_ps(tmp0, tmp2)), mask, reinterpret_cast<char *>(&data[i[0]]));
        _mm_maskmoveu_si128(_mm_castps_si128(_mm_movehl_ps(tmp2, tmp0)), mask, reinterpret_cast<char *>(&data[i[1]]));
        _mm_maskmoveu_si128(_mm_castps_si128(_mm_movelh_ps(tmp1, tmp3)), mask, reinterpret_cast<char *>(&data[i[2]]));
        _mm_maskmoveu_si128(_mm_castps_si128(_mm_movehl_ps(tmp3, tmp1)), mask, reinterpret_cast<char *>(&data[i[3]]));
#else
        const __m128 tmp0 = _mm_unpacklo_ps(SSE::sse_cast<__m128>(v0.data()),SSE::sse_cast<__m128>(v1.data()));
        const __m128 tmp1 = _mm_unpackhi_ps(SSE::sse_cast<__m128>(v0.data()),SSE::sse_cast<__m128>(v1.data()));
        _mm_storel_pi(reinterpret_cast<__m64 *>(&data[i[0]]), tmp0);
        _mm_storeh_pi(reinterpret_cast<__m64 *>(&data[i[1]]), tmp0);
        _mm_storel_pi(reinterpret_cast<__m64 *>(&data[i[2]]), tmp1);
        _mm_storeh_pi(reinterpret_cast<__m64 *>(&data[i[3]]), tmp1);
        v2.scatter(data + 2, i);
#endif
    }
    template <typename I>  // interleave 4 args {{{2
    static inline void interleave(typename V::EntryType *const data, const I &i,
                                  const typename V::AsArg v0, const typename V::AsArg v1,
                                  const typename V::AsArg v2, const typename V::AsArg v3)
    {
        const __m128 tmp0 = _mm_unpacklo_ps(SSE::sse_cast<__m128>(v0.data()),SSE::sse_cast<__m128>(v1.data()));
        const __m128 tmp1 = _mm_unpackhi_ps(SSE::sse_cast<__m128>(v0.data()),SSE::sse_cast<__m128>(v1.data()));
        const __m128 tmp2 = _mm_unpacklo_ps(SSE::sse_cast<__m128>(v2.data()),SSE::sse_cast<__m128>(v3.data()));
        const __m128 tmp3 = _mm_unpackhi_ps(SSE::sse_cast<__m128>(v2.data()),SSE::sse_cast<__m128>(v3.data()));
        _mm_storeu_ps(aliasing_cast<float>(&data[i[0]]), _mm_movelh_ps(tmp0, tmp2));
        _mm_storeu_ps(aliasing_cast<float>(&data[i[1]]), _mm_movehl_ps(tmp2, tmp0));
        _mm_storeu_ps(aliasing_cast<float>(&data[i[2]]), _mm_movelh_ps(tmp1, tmp3));
        _mm_storeu_ps(aliasing_cast<float>(&data[i[3]]), _mm_movehl_ps(tmp3, tmp1));
    }
    template <typename I>  // interleave 5 args {{{2
    static inline void interleave(typename V::EntryType *const data, const I &i,
                                  const typename V::AsArg v0, const typename V::AsArg v1,
                                  const typename V::AsArg v2, const typename V::AsArg v3,
                                  const typename V::AsArg v4)
    {
        interleave(data, i, v0, v1, v2, v3);
        v4.scatter(data + 4, i);
    }
    template <typename I>  // interleave 6 args {{{2
    static inline void interleave(typename V::EntryType *const data, const I &i,
                                  const typename V::AsArg v0, const typename V::AsArg v1,
                                  const typename V::AsArg v2, const typename V::AsArg v3,
                                  const typename V::AsArg v4, const typename V::AsArg v5)
    {
        interleave(data, i, v0, v1, v2, v3);
        interleave(data + 4, i, v4, v5);
    }
    template <typename I>  // interleave 7 args {{{2
    static inline void interleave(typename V::EntryType *const data, const I &i,
                                  const typename V::AsArg v0, const typename V::AsArg v1,
                                  const typename V::AsArg v2, const typename V::AsArg v3,
                                  const typename V::AsArg v4, const typename V::AsArg v5,
                                  const typename V::AsArg v6)
    {
        interleave(data, i, v0, v1, v2, v3);
        interleave(data + 4, i, v4, v5, v6);
    }
    template <typename I>  // interleave 8 args {{{2
    static inline void interleave(typename V::EntryType *const data, const I &i,
                                  const typename V::AsArg v0, const typename V::AsArg v1,
                                  const typename V::AsArg v2, const typename V::AsArg v3,
                                  const typename V::AsArg v4, const typename V::AsArg v5,
                                  const typename V::AsArg v6, const typename V::AsArg v7)
    {
        interleave(data, i, v0, v1, v2, v3);
        interleave(data + 4, i, v4, v5, v6, v7);
    }
    //}}}2
    template<typename I> static inline void deinterleave(typename V::EntryType const *const data,/*{{{*/
            const I &i, V &v0, V &v1)
    {
        const __m128 a = _mm_castpd_ps(_mm_load_sd(aliasing_cast<double>(&data[i[0]])));
        const __m128 b = _mm_castpd_ps(_mm_load_sd(aliasing_cast<double>(&data[i[1]])));
        const __m128 c = _mm_castpd_ps(_mm_load_sd(aliasing_cast<double>(&data[i[2]])));
        const __m128 d = _mm_castpd_ps(_mm_load_sd(aliasing_cast<double>(&data[i[3]])));

        const __m128 tmp0 = _mm_unpacklo_ps(a, b); // [a0 a1 b0 b1]
        const __m128 tmp1 = _mm_unpacklo_ps(c, d); // [a2 a3 b2 b3]

        v0.data() = SSE::sse_cast<typename V::VectorType>(_mm_movelh_ps(tmp0, tmp1));
        v1.data() = SSE::sse_cast<typename V::VectorType>(_mm_movehl_ps(tmp1, tmp0));
    }/*}}}*/
    template<typename I> static inline void deinterleave(typename V::EntryType const *const data,/*{{{*/
            const I &i, V &v0, V &v1, V &v2)
    {
        const __m128 a = _mm_loadu_ps(aliasing_cast<float>(&data[i[0]]));
        const __m128 b = _mm_loadu_ps(aliasing_cast<float>(&data[i[1]]));
        const __m128 c = _mm_loadu_ps(aliasing_cast<float>(&data[i[2]]));
        const __m128 d = _mm_loadu_ps(aliasing_cast<float>(&data[i[3]]));

        const __m128 tmp0 = _mm_unpacklo_ps(a, b); // [a0 a1 b0 b1]
        const __m128 tmp1 = _mm_unpacklo_ps(c, d); // [a2 a3 b2 b3]
        const __m128 tmp2 = _mm_unpackhi_ps(a, b); // [c0 c1 XX XX]
        const __m128 tmp3 = _mm_unpackhi_ps(c, d); // [c2 c3 XX XX]

        v0.data() = SSE::sse_cast<typename V::VectorType>(_mm_movelh_ps(tmp0, tmp1));
        v1.data() = SSE::sse_cast<typename V::VectorType>(_mm_movehl_ps(tmp1, tmp0));
        v2.data() = SSE::sse_cast<typename V::VectorType>(_mm_movelh_ps(tmp2, tmp3));
    }/*}}}*/
    template<typename I> static inline void deinterleave(typename V::EntryType const *const data,/*{{{*/
            const I &i, V &v0, V &v1, V &v2, V &v3)
    {
        const __m128 a = _mm_loadu_ps(aliasing_cast<float>(&data[i[0]]));
        const __m128 b = _mm_loadu_ps(aliasing_cast<float>(&data[i[1]]));
        const __m128 c = _mm_loadu_ps(aliasing_cast<float>(&data[i[2]]));
        const __m128 d = _mm_loadu_ps(aliasing_cast<float>(&data[i[3]]));

        const __m128 tmp0 = _mm_unpacklo_ps(a, b); // [a0 a1 b0 b1]
        const __m128 tmp1 = _mm_unpacklo_ps(c, d); // [a2 a3 b2 b3]
        const __m128 tmp2 = _mm_unpackhi_ps(a, b); // [c0 c1 d0 d1]
        const __m128 tmp3 = _mm_unpackhi_ps(c, d); // [c2 c3 d2 d3]

        v0.data() = SSE::sse_cast<typename V::VectorType>(_mm_movelh_ps(tmp0, tmp1));
        v1.data() = SSE::sse_cast<typename V::VectorType>(_mm_movehl_ps(tmp1, tmp0));
        v2.data() = SSE::sse_cast<typename V::VectorType>(_mm_movelh_ps(tmp2, tmp3));
        v3.data() = SSE::sse_cast<typename V::VectorType>(_mm_movehl_ps(tmp3, tmp2));
    }/*}}}*/
    template<typename I> static inline void deinterleave(typename V::EntryType const *const data,/*{{{*/
            const I &i, V &v0, V &v1, V &v2, V &v3, V &v4)
    {
        deinterleave(data, i, v0, v1, v2, v3);
        v4.gather(data + 4, i);
    }/*}}}*/
    template<typename I> static inline void deinterleave(typename V::EntryType const *const data,/*{{{*/
            const I &i, V &v0, V &v1, V &v2, V &v3, V &v4, V &v5)
    {
        deinterleave(data, i, v0, v1, v2, v3);
        deinterleave(data + 4, i, v4, v5);
    }/*}}}*/
    template<typename I> static inline void deinterleave(typename V::EntryType const *const data,/*{{{*/
            const I &i, V &v0, V &v1, V &v2, V &v3, V &v4, V &v5, V &v6)
    {
        deinterleave(data, i, v0, v1, v2, v3);
        deinterleave(data + 4, i, v4, v5, v6);
    }/*}}}*/
    template<typename I> static inline void deinterleave(typename V::EntryType const *const data,/*{{{*/
            const I &i, V &v0, V &v1, V &v2, V &v3, V &v4, V &v5, V &v6, V &v7)
    {
        deinterleave(data, i, v0, v1, v2, v3);
        deinterleave(data + 4, i, v4, v5, v6, v7);
    }/*}}}*/
};
template<typename V> struct InterleaveImpl<V, 2, 16> {
    template <typename I>  // interleave 2 args {{{2
    static inline void interleave(typename V::EntryType *const data, const I &i,
                                  const typename V::AsArg v0, const typename V::AsArg v1)
    {
        const __m128d tmp0 = _mm_unpacklo_pd(v0.data(), v1.data());
        const __m128d tmp1 = _mm_unpackhi_pd(v0.data(), v1.data());
        _mm_storeu_pd(&data[i[0]], tmp0);
        _mm_storeu_pd(&data[i[1]], tmp1);
    }
    template <typename I>  // interleave 3 args {{{2
    static inline void interleave(typename V::EntryType *const data, const I &i,
                                  const typename V::AsArg v0, const typename V::AsArg v1,
                                  const typename V::AsArg v2)
    {
        interleave(data, i, v0, v1);
        v2.scatter(data + 2, i);
    }
    template <typename I>  // interleave 4 args {{{2
    static inline void interleave(typename V::EntryType *const data, const I &i,
                                  const typename V::AsArg v0, const typename V::AsArg v1,
                                  const typename V::AsArg v2, const typename V::AsArg v3)
    {
        interleave(data, i, v0, v1);
        interleave(data + 2, i, v2, v3);
    }
    template <typename I>  // interleave 5 args {{{2
    static inline void interleave(typename V::EntryType *const data, const I &i,
                                  const typename V::AsArg v0, const typename V::AsArg v1,
                                  const typename V::AsArg v2, const typename V::AsArg v3,
                                  const typename V::AsArg v4)
    {
        interleave(data, i, v0, v1, v2, v3);
        v4.scatter(data + 4, i);
    }
    template <typename I>  // interleave 6 args {{{2
    static inline void interleave(typename V::EntryType *const data, const I &i,
                                  const typename V::AsArg v0, const typename V::AsArg v1,
                                  const typename V::AsArg v2, const typename V::AsArg v3,
                                  const typename V::AsArg v4, const typename V::AsArg v5)
    {
        interleave(data, i, v0, v1, v2, v3);
        interleave(data + 4, i, v4, v5);
    }
    template <typename I>  // interleave 7 args {{{2
    static inline void interleave(typename V::EntryType *const data, const I &i,
                                  const typename V::AsArg v0, const typename V::AsArg v1,
                                  const typename V::AsArg v2, const typename V::AsArg v3,
                                  const typename V::AsArg v4, const typename V::AsArg v5,
                                  const typename V::AsArg v6)
    {
        interleave(data, i, v0, v1, v2, v3);
        interleave(data + 4, i, v4, v5, v6);
    }
    template <typename I>  // interleave 8 args {{{2
    static inline void interleave(typename V::EntryType *const data, const I &i,
                                  const typename V::AsArg v0, const typename V::AsArg v1,
                                  const typename V::AsArg v2, const typename V::AsArg v3,
                                  const typename V::AsArg v4, const typename V::AsArg v5,
                                  const typename V::AsArg v6, const typename V::AsArg v7)
    {
        interleave(data, i, v0, v1, v2, v3);
        interleave(data + 4, i, v4, v5, v6, v7);
    }
    //}}}2
    template<typename I> static inline void deinterleave(typename V::EntryType const *const data,/*{{{*/
            const I &i, V &v0, V &v1)
    {
        const __m128d a = _mm_loadu_pd(&data[i[0]]);
        const __m128d b = _mm_loadu_pd(&data[i[1]]);

        v0.data() = _mm_unpacklo_pd(a, b);
        v1.data() = _mm_unpackhi_pd(a, b);
    }/*}}}*/
    template<typename I> static inline void deinterleave(typename V::EntryType const *const data,/*{{{*/
            const I &i, V &v0, V &v1, V &v2)
    {
        v2.gather(data + 2, i);
        deinterleave(data, i, v0, v1);
    }/*}}}*/
    template<typename I> static inline void deinterleave(typename V::EntryType const *const data,/*{{{*/
            const I &i, V &v0, V &v1, V &v2, V &v3)
    {
        deinterleave(data, i, v0, v1);
        deinterleave(data + 2, i, v2, v3);
    }/*}}}*/
    template<typename I> static inline void deinterleave(typename V::EntryType const *const data,/*{{{*/
            const I &i, V &v0, V &v1, V &v2, V &v3, V &v4)
    {
        deinterleave(data, i, v0, v1);
        deinterleave(data + 2, i, v2, v3);
        v4.gather(data + 4, i);
    }/*}}}*/
    template<typename I> static inline void deinterleave(typename V::EntryType const *const data,/*{{{*/
            const I &i, V &v0, V &v1, V &v2, V &v3, V &v4, V &v5)
    {
        deinterleave(data, i, v0, v1);
        deinterleave(data + 2, i, v2, v3);
        deinterleave(data + 4, i, v4, v5);
    }/*}}}*/
    template<typename I> static inline void deinterleave(typename V::EntryType const *const data,/*{{{*/
            const I &i, V &v0, V &v1, V &v2, V &v3, V &v4, V &v5, V &v6)
    {
        deinterleave(data, i, v0, v1);
        deinterleave(data + 2, i, v2, v3);
        deinterleave(data + 4, i, v4, v5);
        v6.gather(data + 6, i);
    }/*}}}*/
    template<typename I> static inline void deinterleave(typename V::EntryType const *const data,/*{{{*/
            const I &i, V &v0, V &v1, V &v2, V &v3, V &v4, V &v5, V &v6, V &v7)
    {
        deinterleave(data, i, v0, v1);
        deinterleave(data + 2, i, v2, v3);
        deinterleave(data + 4, i, v4, v5);
        deinterleave(data + 6, i, v6, v7);
    }/*}}}*/
};

//}}}1
}  // namespace Detail
}  // namespace Vc

#endif  // VC_SSE_DETAIL_H_

// vim: foldmethod=marker
