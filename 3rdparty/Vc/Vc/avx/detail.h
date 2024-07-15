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

#ifndef VC_AVX_DETAIL_H_
#define VC_AVX_DETAIL_H_

#include "../sse/detail.h"
#include "macros.h"

namespace Vc_VERSIONED_NAMESPACE
{
namespace Detail
{
// (converting) load functions {{{1
template <typename Flags>
Vc_INTRINSIC Vc_PURE __m256 load(const float *x, Flags, LoadTag<__m256, float>,
                                 typename Flags::EnableIfAligned = nullptr)
{
    return _mm256_load_ps(x);
}
template <typename Flags>
Vc_INTRINSIC Vc_PURE __m256 load(const float *x, Flags, LoadTag<__m256, float>,
                                 typename Flags::EnableIfUnaligned = nullptr)
{
    return _mm256_loadu_ps(x);
}
template <typename Flags>
Vc_INTRINSIC Vc_PURE __m256 load(const float *x, Flags, LoadTag<__m256, float>,
                                 typename Flags::EnableIfStreaming = nullptr)
{
    return AvxIntrinsics::stream_load<__m256>(x);
}

template <typename Flags>
Vc_INTRINSIC Vc_PURE __m256d load(const double *x, Flags, LoadTag<__m256d, double>,
                                  typename Flags::EnableIfAligned = nullptr)
{
    return _mm256_load_pd(x);
}
template <typename Flags>
Vc_INTRINSIC Vc_PURE __m256d load(const double *x, Flags, LoadTag<__m256d, double>,
                                  typename Flags::EnableIfUnaligned = nullptr)
{
    return _mm256_loadu_pd(x);
}
template <typename Flags>
Vc_INTRINSIC Vc_PURE __m256d load(const double *x, Flags, LoadTag<__m256d, double>,
                                  typename Flags::EnableIfStreaming = nullptr)
{
    return AvxIntrinsics::stream_load<__m256d>(x);
}

template <typename Flags, typename T, typename = enable_if<std::is_integral<T>::value>>
Vc_INTRINSIC Vc_PURE __m256i
load(const T *x, Flags, LoadTag<__m256i, T>, typename Flags::EnableIfAligned = nullptr)
{
    return _mm256_load_si256(reinterpret_cast<const __m256i *>(x));
}
template <typename Flags, typename T, typename = enable_if<std::is_integral<T>::value>>
Vc_INTRINSIC Vc_PURE __m256i
load(const T *x, Flags, LoadTag<__m256i, T>, typename Flags::EnableIfUnaligned = nullptr)
{
    return _mm256_loadu_si256(reinterpret_cast<const __m256i *>(x));
}
template <typename Flags, typename T, typename = enable_if<std::is_integral<T>::value>>
Vc_INTRINSIC Vc_PURE __m256i
load(const T *x, Flags, LoadTag<__m256i, T>, typename Flags::EnableIfStreaming = nullptr)
{
    return AvxIntrinsics::stream_load<__m256i>(x);
}

// load32{{{2
Vc_INTRINSIC __m256 load32(const float *mem, when_aligned)
{
    return _mm256_load_ps(mem);
}
Vc_INTRINSIC __m256 load32(const float *mem, when_unaligned)
{
    return _mm256_loadu_ps(mem);
}
Vc_INTRINSIC __m256 load32(const float *mem, when_streaming)
{
    return AvxIntrinsics::stream_load<__m256>(mem);
}
Vc_INTRINSIC __m256d load32(const double *mem, when_aligned)
{
    return _mm256_load_pd(mem);
}
Vc_INTRINSIC __m256d load32(const double *mem, when_unaligned)
{
    return _mm256_loadu_pd(mem);
}
Vc_INTRINSIC __m256d load32(const double *mem, when_streaming)
{
    return AvxIntrinsics::stream_load<__m256d>(mem);
}
template <class T> Vc_INTRINSIC __m256i load32(const T *mem, when_aligned)
{
    static_assert(std::is_integral<T>::value, "load32<T> is only intended for integral T");
    return _mm256_load_si256(reinterpret_cast<const __m256i *>(mem));
}
template <class T> Vc_INTRINSIC __m256i load32(const T *mem, when_unaligned)
{
    static_assert(std::is_integral<T>::value, "load32<T> is only intended for integral T");
    return _mm256_loadu_si256(reinterpret_cast<const __m256i *>(mem));
}
template <class T> Vc_INTRINSIC __m256i load32(const T *mem, when_streaming)
{
    static_assert(std::is_integral<T>::value, "load32<T> is only intended for integral T");
    return AvxIntrinsics::stream_load<__m256i>(mem);
}

// MSVC workarounds{{{2
#ifdef Vc_MSVC
// work around: "fatal error C1001: An internal error has occurred in the compiler."
Vc_INTRINSIC __m256i load(const uint *mem, when_aligned, LoadTag<__m256i, int>)
{
    return _mm256_load_si256(reinterpret_cast<const __m256i *>(mem));
}

Vc_INTRINSIC __m256d load(const double *mem, when_unaligned, LoadTag<__m256d, double>)
{
    return _mm256_loadu_pd(mem);
}

template <typename V, typename DstT>
Vc_INTRINSIC __m256 load(const float *mem, when_aligned,
                         enable_if<(std::is_same<DstT, float>::value &&
                                    std::is_same<V, __m256>::value)> = nullarg)
{
    return _mm256_load_ps(mem);
}

template <typename V, typename DstT>
Vc_INTRINSIC __m256 load(const float *mem, when_unaligned,
                         enable_if<(std::is_same<DstT, float>::value &&
                                    std::is_same<V, __m256>::value)> = nullarg)
{
    return _mm256_loadu_ps(mem);
}

template <typename V, typename DstT>
Vc_INTRINSIC __m256 load(const float *mem, when_streaming,
                         enable_if<(std::is_same<DstT, float>::value &&
                                    std::is_same<V, __m256>::value)> = nullarg)
{
    return AvxIntrinsics::stream_load<__m256>(mem);
}

template <typename V, typename DstT>
Vc_INTRINSIC __m256d load(const double *mem, when_aligned,
                          enable_if<(std::is_same<DstT, double>::value &&
                                     std::is_same<V, __m256d>::value)> = nullarg)
{
    return _mm256_load_pd(mem);
}

template <typename V, typename DstT>
Vc_INTRINSIC __m256d load(const double *mem, when_unaligned,
                          enable_if<(std::is_same<DstT, double>::value &&
                                     std::is_same<V, __m256d>::value)> = nullarg)
{
    return _mm256_loadu_pd(mem);
}

template <typename V, typename DstT>
Vc_INTRINSIC __m256d load(const double *mem, when_streaming,
                          enable_if<(std::is_same<DstT, double>::value &&
                                     std::is_same<V, __m256d>::value)> = nullarg)
{
    return AvxIntrinsics::stream_load<__m256d>(mem);
}

template <typename V, typename DstT>
Vc_INTRINSIC __m256i load(const uint *mem, when_aligned,
                          enable_if<(std::is_same<DstT, uint>::value &&
                                     std::is_same<V, __m256i>::value)> = nullarg)
{
    return _mm256_load_si256(reinterpret_cast<const __m256i *>(mem));
}

template <typename V, typename DstT>
Vc_INTRINSIC __m256i load(const uint *mem, when_unaligned,
                          enable_if<(std::is_same<DstT, uint>::value &&
                                     std::is_same<V, __m256i>::value)> = nullarg)
{
    return _mm256_loadu_si256(reinterpret_cast<const __m256i *>(mem));
}

template <typename V, typename DstT>
Vc_INTRINSIC __m256i load(const uint *mem, when_streaming,
                          enable_if<(std::is_same<DstT, uint>::value &&
                                     std::is_same<V, __m256i>::value)> = nullarg)
{
    return AvxIntrinsics::stream_load<__m256i>(mem);
}

template <typename V, typename DstT>
Vc_INTRINSIC __m256i load(const int *mem, when_unaligned,
                          enable_if<(std::is_same<DstT, int>::value &&
                                     std::is_same<V, __m256i>::value)> = nullarg)
{
    return _mm256_loadu_si256(reinterpret_cast<const __m256i *>(mem));
}

template <typename V, typename DstT>
Vc_INTRINSIC __m256i load(const int *mem, when_aligned,
                          enable_if<(std::is_same<DstT, int>::value &&
                                     std::is_same<V, __m256i>::value)> = nullarg)
{
    return _mm256_load_si256(reinterpret_cast<const __m256i *>(mem));
}

template <typename V, typename DstT>
Vc_INTRINSIC __m256i load(const int *mem, when_streaming,
                          enable_if<(std::is_same<DstT, int>::value &&
                                     std::is_same<V, __m256i>::value)> = nullarg)
{
    return AvxIntrinsics::stream_load<__m256i>(mem);
}

template <typename V, typename DstT>
Vc_INTRINSIC __m256i load(const short *mem, when_unaligned,
                          enable_if<(std::is_same<DstT, short>::value &&
                                     std::is_same<V, __m256i>::value)> = nullarg)
{
    return _mm256_loadu_si256(reinterpret_cast<const __m256i *>(mem));
}

template <typename V, typename DstT>
Vc_INTRINSIC __m256i load(const short *mem, when_aligned,
                          enable_if<(std::is_same<DstT, short>::value &&
                                     std::is_same<V, __m256i>::value)> = nullarg)
{
    return _mm256_load_si256(reinterpret_cast<const __m256i *>(mem));
}

template <typename V, typename DstT>
Vc_INTRINSIC __m256i load(const short *mem, when_streaming,
                          enable_if<(std::is_same<DstT, short>::value &&
                                     std::is_same<V, __m256i>::value)> = nullarg)
{
    return AvxIntrinsics::stream_load<__m256i>(mem);
}

template <typename V, typename DstT>
Vc_INTRINSIC __m256i load(const ushort *mem, when_unaligned,
                          enable_if<(std::is_same<DstT, ushort>::value &&
                                     std::is_same<V, __m256i>::value)> = nullarg)
{
    return _mm256_loadu_si256(reinterpret_cast<const __m256i *>(mem));
}

template <typename V, typename DstT>
Vc_INTRINSIC __m256i load(const ushort *mem, when_aligned,
                          enable_if<(std::is_same<DstT, ushort>::value &&
                                     std::is_same<V, __m256i>::value)> = nullarg)
{
    return _mm256_load_si256(reinterpret_cast<const __m256i *>(mem));
}

template <typename V, typename DstT>
Vc_INTRINSIC __m256i load(const ushort *mem, when_streaming,
                          enable_if<(std::is_same<DstT, ushort>::value &&
                                     std::is_same<V, __m256i>::value)> = nullarg)
{
    return AvxIntrinsics::stream_load<__m256i>(mem);
}

#endif  // Vc_MSVC

// short {{{2
template <typename Flags>
Vc_INTRINSIC __m256i load(const ushort *mem, Flags f, LoadTag<__m256i, short>)
{
    return load32(mem, f);
}
template <typename Flags>
Vc_INTRINSIC __m256i load(const uchar *mem, Flags f, LoadTag<__m256i, short>)
{
    return AVX::cvtepu8_epi16(load16(mem, f));
}
template <typename Flags>
Vc_INTRINSIC __m256i load(const schar *mem, Flags f, LoadTag<__m256i, short>)
{
    return AVX::cvtepi8_epi16(load16(mem, f));
}

// ushort {{{2
template <typename Flags>
Vc_INTRINSIC __m256i load(const uchar *mem, Flags f, LoadTag<__m256i, ushort>)
{
    return AVX::cvtepu8_epi16(load16(mem, f));
}

// int {{{2
template <typename Flags>
Vc_INTRINSIC __m256i load(const uint *mem, Flags f, LoadTag<__m256i, int>)
{
    return load32(mem, f);
}
template <typename Flags>
Vc_INTRINSIC __m256i load(const ushort *mem, Flags f, LoadTag<__m256i, int>)
{
    return AVX::cvtepu16_epi32(load16(mem, f));
}
template <typename Flags>
Vc_INTRINSIC __m256i load(const short *mem, Flags f, LoadTag<__m256i, int>)
{
    return AVX::cvtepi16_epi32(load16(mem, f));
}
template <typename Flags>
Vc_INTRINSIC __m256i load(const uchar *mem, Flags, LoadTag<__m256i, int>)
{
    return AVX::cvtepu8_epi32(_mm_loadl_epi64(reinterpret_cast<const __m128i *>(mem)));
}
template <typename Flags>
Vc_INTRINSIC __m256i load(const schar *mem, Flags, LoadTag<__m256i, int>)
{
    return AVX::cvtepi8_epi32(_mm_loadl_epi64(reinterpret_cast<const __m128i *>(mem)));
}

// uint {{{2
template <typename Flags>
Vc_INTRINSIC __m256i load(const ushort *mem, Flags f, LoadTag<__m256i, uint>)
{
    return AVX::cvtepu16_epi32(load16(mem, f));
}
template <typename Flags>
Vc_INTRINSIC __m256i load(const uchar *mem, Flags, LoadTag<__m256i, uint>)
{
    return AVX::cvtepu8_epi32(_mm_loadl_epi64(reinterpret_cast<const __m128i *>(mem)));
}

// double {{{2
template <typename Flags>
Vc_INTRINSIC __m256d load(const float *mem, Flags f, LoadTag<__m256d, double>)
{
    return AVX::convert<float, double>(load16(mem, f));
}
template <typename Flags>
Vc_INTRINSIC __m256d load(const uint *mem, Flags f, LoadTag<__m256d, double>)
{
    return AVX::convert<uint, double>(load16(mem, f));
}
template <typename Flags>
Vc_INTRINSIC __m256d load(const int *mem, Flags f, LoadTag<__m256d, double>)
{
    return AVX::convert<int, double>(load16(mem, f));
}
template <typename Flags>
Vc_INTRINSIC __m256d load(const ushort *mem, Flags f, LoadTag<__m256d, double>)
{
    return AVX::convert<int, double>(load16(mem, f));
}
template <typename Flags>
Vc_INTRINSIC __m256d load(const short *mem, Flags f, LoadTag<__m256d, double>)
{
    return AVX::convert<int, double>(load16(mem, f));
}
template <typename Flags>
Vc_INTRINSIC __m256d load(const uchar *mem, Flags f, LoadTag<__m256d, double>)
{
    return AVX::convert<int, double>(load16(mem, f));
}
template <typename Flags>
Vc_INTRINSIC __m256d load(const schar *mem, Flags f, LoadTag<__m256d, double>)
{
    return AVX::convert<int, double>(load16(mem, f));
}

// float {{{2
template <typename Flags>
Vc_INTRINSIC __m256 load(const double *mem, Flags f, LoadTag<__m256, float>)
{
    return AVX::concat(_mm256_cvtpd_ps(load32(&mem[0], f)),
                       _mm256_cvtpd_ps(load32(&mem[4], f)));
}
template <typename Flags>
Vc_INTRINSIC __m256 load(const uint *mem, Flags f, LoadTag<__m256, float>)
{
    const auto v = load32(mem, f);
    return _mm256_blendv_ps(
        _mm256_cvtepi32_ps(v),
        _mm256_add_ps(_mm256_cvtepi32_ps(AVX::sub_epi32(v, AVX::set2power31_epu32())),
                      AVX::set2power31_ps()),
        _mm256_castsi256_ps(AVX::cmplt_epi32(v, _mm256_setzero_si256())));
}
template <typename Flags>
Vc_INTRINSIC __m256 load(const int *mem, Flags f, LoadTag<__m256, float>)
{
    return AVX::convert<int, float>(load32(mem, f));
}
template <typename T, typename Flags,
          typename = enable_if<!std::is_same<T, float>::value>>
Vc_INTRINSIC __m256 load(const T *mem, Flags f, LoadTag<__m256, float>)
{
    return _mm256_cvtepi32_ps(load<__m256i, int>(mem, f));
}
template <typename Flags>
Vc_INTRINSIC __m256 load(const ushort *mem, Flags f, LoadTag<__m256, float>)
{
    return AVX::convert<ushort, float>(load16(mem, f));
}
template <typename Flags>
Vc_INTRINSIC __m256 load(const short *mem, Flags f, LoadTag<__m256, float>)
{
    return AVX::convert<short, float>(load16(mem, f));
}
/*
template<typename Flags> struct LoadHelper<float, unsigned char, Flags> {
    static __m256 load(const unsigned char *mem, Flags)
    {
        return _mm256_cvtepi32_ps(
            cvtepu8_epi32(_mm_loadl_epi64(reinterpret_cast<const __m128i *>(mem))));
    }
};
template<typename Flags> struct LoadHelper<float, signed char, Flags> {
    static __m256 load(const signed char *mem, Flags)
    {
        return _mm256_cvtepi32_ps(
            cvtepi8_epi32(_mm_loadl_epi64(reinterpret_cast<const __m128i *>(mem))));
    }
};
*/

// shifted{{{1
template <int amount, typename T>
Vc_INTRINSIC Vc_CONST enable_if<(sizeof(T) == 32 && amount >= 16), T> shifted(T k)
{
    return AVX::avx_cast<T>(AVX::zeroExtend(
        _mm_srli_si128(AVX::hi128(AVX::avx_cast<__m256i>(k)), amount - 16)));
}
template <int amount, typename T>
Vc_INTRINSIC Vc_CONST enable_if<(sizeof(T) == 32 && amount > 0 && amount < 16), T>
shifted(T k)
{
    return AVX::avx_cast<T>(
        AVX::alignr<amount>(Mem::permute128<X1, Const0>(AVX::avx_cast<__m256i>(k)),
                            AVX::avx_cast<__m256i>(k)));
}
template <int amount, typename T>
Vc_INTRINSIC Vc_CONST enable_if<(sizeof(T) == 32 && amount <= -16), T> shifted(T k)
{
    return AVX::avx_cast<T>(Mem::permute128<Const0, X0>(AVX::avx_cast<__m256i>(
        _mm_slli_si128(AVX::lo128(AVX::avx_cast<__m256i>(k)), -16 - amount))));
}
template <int amount, typename T>
Vc_INTRINSIC Vc_CONST enable_if<(sizeof(T) == 32 && amount > -16 && amount < 0), T>
shifted(T k)
{
    return AVX::avx_cast<T>(
        AVX::alignr<16 + amount>(AVX::avx_cast<__m256i>(k),
                                 Mem::permute128<Const0, X0>(AVX::avx_cast<__m256i>(k))));
}
// mask_cast{{{1
template<size_t From, size_t To, typename R> Vc_INTRINSIC Vc_CONST R mask_cast(__m256i k)
{
    static_assert(From == To, "Incorrect mask cast.");
    static_assert(std::is_same<R, __m256>::value, "Incorrect mask cast.");
    return AVX::avx_cast<__m256>(k);
}

// 4 -> 4
template <> Vc_INTRINSIC Vc_CONST __m128 mask_cast<4, 4, __m128>(__m256i k)
{
    return AVX::avx_cast<__m128>(_mm_packs_epi32(AVX::lo128(k), AVX::hi128(k)));
}

template <> Vc_INTRINSIC Vc_CONST __m256 mask_cast<4, 4, __m256>(__m128i k)
{
    const auto kk = _mm_castsi128_ps(k);
    return AVX::concat(_mm_unpacklo_ps(kk, kk), _mm_unpackhi_ps(kk, kk));
}

// 4 -> 8
template<> Vc_INTRINSIC Vc_CONST __m256 mask_cast<4, 8, __m256>(__m256i k)
{
    // aabb ccdd -> abcd 0000
    return AVX::avx_cast<__m256>(AVX::concat(_mm_packs_epi32(AVX::lo128(k), AVX::hi128(k)),
                                 _mm_setzero_si128()));
}

template<> Vc_INTRINSIC Vc_CONST __m128 mask_cast<4, 8, __m128>(__m256i k)
{
    // aaaa bbbb cccc dddd -> abcd 0000
    return AVX::avx_cast<__m128>(_mm_packs_epi16(_mm_packs_epi32(AVX::lo128(k), AVX::hi128(k)), _mm_setzero_si128()));
}

template <> Vc_INTRINSIC Vc_CONST __m256 mask_cast<4, 8, __m256>(__m128i k)
{
    return AVX::zeroExtend(AVX::avx_cast<__m128>(k));
}

// 4 -> 16
template<> Vc_INTRINSIC Vc_CONST __m256 mask_cast<4, 16, __m256>(__m256i k)
{
    // aaaa bbbb cccc dddd -> abcd 0000 0000 0000
    return AVX::zeroExtend(mask_cast<4, 8, __m128>(k));
}

// 8 -> 4
template<> Vc_INTRINSIC Vc_CONST __m256 mask_cast<8, 4, __m256>(__m256i k)
{
    // aabb ccdd eeff gghh -> aaaa bbbb cccc dddd
    const auto lo = AVX::lo128(AVX::avx_cast<__m256>(k));
    return AVX::concat(_mm_unpacklo_ps(lo, lo),
                  _mm_unpackhi_ps(lo, lo));
}

template<> Vc_INTRINSIC Vc_CONST __m128 mask_cast<8, 4, __m128>(__m256i k)
{
    return AVX::avx_cast<__m128>(AVX::lo128(k));
}

template<> Vc_INTRINSIC Vc_CONST __m256 mask_cast<8, 4, __m256>(__m128i k)
{
    // abcd efgh -> aaaa bbbb cccc dddd
    const auto tmp = _mm_unpacklo_epi16(k, k); // aa bb cc dd
    return AVX::avx_cast<__m256>(AVX::concat(_mm_unpacklo_epi32(tmp, tmp), // aaaa bbbb
                                 _mm_unpackhi_epi32(tmp, tmp))); // cccc dddd
}

// 8 -> 8
template<> Vc_INTRINSIC Vc_CONST __m128 mask_cast<8, 8, __m128>(__m256i k)
{
    // aabb ccdd eeff gghh -> abcd efgh
    return AVX::avx_cast<__m128>(_mm_packs_epi16(AVX::lo128(k), AVX::hi128(k)));
}

template<> Vc_INTRINSIC Vc_CONST __m256 mask_cast<8, 8, __m256>(__m128i k)
{
    return AVX::avx_cast<__m256>(AVX::concat(_mm_unpacklo_epi16(k, k),
                                 _mm_unpackhi_epi16(k, k)));
}

// 8 -> 16
template<> Vc_INTRINSIC Vc_CONST __m256 mask_cast<8, 16, __m256>(__m256i k)
{
    // aabb ccdd eeff gghh -> abcd efgh 0000 0000
    return AVX::zeroExtend(mask_cast<8, 8, __m128>(k));
}

// 16 -> 8
#ifdef Vc_IMPL_AVX2
template<> Vc_INTRINSIC Vc_CONST __m256 mask_cast<16, 8, __m256>(__m256i k)
{
    // abcd efgh ijkl mnop -> aabb ccdd eeff gghh
    const auto flipped = Mem::permute4x64<X0, X2, X1, X3>(k);
    return _mm256_castsi256_ps(AVX::unpacklo_epi16(flipped, flipped));
}
#endif

// 16 -> 4
template<> Vc_INTRINSIC Vc_CONST __m256 mask_cast<16, 4, __m256>(__m256i k)
{
    // abcd efgh ijkl mnop -> aaaa bbbb cccc dddd
    const auto tmp = _mm_unpacklo_epi16(AVX::lo128(k), AVX::lo128(k)); // aabb ccdd
    return _mm256_castsi256_ps(AVX::concat(_mm_unpacklo_epi32(tmp, tmp), _mm_unpackhi_epi32(tmp, tmp)));
}

// allone{{{1
template<> Vc_INTRINSIC Vc_CONST __m256  allone<__m256 >() { return AVX::setallone_ps(); }
template<> Vc_INTRINSIC Vc_CONST __m256i allone<__m256i>() { return AVX::setallone_si256(); }
template<> Vc_INTRINSIC Vc_CONST __m256d allone<__m256d>() { return AVX::setallone_pd(); }

// zero{{{1
template<> Vc_INTRINSIC Vc_CONST __m256  zero<__m256 >() { return _mm256_setzero_ps(); }
template<> Vc_INTRINSIC Vc_CONST __m256i zero<__m256i>() { return _mm256_setzero_si256(); }
template<> Vc_INTRINSIC Vc_CONST __m256d zero<__m256d>() { return _mm256_setzero_pd(); }

// one{{{1
Vc_INTRINSIC Vc_CONST __m256  one( float) { return AVX::setone_ps   (); }
Vc_INTRINSIC Vc_CONST __m256d one(double) { return AVX::setone_pd   (); }
Vc_INTRINSIC Vc_CONST __m256i one(   int) { return AVX::setone_epi32(); }
Vc_INTRINSIC Vc_CONST __m256i one(  uint) { return AVX::setone_epu32(); }
Vc_INTRINSIC Vc_CONST __m256i one( short) { return AVX::setone_epi16(); }
Vc_INTRINSIC Vc_CONST __m256i one(ushort) { return AVX::setone_epu16(); }
Vc_INTRINSIC Vc_CONST __m256i one( schar) { return AVX::setone_epi8 (); }
Vc_INTRINSIC Vc_CONST __m256i one( uchar) { return AVX::setone_epu8 (); }

// negate{{{1
Vc_ALWAYS_INLINE Vc_CONST __m256 negate(__m256 v, std::integral_constant<std::size_t, 4>)
{
    return _mm256_xor_ps(v, AVX::setsignmask_ps());
}
Vc_ALWAYS_INLINE Vc_CONST __m256d negate(__m256d v, std::integral_constant<std::size_t, 8>)
{
    return _mm256_xor_pd(v, AVX::setsignmask_pd());
}
Vc_ALWAYS_INLINE Vc_CONST __m256i negate(__m256i v, std::integral_constant<std::size_t, 4>)
{
    return AVX::sign_epi32(v, Detail::allone<__m256i>());
}
Vc_ALWAYS_INLINE Vc_CONST __m256i negate(__m256i v, std::integral_constant<std::size_t, 2>)
{
    return AVX::sign_epi16(v, Detail::allone<__m256i>());
}

// xor_{{{1
Vc_INTRINSIC __m256 xor_(__m256 a, __m256 b) { return _mm256_xor_ps(a, b); }
Vc_INTRINSIC __m256d xor_(__m256d a, __m256d b) { return _mm256_xor_pd(a, b); }
Vc_INTRINSIC __m256i xor_(__m256i a, __m256i b)
{
#ifdef Vc_IMPL_AVX2
    return _mm256_xor_si256(a, b);
#else
    return _mm256_castps_si256(
        _mm256_xor_ps(_mm256_castsi256_ps(a), _mm256_castsi256_ps(b)));
#endif
}

// or_{{{1
Vc_INTRINSIC __m256 or_(__m256 a, __m256 b) { return _mm256_or_ps(a, b); }
Vc_INTRINSIC __m256d or_(__m256d a, __m256d b) { return _mm256_or_pd(a, b); }
Vc_INTRINSIC __m256i or_(__m256i a, __m256i b)
{
#ifdef Vc_IMPL_AVX2
    return _mm256_or_si256(a, b);
#else
    return _mm256_castps_si256(
        _mm256_or_ps(_mm256_castsi256_ps(a), _mm256_castsi256_ps(b)));
#endif
}

// and_{{{1
Vc_INTRINSIC __m256 and_(__m256 a, __m256 b) { return _mm256_and_ps(a, b); }
Vc_INTRINSIC __m256d and_(__m256d a, __m256d b) { return _mm256_and_pd(a, b); }
Vc_INTRINSIC __m256i and_(__m256i a, __m256i b) {
#ifdef Vc_IMPL_AVX2
    return _mm256_and_si256(a, b);
#else
    return _mm256_castps_si256(
        _mm256_and_ps(_mm256_castsi256_ps(a), _mm256_castsi256_ps(b)));
#endif
}

// andnot_{{{1
Vc_INTRINSIC __m256 andnot_(__m256 a, __m256 b) { return _mm256_andnot_ps(a, b); }
Vc_INTRINSIC __m256d andnot_(__m256d a, __m256d b) { return _mm256_andnot_pd(a, b); }
Vc_INTRINSIC __m256i andnot_(__m256i a, __m256i b)
{
#ifdef Vc_IMPL_AVX2
    return _mm256_andnot_si256(a, b);
#else
    return _mm256_castps_si256(
        _mm256_andnot_ps(_mm256_castsi256_ps(a), _mm256_castsi256_ps(b)));
#endif
}

// not_{{{1
Vc_INTRINSIC __m256  not_(__m256  a) { return andnot_(a, allone<__m256 >()); }
Vc_INTRINSIC __m256d not_(__m256d a) { return andnot_(a, allone<__m256d>()); }
Vc_INTRINSIC __m256i not_(__m256i a) { return andnot_(a, allone<__m256i>()); }

// blend{{{1
Vc_INTRINSIC __m256  blend(__m256  a, __m256  b, __m256  c) { return _mm256_blendv_ps(a, b, c); }
Vc_INTRINSIC __m256d blend(__m256d a, __m256d b, __m256d c) { return _mm256_blendv_pd(a, b, c); }
Vc_INTRINSIC __m256i blend(__m256i a, __m256i b, __m256i c) { return AVX::blendv_epi8(a, b, c); }

// abs{{{1
Vc_INTRINSIC __m256  abs(__m256  a,  float) { return and_(a, AVX::setabsmask_ps()); }
Vc_INTRINSIC __m256d abs(__m256d a, double) { return and_(a, AVX::setabsmask_pd()); }
Vc_INTRINSIC __m256i abs(__m256i a,    int) { return AVX::abs_epi32(a); }
Vc_INTRINSIC __m256i abs(__m256i a,   uint) { return a; }
Vc_INTRINSIC __m256i abs(__m256i a,  short) { return AVX::abs_epi16(a); }
Vc_INTRINSIC __m256i abs(__m256i a, ushort) { return a; }
Vc_INTRINSIC __m256i abs(__m256i a,  schar) { return AVX::abs_epi8 (a); }
Vc_INTRINSIC __m256i abs(__m256i a,  uchar) { return a; }

// add{{{1
Vc_INTRINSIC __m256  add(__m256  a, __m256  b,  float) { return _mm256_add_ps(a, b); }
Vc_INTRINSIC __m256d add(__m256d a, __m256d b, double) { return _mm256_add_pd(a, b); }
Vc_INTRINSIC __m256i add(__m256i a, __m256i b,    int) { return AVX::add_epi32(a, b); }
Vc_INTRINSIC __m256i add(__m256i a, __m256i b,   uint) { return AVX::add_epi32(a, b); }
Vc_INTRINSIC __m256i add(__m256i a, __m256i b,  short) { return AVX::add_epi16(a, b); }
Vc_INTRINSIC __m256i add(__m256i a, __m256i b, ushort) { return AVX::add_epi16(a, b); }

// sub{{{1
Vc_INTRINSIC __m256  sub(__m256  a, __m256  b,  float) { return _mm256_sub_ps(a, b); }
Vc_INTRINSIC __m256d sub(__m256d a, __m256d b, double) { return _mm256_sub_pd(a, b); }
Vc_INTRINSIC __m256i sub(__m256i a, __m256i b,    int) { return AVX::sub_epi32(a, b); }
Vc_INTRINSIC __m256i sub(__m256i a, __m256i b,   uint) { return AVX::sub_epi32(a, b); }
Vc_INTRINSIC __m256i sub(__m256i a, __m256i b,  short) { return AVX::sub_epi16(a, b); }
Vc_INTRINSIC __m256i sub(__m256i a, __m256i b, ushort) { return AVX::sub_epi16(a, b); }

// mul{{{1
Vc_INTRINSIC __m256  mul(__m256  a, __m256  b,  float) { return _mm256_mul_ps(a, b); }
Vc_INTRINSIC __m256d mul(__m256d a, __m256d b, double) { return _mm256_mul_pd(a, b); }
Vc_INTRINSIC __m256i mul(__m256i a, __m256i b,    int) { return AVX::mullo_epi32(a, b); }
Vc_INTRINSIC __m256i mul(__m256i a, __m256i b,   uint) { return AVX::mullo_epi32(a, b); }
Vc_INTRINSIC __m256i mul(__m256i a, __m256i b,  short) { return AVX::mullo_epi16(a, b); }
Vc_INTRINSIC __m256i mul(__m256i a, __m256i b, ushort) { return AVX::mullo_epi16(a, b); }

// mul{{{1
Vc_INTRINSIC __m256  div(__m256  a, __m256  b,  float) { return _mm256_div_ps(a, b); }
Vc_INTRINSIC __m256d div(__m256d a, __m256d b, double) { return _mm256_div_pd(a, b); }
Vc_INTRINSIC __m256i div(__m256i a, __m256i b,    int) {
    using namespace AVX;
    const __m256d lo1 = _mm256_cvtepi32_pd(lo128(a));
    const __m256d lo2 = _mm256_cvtepi32_pd(lo128(b));
    const __m256d hi1 = _mm256_cvtepi32_pd(hi128(a));
    const __m256d hi2 = _mm256_cvtepi32_pd(hi128(b));
    return concat(_mm256_cvttpd_epi32(_mm256_div_pd(lo1, lo2)),
                  _mm256_cvttpd_epi32(_mm256_div_pd(hi1, hi2)));
}
Vc_INTRINSIC __m256i div(__m256i a, __m256i b,   uint) {
    // SSE/AVX only has signed int conversion to doubles. Therefore we first adjust the input before
    // conversion and take the adjustment back after the conversion.
    // It could be argued that for b this is not really important because division by a b >= 2^31 is
    // useless. But for full correctness it cannot be ignored.
    using namespace AVX;
    const __m256i aa = add_epi32(a, set1_epi32(-2147483648));
    const __m256i bb = add_epi32(b, set1_epi32(-2147483648));
    const __m256d loa = _mm256_add_pd(_mm256_cvtepi32_pd(lo128(aa)), set1_pd(2147483648.));
    const __m256d hia = _mm256_add_pd(_mm256_cvtepi32_pd(hi128(aa)), set1_pd(2147483648.));
    const __m256d lob = _mm256_add_pd(_mm256_cvtepi32_pd(lo128(bb)), set1_pd(2147483648.));
    const __m256d hib = _mm256_add_pd(_mm256_cvtepi32_pd(hi128(bb)), set1_pd(2147483648.));
    // there is one remaining problem: a >= 2^31 and b == 1
    // in that case the return value would be 2^31
    return avx_cast<__m256i>(_mm256_blendv_ps(
        avx_cast<__m256>(concat(_mm256_cvttpd_epi32(_mm256_div_pd(loa, lob)),
                                          _mm256_cvttpd_epi32(_mm256_div_pd(hia, hib)))),
        avx_cast<__m256>(a),
        avx_cast<__m256>(cmpeq_epi32(b, setone_epi32()))));
}
Vc_INTRINSIC __m256i div(__m256i a, __m256i b,  short) {
    using namespace AVX;
    const __m256 lo =
        _mm256_div_ps(convert<short, float>(lo128(a)), convert<short, float>(lo128(b)));
    const __m256 hi =
        _mm256_div_ps(convert<short, float>(hi128(a)), convert<short, float>(hi128(b)));
    return concat(convert<float, short>(lo), convert<float, short>(hi));
}

// horizontal add{{{1
template <typename T> Vc_INTRINSIC T add(Common::IntrinsicType<T, 32 / sizeof(T)> a, T)
{
    return {add(add(AVX::lo128(a), AVX::hi128(a), T()), T())};
}

// horizontal mul{{{1
template <typename T> Vc_INTRINSIC T mul(Common::IntrinsicType<T, 32 / sizeof(T)> a, T)
{
    return {mul(mul(AVX::lo128(a), AVX::hi128(a), T()), T())};
}

// horizontal min{{{1
template <typename T> Vc_INTRINSIC T min(Common::IntrinsicType<T, 32 / sizeof(T)> a, T)
{
    return {min(min(AVX::lo128(a), AVX::hi128(a), T()), T())};
}

// horizontal max{{{1
template <typename T> Vc_INTRINSIC T max(Common::IntrinsicType<T, 32 / sizeof(T)> a, T)
{
    return {max(max(AVX::lo128(a), AVX::hi128(a), T()), T())};
}
// cmpeq{{{1
Vc_INTRINSIC __m256  cmpeq(__m256  a, __m256  b,  float) { return AvxIntrinsics::cmpeq_ps(a, b); }
Vc_INTRINSIC __m256d cmpeq(__m256d a, __m256d b, double) { return AvxIntrinsics::cmpeq_pd(a, b); }
Vc_INTRINSIC __m256i cmpeq(__m256i a, __m256i b,    int) { return AvxIntrinsics::cmpeq_epi32(a, b); }
Vc_INTRINSIC __m256i cmpeq(__m256i a, __m256i b,   uint) { return AvxIntrinsics::cmpeq_epi32(a, b); }
Vc_INTRINSIC __m256i cmpeq(__m256i a, __m256i b,  short) { return AvxIntrinsics::cmpeq_epi16(a, b); }
Vc_INTRINSIC __m256i cmpeq(__m256i a, __m256i b, ushort) { return AvxIntrinsics::cmpeq_epi16(a, b); }

// cmpneq{{{1
Vc_INTRINSIC __m256  cmpneq(__m256  a, __m256  b,  float) { return AvxIntrinsics::cmpneq_ps(a, b); }
Vc_INTRINSIC __m256d cmpneq(__m256d a, __m256d b, double) { return AvxIntrinsics::cmpneq_pd(a, b); }
Vc_INTRINSIC __m256i cmpneq(__m256i a, __m256i b,    int) { return not_(AvxIntrinsics::cmpeq_epi32(a, b)); }
Vc_INTRINSIC __m256i cmpneq(__m256i a, __m256i b,   uint) { return not_(AvxIntrinsics::cmpeq_epi32(a, b)); }
Vc_INTRINSIC __m256i cmpneq(__m256i a, __m256i b,  short) { return not_(AvxIntrinsics::cmpeq_epi16(a, b)); }
Vc_INTRINSIC __m256i cmpneq(__m256i a, __m256i b, ushort) { return not_(AvxIntrinsics::cmpeq_epi16(a, b)); }
Vc_INTRINSIC __m256i cmpneq(__m256i a, __m256i b,  schar) { return not_(AvxIntrinsics::cmpeq_epi8 (a, b)); }
Vc_INTRINSIC __m256i cmpneq(__m256i a, __m256i b,  uchar) { return not_(AvxIntrinsics::cmpeq_epi8 (a, b)); }

// cmpgt{{{1
Vc_INTRINSIC __m256  cmpgt(__m256  a, __m256  b,  float) { return AVX::cmpgt_ps(a, b); }
Vc_INTRINSIC __m256d cmpgt(__m256d a, __m256d b, double) { return AVX::cmpgt_pd(a, b); }
Vc_INTRINSIC __m256i cmpgt(__m256i a, __m256i b,    int) { return AVX::cmpgt_epi32(a, b); }
Vc_INTRINSIC __m256i cmpgt(__m256i a, __m256i b,   uint) { return AVX::cmpgt_epu32(a, b); }
Vc_INTRINSIC __m256i cmpgt(__m256i a, __m256i b,  short) { return AVX::cmpgt_epi16(a, b); }
Vc_INTRINSIC __m256i cmpgt(__m256i a, __m256i b, ushort) { return AVX::cmpgt_epu16(a, b); }
Vc_INTRINSIC __m256i cmpgt(__m256i a, __m256i b,  schar) { return AVX::cmpgt_epi8 (a, b); }
Vc_INTRINSIC __m256i cmpgt(__m256i a, __m256i b,  uchar) { return AVX::cmpgt_epu8 (a, b); }

// cmpge{{{1
Vc_INTRINSIC __m256  cmpge(__m256  a, __m256  b,  float) { return AVX::cmpge_ps(a, b); }
Vc_INTRINSIC __m256d cmpge(__m256d a, __m256d b, double) { return AVX::cmpge_pd(a, b); }
Vc_INTRINSIC __m256i cmpge(__m256i a, __m256i b,    int) { return not_(AVX::cmpgt_epi32(b, a)); }
Vc_INTRINSIC __m256i cmpge(__m256i a, __m256i b,   uint) { return not_(AVX::cmpgt_epu32(b, a)); }
Vc_INTRINSIC __m256i cmpge(__m256i a, __m256i b,  short) { return not_(AVX::cmpgt_epi16(b, a)); }
Vc_INTRINSIC __m256i cmpge(__m256i a, __m256i b, ushort) { return not_(AVX::cmpgt_epu16(b, a)); }
Vc_INTRINSIC __m256i cmpge(__m256i a, __m256i b,  schar) { return not_(AVX::cmpgt_epi8 (b, a)); }
Vc_INTRINSIC __m256i cmpge(__m256i a, __m256i b,  uchar) { return not_(AVX::cmpgt_epu8 (b, a)); }

// cmple{{{1
Vc_INTRINSIC __m256  cmple(__m256  a, __m256  b,  float) { return AVX::cmple_ps(a, b); }
Vc_INTRINSIC __m256d cmple(__m256d a, __m256d b, double) { return AVX::cmple_pd(a, b); }
Vc_INTRINSIC __m256i cmple(__m256i a, __m256i b,    int) { return not_(AVX::cmpgt_epi32(a, b)); }
Vc_INTRINSIC __m256i cmple(__m256i a, __m256i b,   uint) { return not_(AVX::cmpgt_epu32(a, b)); }
Vc_INTRINSIC __m256i cmple(__m256i a, __m256i b,  short) { return not_(AVX::cmpgt_epi16(a, b)); }
Vc_INTRINSIC __m256i cmple(__m256i a, __m256i b, ushort) { return not_(AVX::cmpgt_epu16(a, b)); }
Vc_INTRINSIC __m256i cmple(__m256i a, __m256i b,  schar) { return not_(AVX::cmpgt_epi8 (a, b)); }
Vc_INTRINSIC __m256i cmple(__m256i a, __m256i b,  uchar) { return not_(AVX::cmpgt_epu8 (a, b)); }

// cmplt{{{1
Vc_INTRINSIC __m256  cmplt(__m256  a, __m256  b,  float) { return AVX::cmplt_ps(a, b); }
Vc_INTRINSIC __m256d cmplt(__m256d a, __m256d b, double) { return AVX::cmplt_pd(a, b); }
Vc_INTRINSIC __m256i cmplt(__m256i a, __m256i b,    int) { return AVX::cmpgt_epi32(b, a); }
Vc_INTRINSIC __m256i cmplt(__m256i a, __m256i b,   uint) { return AVX::cmpgt_epu32(b, a); }
Vc_INTRINSIC __m256i cmplt(__m256i a, __m256i b,  short) { return AVX::cmpgt_epi16(b, a); }
Vc_INTRINSIC __m256i cmplt(__m256i a, __m256i b, ushort) { return AVX::cmpgt_epu16(b, a); }
Vc_INTRINSIC __m256i cmplt(__m256i a, __m256i b,  schar) { return AVX::cmpgt_epi8 (b, a); }
Vc_INTRINSIC __m256i cmplt(__m256i a, __m256i b,  uchar) { return AVX::cmpgt_epu8 (b, a); }

// fma{{{1
Vc_INTRINSIC __m256 fma(__m256  a, __m256  b, __m256  c,  float) {
#ifdef Vc_IMPL_FMA4
    return _mm256_macc_ps(a, b, c);
#elif defined Vc_IMPL_FMA
    return _mm256_fmadd_ps(a, b, c);
#else
    using namespace AVX;
    __m256d v1_0 = _mm256_cvtps_pd(lo128(a));
    __m256d v1_1 = _mm256_cvtps_pd(hi128(a));
    __m256d v2_0 = _mm256_cvtps_pd(lo128(b));
    __m256d v2_1 = _mm256_cvtps_pd(hi128(b));
    __m256d v3_0 = _mm256_cvtps_pd(lo128(c));
    __m256d v3_1 = _mm256_cvtps_pd(hi128(c));
    return concat(_mm256_cvtpd_ps(_mm256_add_pd(_mm256_mul_pd(v1_0, v2_0), v3_0)),
                  _mm256_cvtpd_ps(_mm256_add_pd(_mm256_mul_pd(v1_1, v2_1), v3_1)));
#endif
}
Vc_INTRINSIC __m256d fma(__m256d a, __m256d b, __m256d c, double)
{
#ifdef Vc_IMPL_FMA4
    return _mm256_macc_pd(a, b, c);
#elif defined Vc_IMPL_FMA
    return _mm256_fmadd_pd(a, b, c);
#else
    using namespace AVX;
    __m256d h1 = and_(a, _mm256_broadcast_sd(reinterpret_cast<const double *>(
                             &c_general::highMaskDouble)));
    __m256d h2 = and_(b, _mm256_broadcast_sd(reinterpret_cast<const double *>(
                             &c_general::highMaskDouble)));
    const __m256d l1 = _mm256_sub_pd(a, h1);
    const __m256d l2 = _mm256_sub_pd(b, h2);
    const __m256d ll = mul(l1, l2, double());
    const __m256d lh = add(mul(l1, h2, double()), mul(h1, l2, double()), double());
    const __m256d hh = mul(h1, h2, double());
    // ll < lh < hh for all entries is certain
    const __m256d lh_lt_v3 = cmplt(abs(lh, double()), abs(c, double()), double());  // |lh| < |c|
    const __m256d x = _mm256_blendv_pd(c, lh, lh_lt_v3);
    const __m256d y = _mm256_blendv_pd(lh, c, lh_lt_v3);
    return add(add(ll, x, double()), add(y, hh, double()), double());
#endif
}
template <typename T> Vc_INTRINSIC __m256i fma(__m256i a, __m256i b, __m256i c, T)
{
    return add(mul(a, b, T()), c, T());
}

// shiftRight{{{1
template <int shift> Vc_INTRINSIC __m256i shiftRight(__m256i a,    int) { return AVX::srai_epi32<shift>(a); }
template <int shift> Vc_INTRINSIC __m256i shiftRight(__m256i a,   uint) { return AVX::srli_epi32<shift>(a); }
template <int shift> Vc_INTRINSIC __m256i shiftRight(__m256i a,  short) { return AVX::srai_epi16<shift>(a); }
template <int shift> Vc_INTRINSIC __m256i shiftRight(__m256i a, ushort) { return AVX::srli_epi16<shift>(a); }
//template <int shift> Vc_INTRINSIC __m256i shiftRight(__m256i a,  schar) { return AVX::srai_epi8 <shift>(a); }
//template <int shift> Vc_INTRINSIC __m256i shiftRight(__m256i a,  uchar) { return AVX::srli_epi8 <shift>(a); }

Vc_INTRINSIC __m256i shiftRight(__m256i a, int shift,    int) { return AVX::sra_epi32(a, _mm_cvtsi32_si128(shift)); }
Vc_INTRINSIC __m256i shiftRight(__m256i a, int shift,   uint) { return AVX::srl_epi32(a, _mm_cvtsi32_si128(shift)); }
Vc_INTRINSIC __m256i shiftRight(__m256i a, int shift,  short) { return AVX::sra_epi16(a, _mm_cvtsi32_si128(shift)); }
Vc_INTRINSIC __m256i shiftRight(__m256i a, int shift, ushort) { return AVX::srl_epi16(a, _mm_cvtsi32_si128(shift)); }
//Vc_INTRINSIC __m256i shiftRight(__m256i a, int shift,  schar) { return AVX::sra_epi8 (a, _mm_cvtsi32_si128(shift)); }
//Vc_INTRINSIC __m256i shiftRight(__m256i a, int shift,  uchar) { return AVX::srl_epi8 (a, _mm_cvtsi32_si128(shift)); }

// shiftLeft{{{1
template <int shift> Vc_INTRINSIC __m256i shiftLeft(__m256i a,    int) { return AVX::slli_epi32<shift>(a); }
template <int shift> Vc_INTRINSIC __m256i shiftLeft(__m256i a,   uint) { return AVX::slli_epi32<shift>(a); }
template <int shift> Vc_INTRINSIC __m256i shiftLeft(__m256i a,  short) { return AVX::slli_epi16<shift>(a); }
template <int shift> Vc_INTRINSIC __m256i shiftLeft(__m256i a, ushort) { return AVX::slli_epi16<shift>(a); }
//template <int shift> Vc_INTRINSIC __m256i shiftLeft(__m256i a,  schar) { return AVX::slli_epi8 <shift>(a); }
//template <int shift> Vc_INTRINSIC __m256i shiftLeft(__m256i a,  uchar) { return AVX::slli_epi8 <shift>(a); }

Vc_INTRINSIC __m256i shiftLeft(__m256i a, int shift,    int) { return AVX::sll_epi32(a, _mm_cvtsi32_si128(shift)); }
Vc_INTRINSIC __m256i shiftLeft(__m256i a, int shift,   uint) { return AVX::sll_epi32(a, _mm_cvtsi32_si128(shift)); }
Vc_INTRINSIC __m256i shiftLeft(__m256i a, int shift,  short) { return AVX::sll_epi16(a, _mm_cvtsi32_si128(shift)); }
Vc_INTRINSIC __m256i shiftLeft(__m256i a, int shift, ushort) { return AVX::sll_epi16(a, _mm_cvtsi32_si128(shift)); }
//Vc_INTRINSIC __m256i shiftLeft(__m256i a, int shift,  schar) { return AVX::sll_epi8 (a, _mm_cvtsi32_si128(shift)); }
//Vc_INTRINSIC __m256i shiftLeft(__m256i a, int shift,  uchar) { return AVX::sll_epi8 (a, _mm_cvtsi32_si128(shift)); }

// zeroExtendIfNeeded{{{1
Vc_INTRINSIC __m256  zeroExtendIfNeeded(__m256  x) { return x; }
Vc_INTRINSIC __m256d zeroExtendIfNeeded(__m256d x) { return x; }
Vc_INTRINSIC __m256i zeroExtendIfNeeded(__m256i x) { return x; }
Vc_INTRINSIC __m256  zeroExtendIfNeeded(__m128  x) { return AVX::zeroExtend(x); }
Vc_INTRINSIC __m256d zeroExtendIfNeeded(__m128d x) { return AVX::zeroExtend(x); }
Vc_INTRINSIC __m256i zeroExtendIfNeeded(__m128i x) { return AVX::zeroExtend(x); }

// broadcast{{{1
Vc_INTRINSIC __m256  avx_broadcast( float x) { return _mm256_set1_ps(x); }
Vc_INTRINSIC __m256d avx_broadcast(double x) { return _mm256_set1_pd(x); }
Vc_INTRINSIC __m256i avx_broadcast(   int x) { return _mm256_set1_epi32(x); }
Vc_INTRINSIC __m256i avx_broadcast(  uint x) { return _mm256_set1_epi32(x); }
Vc_INTRINSIC __m256i avx_broadcast( short x) { return _mm256_set1_epi16(x); }
Vc_INTRINSIC __m256i avx_broadcast(ushort x) { return _mm256_set1_epi16(x); }
Vc_INTRINSIC __m256i avx_broadcast(  char x) { return _mm256_set1_epi8(x); }
Vc_INTRINSIC __m256i avx_broadcast( schar x) { return _mm256_set1_epi8(x); }
Vc_INTRINSIC __m256i avx_broadcast( uchar x) { return _mm256_set1_epi8(x); }

// sorted{{{1
template <Vc::Implementation Impl, typename T,
          typename = enable_if<(Impl >= AVXImpl && Impl <= AVX2Impl)>>
Vc_CONST_L AVX2::Vector<T> Vc_VDECL sorted(AVX2::Vector<T> x) Vc_CONST_R;
template <typename T> Vc_INTRINSIC Vc_CONST AVX2::Vector<T> sorted(AVX2::Vector<T> x)
{
    return sorted<CurrentImplementation::current()>(x);
}

// shifted{{{1
template <typename T, typename V>
static Vc_INTRINSIC Vc_CONST enable_if<(sizeof(V) == 32), V> shifted(V v, int amount)
{
    using namespace AVX;
    constexpr int S = sizeof(T);
    switch (amount) {
    case  0: return v;
    case  1: return shifted<sanitize<V>( 1 * S)>(v);
    case  2: return shifted<sanitize<V>( 2 * S)>(v);
    case  3: return shifted<sanitize<V>( 3 * S)>(v);
    case -1: return shifted<sanitize<V>(-1 * S)>(v);
    case -2: return shifted<sanitize<V>(-2 * S)>(v);
    case -3: return shifted<sanitize<V>(-3 * S)>(v);
    }
    if (sizeof(T) <= 4) {
        switch (amount) {
        case  4: return shifted<sanitize<V>( 4 * S)>(v);
        case  5: return shifted<sanitize<V>( 5 * S)>(v);
        case  6: return shifted<sanitize<V>( 6 * S)>(v);
        case  7: return shifted<sanitize<V>( 7 * S)>(v);
        case -4: return shifted<sanitize<V>(-4 * S)>(v);
        case -5: return shifted<sanitize<V>(-5 * S)>(v);
        case -6: return shifted<sanitize<V>(-6 * S)>(v);
        case -7: return shifted<sanitize<V>(-7 * S)>(v);
        }
        if (sizeof(T) <= 2) {
            switch (amount) {
            case   8: return shifted<sanitize<V>(  8 * S)>(v);
            case   9: return shifted<sanitize<V>(  9 * S)>(v);
            case  10: return shifted<sanitize<V>( 10 * S)>(v);
            case  11: return shifted<sanitize<V>( 11 * S)>(v);
            case  12: return shifted<sanitize<V>( 12 * S)>(v);
            case  13: return shifted<sanitize<V>( 13 * S)>(v);
            case  14: return shifted<sanitize<V>( 14 * S)>(v);
            case  15: return shifted<sanitize<V>( 15 * S)>(v);
            case  -8: return shifted<sanitize<V>(- 8 * S)>(v);
            case  -9: return shifted<sanitize<V>(- 9 * S)>(v);
            case -10: return shifted<sanitize<V>(-10 * S)>(v);
            case -11: return shifted<sanitize<V>(-11 * S)>(v);
            case -12: return shifted<sanitize<V>(-12 * S)>(v);
            case -13: return shifted<sanitize<V>(-13 * S)>(v);
            case -14: return shifted<sanitize<V>(-14 * S)>(v);
            case -15: return shifted<sanitize<V>(-15 * S)>(v);
            }
            if (sizeof(T) == 1) {
                switch (amount) {
                case  16: return shifted<sanitize<V>( 16)>(v);
                case  17: return shifted<sanitize<V>( 17)>(v);
                case  18: return shifted<sanitize<V>( 18)>(v);
                case  19: return shifted<sanitize<V>( 19)>(v);
                case  20: return shifted<sanitize<V>( 20)>(v);
                case  21: return shifted<sanitize<V>( 21)>(v);
                case  22: return shifted<sanitize<V>( 22)>(v);
                case  23: return shifted<sanitize<V>( 23)>(v);
                case  24: return shifted<sanitize<V>( 24)>(v);
                case  25: return shifted<sanitize<V>( 25)>(v);
                case  26: return shifted<sanitize<V>( 26)>(v);
                case  27: return shifted<sanitize<V>( 27)>(v);
                case  28: return shifted<sanitize<V>( 28)>(v);
                case  29: return shifted<sanitize<V>( 29)>(v);
                case  30: return shifted<sanitize<V>( 30)>(v);
                case  31: return shifted<sanitize<V>( 31)>(v);
                case -16: return shifted<sanitize<V>(-16)>(v);
                case -17: return shifted<sanitize<V>(-17)>(v);
                case -18: return shifted<sanitize<V>(-18)>(v);
                case -19: return shifted<sanitize<V>(-19)>(v);
                case -20: return shifted<sanitize<V>(-20)>(v);
                case -21: return shifted<sanitize<V>(-21)>(v);
                case -22: return shifted<sanitize<V>(-22)>(v);
                case -23: return shifted<sanitize<V>(-23)>(v);
                case -24: return shifted<sanitize<V>(-24)>(v);
                case -25: return shifted<sanitize<V>(-25)>(v);
                case -26: return shifted<sanitize<V>(-26)>(v);
                case -27: return shifted<sanitize<V>(-27)>(v);
                case -28: return shifted<sanitize<V>(-28)>(v);
                case -29: return shifted<sanitize<V>(-29)>(v);
                case -30: return shifted<sanitize<V>(-30)>(v);
                case -31: return shifted<sanitize<V>(-31)>(v);
                }
            }
        }
    }
    return avx_cast<V>(_mm256_setzero_ps());
}

template <typename T, typename V>
static Vc_INTRINSIC Vc_CONST enable_if<(sizeof(V) == 16), V> shifted(V v, int amount)
{
    using namespace AVX;
    switch (amount) {
    case  0: return v;
    case  1: return avx_cast<V>(_mm_srli_si128(avx_cast<__m128i>(v), sanitize<V>(1 * sizeof(T))));
    case  2: return avx_cast<V>(_mm_srli_si128(avx_cast<__m128i>(v), sanitize<V>(2 * sizeof(T))));
    case  3: return avx_cast<V>(_mm_srli_si128(avx_cast<__m128i>(v), sanitize<V>(3 * sizeof(T))));
    case -1: return avx_cast<V>(_mm_slli_si128(avx_cast<__m128i>(v), sanitize<V>(1 * sizeof(T))));
    case -2: return avx_cast<V>(_mm_slli_si128(avx_cast<__m128i>(v), sanitize<V>(2 * sizeof(T))));
    case -3: return avx_cast<V>(_mm_slli_si128(avx_cast<__m128i>(v), sanitize<V>(3 * sizeof(T))));
    }
    if (sizeof(T) <= 2) {
        switch (amount) {
        case  4: return avx_cast<V>(_mm_srli_si128(avx_cast<__m128i>(v), sanitize<V>(4 * sizeof(T))));
        case  5: return avx_cast<V>(_mm_srli_si128(avx_cast<__m128i>(v), sanitize<V>(5 * sizeof(T))));
        case  6: return avx_cast<V>(_mm_srli_si128(avx_cast<__m128i>(v), sanitize<V>(6 * sizeof(T))));
        case  7: return avx_cast<V>(_mm_srli_si128(avx_cast<__m128i>(v), sanitize<V>(7 * sizeof(T))));
        case -4: return avx_cast<V>(_mm_slli_si128(avx_cast<__m128i>(v), sanitize<V>(4 * sizeof(T))));
        case -5: return avx_cast<V>(_mm_slli_si128(avx_cast<__m128i>(v), sanitize<V>(5 * sizeof(T))));
        case -6: return avx_cast<V>(_mm_slli_si128(avx_cast<__m128i>(v), sanitize<V>(6 * sizeof(T))));
        case -7: return avx_cast<V>(_mm_slli_si128(avx_cast<__m128i>(v), sanitize<V>(7 * sizeof(T))));
        }
    }
    return avx_cast<V>(_mm_setzero_ps());
}
// rotated{{{1
template <typename T, size_t N, typename V>
static Vc_INTRINSIC Vc_CONST enable_if<(sizeof(V) == 32 && N == 4), V> rotated(V v,
                                                                               int amount)
{
    using namespace AVX;
    const __m128i vLo = avx_cast<__m128i>(lo128(v));
    const __m128i vHi = avx_cast<__m128i>(hi128(v));
    switch (static_cast<unsigned int>(amount) % N) {
    case 0:
        return v;
    case 1:
        return avx_cast<V>(concat(SSE::alignr_epi8<sizeof(T)>(vHi, vLo),
                                  SSE::alignr_epi8<sizeof(T)>(vLo, vHi)));
    case 2:
        return Mem::permute128<X1, X0>(v);
    case 3:
        return avx_cast<V>(concat(SSE::alignr_epi8<sizeof(T)>(vLo, vHi),
                                  SSE::alignr_epi8<sizeof(T)>(vHi, vLo)));
    }
    return avx_cast<V>(_mm256_setzero_ps());
}

template <typename T, size_t N, typename V>
static Vc_INTRINSIC Vc_CONST enable_if<(sizeof(V) == 32 && N == 8), V> rotated(V v,
                                                                               int amount)
{
    using namespace AVX;
    const __m128i vLo = avx_cast<__m128i>(lo128(v));
    const __m128i vHi = avx_cast<__m128i>(hi128(v));
    switch (static_cast<unsigned int>(amount) % N) {
    case 0:
        return v;
    case 1:
        return avx_cast<V>(concat(SSE::alignr_epi8<1 * sizeof(T)>(vHi, vLo),
                                  SSE::alignr_epi8<1 * sizeof(T)>(vLo, vHi)));
    case 2:
        return avx_cast<V>(concat(SSE::alignr_epi8<2 * sizeof(T)>(vHi, vLo),
                                  SSE::alignr_epi8<2 * sizeof(T)>(vLo, vHi)));
    case 3:
        return avx_cast<V>(concat(SSE::alignr_epi8<3 * sizeof(T)>(vHi, vLo),
                                  SSE::alignr_epi8<3 * sizeof(T)>(vLo, vHi)));
    case 4:
        return Mem::permute128<X1, X0>(v);
    case 5:
        return avx_cast<V>(concat(SSE::alignr_epi8<1 * sizeof(T)>(vLo, vHi),
                                  SSE::alignr_epi8<1 * sizeof(T)>(vHi, vLo)));
    case 6:
        return avx_cast<V>(concat(SSE::alignr_epi8<2 * sizeof(T)>(vLo, vHi),
                                  SSE::alignr_epi8<2 * sizeof(T)>(vHi, vLo)));
    case 7:
        return avx_cast<V>(concat(SSE::alignr_epi8<3 * sizeof(T)>(vLo, vHi),
                                  SSE::alignr_epi8<3 * sizeof(T)>(vHi, vLo)));
    }
    return avx_cast<V>(_mm256_setzero_ps());
}

#ifdef Vc_IMPL_AVX2
template <typename T, size_t N, typename V>
static Vc_INTRINSIC Vc_CONST enable_if<(sizeof(V) == 32 && N == 16), V> rotated(
    V v, int amount)
{
    using namespace AVX;
    const __m128i vLo = avx_cast<__m128i>(lo128(v));
    const __m128i vHi = avx_cast<__m128i>(hi128(v));
    switch (static_cast<unsigned int>(amount) % N) {
    case 0:
        return v;
    case 1:
        return avx_cast<V>(concat(SSE::alignr_epi8<1 * sizeof(T)>(vHi, vLo),
                                  SSE::alignr_epi8<1 * sizeof(T)>(vLo, vHi)));
    case 2:
        return avx_cast<V>(concat(SSE::alignr_epi8<2 * sizeof(T)>(vHi, vLo),
                                  SSE::alignr_epi8<2 * sizeof(T)>(vLo, vHi)));
    case 3:
        return avx_cast<V>(concat(SSE::alignr_epi8<3 * sizeof(T)>(vHi, vLo),
                                  SSE::alignr_epi8<3 * sizeof(T)>(vLo, vHi)));
    case 4:
        return Mem::permute4x64<X1, X2, X3, X0>(v);
    case 5:
        return avx_cast<V>(concat(SSE::alignr_epi8<5 * sizeof(T)>(vHi, vLo),
                                  SSE::alignr_epi8<5 * sizeof(T)>(vLo, vHi)));
    case 6:
        return avx_cast<V>(concat(SSE::alignr_epi8<6 * sizeof(T)>(vHi, vLo),
                                  SSE::alignr_epi8<6 * sizeof(T)>(vLo, vHi)));
    case 7:
        return avx_cast<V>(concat(SSE::alignr_epi8<7 * sizeof(T)>(vHi, vLo),
                                  SSE::alignr_epi8<7 * sizeof(T)>(vLo, vHi)));
    case 8:
        return Mem::permute128<X1, X0>(v);
    case 9:
        return avx_cast<V>(concat(SSE::alignr_epi8<1 * sizeof(T)>(vLo, vHi),
                                  SSE::alignr_epi8<1 * sizeof(T)>(vHi, vLo)));
    case 10:
        return avx_cast<V>(concat(SSE::alignr_epi8<2 * sizeof(T)>(vLo, vHi),
                                  SSE::alignr_epi8<2 * sizeof(T)>(vHi, vLo)));
    case 11:
        return avx_cast<V>(concat(SSE::alignr_epi8<3 * sizeof(T)>(vLo, vHi),
                                  SSE::alignr_epi8<3 * sizeof(T)>(vHi, vLo)));
    case 12:
        return Mem::permute4x64<X3, X0, X1, X2>(v);
    case 13:
        return avx_cast<V>(concat(SSE::alignr_epi8<5 * sizeof(T)>(vLo, vHi),
                                  SSE::alignr_epi8<5 * sizeof(T)>(vHi, vLo)));
    case 14:
        return avx_cast<V>(concat(SSE::alignr_epi8<6 * sizeof(T)>(vLo, vHi),
                                  SSE::alignr_epi8<6 * sizeof(T)>(vHi, vLo)));
    case 15:
        return avx_cast<V>(concat(SSE::alignr_epi8<7 * sizeof(T)>(vLo, vHi),
                                  SSE::alignr_epi8<7 * sizeof(T)>(vHi, vLo)));
    }
    return avx_cast<V>(_mm256_setzero_ps());
}
#endif  // Vc_IMPL_AVX2

// testc{{{1
Vc_INTRINSIC Vc_CONST int testc(__m128  a, __m128  b) { return _mm_testc_si128(_mm_castps_si128(a), _mm_castps_si128(b)); }
Vc_INTRINSIC Vc_CONST int testc(__m256  a, __m256  b) { return _mm256_testc_ps(a, b); }
Vc_INTRINSIC Vc_CONST int testc(__m256d a, __m256d b) { return _mm256_testc_pd(a, b); }
Vc_INTRINSIC Vc_CONST int testc(__m256i a, __m256i b) { return _mm256_testc_si256(a, b); }

// testz{{{1
Vc_INTRINSIC Vc_CONST int testz(__m128  a, __m128  b) { return _mm_testz_si128(_mm_castps_si128(a), _mm_castps_si128(b)); }
Vc_INTRINSIC Vc_CONST int testz(__m256  a, __m256  b) { return _mm256_testz_ps(a, b); }
Vc_INTRINSIC Vc_CONST int testz(__m256d a, __m256d b) { return _mm256_testz_pd(a, b); }
Vc_INTRINSIC Vc_CONST int testz(__m256i a, __m256i b) { return _mm256_testz_si256(a, b); }

// testnzc{{{1
Vc_INTRINSIC Vc_CONST int testnzc(__m128 a, __m128 b) { return _mm_testnzc_si128(_mm_castps_si128(a), _mm_castps_si128(b)); }
Vc_INTRINSIC Vc_CONST int testnzc(__m256  a, __m256  b) { return _mm256_testnzc_ps(a, b); }
Vc_INTRINSIC Vc_CONST int testnzc(__m256d a, __m256d b) { return _mm256_testnzc_pd(a, b); }
Vc_INTRINSIC Vc_CONST int testnzc(__m256i a, __m256i b) { return _mm256_testnzc_si256(a, b); }

// movemask{{{1
Vc_INTRINSIC Vc_CONST int movemask(__m256i a) { return AVX::movemask_epi8(a); }
Vc_INTRINSIC Vc_CONST int movemask(__m128i a) { return _mm_movemask_epi8(a); }
Vc_INTRINSIC Vc_CONST int movemask(__m256d a) { return _mm256_movemask_pd(a); }
Vc_INTRINSIC Vc_CONST int movemask(__m128d a) { return _mm_movemask_pd(a); }
Vc_INTRINSIC Vc_CONST int movemask(__m256  a) { return _mm256_movemask_ps(a); }
Vc_INTRINSIC Vc_CONST int movemask(__m128  a) { return _mm_movemask_ps(a); }

// mask_store{{{1
template <size_t N, typename Flags>
Vc_INTRINSIC void mask_store(__m256i k, bool *mem, Flags)
{
    static_assert(
        N == 4 || N == 8 || N == 16,
        "mask_store(__m256i, bool *) is only implemented for 4, 8, and 16 entries");
    switch (N) {
    case 4:
        *aliasing_cast<int32_t>(mem) = (_mm_movemask_epi8(AVX::lo128(k)) |
                                        (_mm_movemask_epi8(AVX::hi128(k)) << 16)) &
                                       0x01010101;
        break;
    case 8: {
        const auto k2 = _mm_srli_epi16(_mm_packs_epi16(AVX::lo128(k), AVX::hi128(k)), 15);
        const auto k3 = _mm_packs_epi16(k2, _mm_setzero_si128());
#ifdef __x86_64__
        *aliasing_cast<int64_t>(mem) = _mm_cvtsi128_si64(k3);
#else
        *aliasing_cast<int32_t>(mem) = _mm_cvtsi128_si32(k3);
        *aliasing_cast<int32_t>(mem + 4) = _mm_extract_epi32(k3, 1);
#endif
    } break;
    case 16: {
        const auto bools = Detail::and_(_mm_set1_epi8(1),
                                        _mm_packs_epi16(AVX::lo128(k), AVX::hi128(k)));
        if (Flags::IsAligned) {
            _mm_store_si128(reinterpret_cast<__m128i *>(mem), bools);
        } else {
            _mm_storeu_si128(reinterpret_cast<__m128i *>(mem), bools);
        }
    } break;
    default:
        Vc_UNREACHABLE();
    }
}

// mask_load{{{1
template <typename R, size_t N, typename Flags>
Vc_INTRINSIC R mask_load(const bool *mem, Flags,
                         enable_if<std::is_same<R, __m128>::value> = nullarg)
{
    static_assert(N == 4 || N == 8,
                  "mask_load<__m128>(const bool *) is only implemented for 4, 8 entries");
    switch (N) {
    case 4: {
        __m128i k = _mm_cvtsi32_si128(*aliasing_cast<int32_t>(mem));
        k = _mm_unpacklo_epi8(k, k);
        k = _mm_unpacklo_epi16(k, k);
        k = _mm_cmpgt_epi32(k, _mm_setzero_si128());
        return AVX::avx_cast<__m128>(k);
    }
    case 8: {
#ifdef __x86_64__
        __m128i k = _mm_cvtsi64_si128(*aliasing_cast<int64_t>(mem));
#else
        __m128i k = _mm_castpd_si128(_mm_load_sd(aliasing_cast<double>(mem)));
#endif
        return AVX::avx_cast<__m128>(
            _mm_cmpgt_epi16(_mm_unpacklo_epi8(k, k), _mm_setzero_si128()));
    }
    default:
        Vc_UNREACHABLE();
    }
}

template <typename R, size_t N, typename Flags>
Vc_INTRINSIC R mask_load(const bool *mem, Flags,
                         enable_if<std::is_same<R, __m256>::value> = nullarg)
{
    static_assert(
        N == 4 || N == 8 || N == 16,
        "mask_load<__m256>(const bool *) is only implemented for 4, 8, and 16 entries");
    switch (N) {
    case 4: {
        __m128i k = AVX::avx_cast<__m128i>(_mm_and_ps(
            _mm_set1_ps(*aliasing_cast<float>(mem)),
            AVX::avx_cast<__m128>(_mm_setr_epi32(0x1, 0x100, 0x10000, 0x1000000))));
        k = _mm_cmpgt_epi32(k, _mm_setzero_si128());
        return AVX::avx_cast<__m256>(
            AVX::concat(_mm_unpacklo_epi32(k, k), _mm_unpackhi_epi32(k, k)));
    }
    case 8: {
#ifdef __x86_64__
        __m128i k = _mm_cvtsi64_si128(*aliasing_cast<int64_t>(mem));
#else
        __m128i k = _mm_castpd_si128(_mm_load_sd(aliasing_cast<double>(mem)));
#endif
        k = _mm_cmpgt_epi16(_mm_unpacklo_epi8(k, k), _mm_setzero_si128());
        return AVX::avx_cast<__m256>(
            AVX::concat(_mm_unpacklo_epi16(k, k), _mm_unpackhi_epi16(k, k)));
    }
    case 16: {
        const auto k128 = _mm_cmpgt_epi8(
            Flags::IsAligned ? _mm_load_si128(reinterpret_cast<const __m128i *>(mem))
                             : _mm_loadu_si128(reinterpret_cast<const __m128i *>(mem)),
            _mm_setzero_si128());
        return AVX::avx_cast<__m256>(
            AVX::concat(_mm_unpacklo_epi8(k128, k128), _mm_unpackhi_epi8(k128, k128)));
    }
    default:
        Vc_UNREACHABLE();
        return R();
    }
}

// mask_to_int{{{1
template <size_t Size>
Vc_INTRINSIC_L Vc_CONST_L int mask_to_int(__m256i x) Vc_INTRINSIC_R Vc_CONST_R;
template <> Vc_INTRINSIC Vc_CONST int mask_to_int<4>(__m256i k)
{
    return movemask(AVX::avx_cast<__m256d>(k));
}
template <> Vc_INTRINSIC Vc_CONST int mask_to_int<8>(__m256i k)
{
    return movemask(AVX::avx_cast<__m256>(k));
}
#ifdef Vc_IMPL_BMI2
template <> Vc_INTRINSIC Vc_CONST int mask_to_int<16>(__m256i k)
{
    return _pext_u32(movemask(k), 0x55555555u);
}
#endif
template <> Vc_INTRINSIC Vc_CONST int mask_to_int<32>(__m256i k)
{
    return movemask(k);
}

//InterleaveImpl{{{1
template<typename V> struct InterleaveImpl<V, 16, 32> {
    template<typename I> static inline void interleave(typename V::EntryType *const data, const I &i,/*{{{*/
            const typename V::AsArg v0, // a0 a1 a2 a3 a4 a5 a6 a7 | a8 a9 ...
            const typename V::AsArg v1) // b0 b1 b2 b3 b4 b5 b6 b7 | b8 b9 ...
    {
        const __m256i tmp0 = AVX::unpacklo_epi16(v0.data(), v1.data()); // a0 b0 a1 b1 a2 b2 a3 b3 | a8 b8 a9 ...
        const __m256i tmp1 = AVX::unpackhi_epi16(v0.data(), v1.data()); // a4 b4 a5 ...
        using namespace AVX;
        *aliasing_cast<uint32_t>(&data[i[ 0]]) = _mm_cvtsi128_si32(lo128(tmp0));
        *aliasing_cast<uint32_t>(&data[i[ 1]]) = _mm_extract_epi32(lo128(tmp0), 1);
        *aliasing_cast<uint32_t>(&data[i[ 2]]) = _mm_extract_epi32(lo128(tmp0), 2);
        *aliasing_cast<uint32_t>(&data[i[ 3]]) = _mm_extract_epi32(lo128(tmp0), 3);
        *aliasing_cast<uint32_t>(&data[i[ 4]]) = _mm_cvtsi128_si32(lo128(tmp1));
        *aliasing_cast<uint32_t>(&data[i[ 5]]) = _mm_extract_epi32(lo128(tmp1), 1);
        *aliasing_cast<uint32_t>(&data[i[ 6]]) = _mm_extract_epi32(lo128(tmp1), 2);
        *aliasing_cast<uint32_t>(&data[i[ 7]]) = _mm_extract_epi32(lo128(tmp1), 3);
        *aliasing_cast<uint32_t>(&data[i[ 8]]) = _mm_cvtsi128_si32(hi128(tmp0));
        *aliasing_cast<uint32_t>(&data[i[ 9]]) = _mm_extract_epi32(hi128(tmp0), 1);
        *aliasing_cast<uint32_t>(&data[i[10]]) = _mm_extract_epi32(hi128(tmp0), 2);
        *aliasing_cast<uint32_t>(&data[i[11]]) = _mm_extract_epi32(hi128(tmp0), 3);
        *aliasing_cast<uint32_t>(&data[i[12]]) = _mm_cvtsi128_si32(hi128(tmp1));
        *aliasing_cast<uint32_t>(&data[i[13]]) = _mm_extract_epi32(hi128(tmp1), 1);
        *aliasing_cast<uint32_t>(&data[i[14]]) = _mm_extract_epi32(hi128(tmp1), 2);
        *aliasing_cast<uint32_t>(&data[i[15]]) = _mm_extract_epi32(hi128(tmp1), 3);
    }/*}}}*/
    static inline void interleave(typename V::EntryType *const data, const Common::SuccessiveEntries<2> &i,/*{{{*/
            const typename V::AsArg v0, const typename V::AsArg v1)
    {
        const __m256i tmp0 = AVX::unpacklo_epi16(v0.data(), v1.data()); // a0 b0 a1 b1 a2 b2 a3 b3 | a8 b8 a9 ...
        const __m256i tmp1 = AVX::unpackhi_epi16(v0.data(), v1.data()); // a4 b4 a5 ...
        V(Mem::shuffle128<X0, Y0>(tmp0, tmp1)).store(&data[i[0]], Vc::Unaligned);
        V(Mem::shuffle128<X1, Y1>(tmp0, tmp1)).store(&data[i[8]], Vc::Unaligned);
    }/*}}}*/
    template<typename I> static inline void interleave(typename V::EntryType *const data, const I &i,/*{{{*/
            const typename V::AsArg v0, const typename V::AsArg v1, const typename V::AsArg v2)
    {
        interleave(data, i, v0, v1);
        v2.scatter(data + 2, i);
    }/*}}}*/
    template<typename I> static inline void interleave(typename V::EntryType *const data, const I &i,/*{{{*/
            const typename V::AsArg v0, const typename V::AsArg v1,
            const typename V::AsArg v2, const typename V::AsArg v3)
    {
        const __m256i tmp0 = AVX::unpacklo_epi16(v0.data(), v2.data()); // a0 c0 a1 c1 a2 c2 a3 c3 | a8 c8 a9 c9 ...
        const __m256i tmp1 = AVX::unpackhi_epi16(v0.data(), v2.data()); // a4 c4 a5 c5 a6 c6 a7 c7 | a12 c12 ...
        const __m256i tmp2 = AVX::unpacklo_epi16(v1.data(), v3.data()); // b0 d0 b1 d1 b2 d2 b3 d3 | b8 d8 b9 d9 ...
        const __m256i tmp3 = AVX::unpackhi_epi16(v1.data(), v3.data()); // b4 d4 b5 ...

        const __m256i tmp4 = AVX::unpacklo_epi16(tmp0, tmp2); // a0 b0 c0 d0 a1 b1 c1 d1 | a8 b8 c8 d8 a9 b9 ...
        const __m256i tmp5 = AVX::unpackhi_epi16(tmp0, tmp2); // [abcd]2 [abcd]3 | [abcd]10 [abcd]11
        const __m256i tmp6 = AVX::unpacklo_epi16(tmp1, tmp3); // [abcd]4 [abcd]5 | [abcd]12 [abcd]13
        const __m256i tmp7 = AVX::unpackhi_epi16(tmp1, tmp3); // [abcd]6 [abcd]7 | [abcd]14 [abcd]15

        using namespace AVX;
        auto &&store = [&](__m256i x, int offset) {
            _mm_storel_epi64(reinterpret_cast<__m128i *>(&data[i[offset + 0]]), lo128(x));
            _mm_storel_epi64(reinterpret_cast<__m128i *>(&data[i[offset + 8]]), hi128(x));
            _mm_storeh_pi(reinterpret_cast<__m64 *>(&data[i[offset + 1]]), avx_cast<__m128>(x));
            _mm_storeh_pi(reinterpret_cast<__m64 *>(&data[i[offset + 9]]), avx_cast<__m128>(hi128(x)));
        };
        store(tmp4, 0);
        store(tmp5, 2);
        store(tmp6, 4);
        store(tmp7, 6);
    }/*}}}*/
    static inline void interleave(typename V::EntryType *const data, const Common::SuccessiveEntries<4> &i,/*{{{*/
            const typename V::AsArg v0, const typename V::AsArg v1,
            const typename V::AsArg v2, const typename V::AsArg v3)
    {
        const __m256i tmp0 = AVX::unpacklo_epi16(v0.data(), v2.data()); // a0 c0 a1 c1 a2 c2 a3 c3 | a8 c8 a9 c9 ...
        const __m256i tmp1 = AVX::unpackhi_epi16(v0.data(), v2.data()); // a4 c4 a5 c5 a6 c6 a7 c7 | a12 c12 ...
        const __m256i tmp2 = AVX::unpacklo_epi16(v1.data(), v3.data()); // b0 d0 b1 d1 b2 d2 b3 d3 | b8 d8 b9 d9 ...
        const __m256i tmp3 = AVX::unpackhi_epi16(v1.data(), v3.data()); // b4 d4 b5 ...

        const __m256i tmp4 = AVX::unpacklo_epi16(tmp0, tmp2); // a0 b0 c0 d0 a1 b1 c1 d1 | a8 b8 c8 d8 a9 b9 ...
        const __m256i tmp5 = AVX::unpackhi_epi16(tmp0, tmp2); // [abcd]2 [abcd]3 | [abcd]10 [abcd]11
        const __m256i tmp6 = AVX::unpacklo_epi16(tmp1, tmp3); // [abcd]4 [abcd]5 | [abcd]12 [abcd]13
        const __m256i tmp7 = AVX::unpackhi_epi16(tmp1, tmp3); // [abcd]6 [abcd]7 | [abcd]14 [abcd]15

        V(Mem::shuffle128<X0, Y0>(tmp4, tmp5)).store(&data[i[0]], ::Vc::Unaligned);
        V(Mem::shuffle128<X0, Y0>(tmp6, tmp7)).store(&data[i[4]], ::Vc::Unaligned);
        V(Mem::shuffle128<X1, Y1>(tmp4, tmp5)).store(&data[i[8]], ::Vc::Unaligned);
        V(Mem::shuffle128<X1, Y1>(tmp6, tmp7)).store(&data[i[12]], ::Vc::Unaligned);
    }/*}}}*/
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
        const __m256i tmp4 =  // a0 b0 a1 b1 a2 b2 a3 b3 | a8 b8 a9 b9 a10 b10 a11 b11
            _mm256_setr_epi32(
                *aliasing_cast<int>(&data[i[0]]), *aliasing_cast<int>(&data[i[1]]),
                *aliasing_cast<int>(&data[i[2]]), *aliasing_cast<int>(&data[i[3]]),
                *aliasing_cast<int>(&data[i[8]]), *aliasing_cast<int>(&data[i[9]]),
                *aliasing_cast<int>(&data[i[10]]), *aliasing_cast<int>(&data[i[11]]));
        const __m256i tmp5 =  // a4 b4 a5 b5 a6 b6 a7 b7 | a12 b12 a13 b13 a14 b14 a15 b15
            _mm256_setr_epi32(
                *aliasing_cast<int>(&data[i[4]]), *aliasing_cast<int>(&data[i[5]]),
                *aliasing_cast<int>(&data[i[6]]), *aliasing_cast<int>(&data[i[7]]),
                *aliasing_cast<int>(&data[i[12]]), *aliasing_cast<int>(&data[i[13]]),
                *aliasing_cast<int>(&data[i[14]]), *aliasing_cast<int>(&data[i[15]]));

        const __m256i tmp2 = AVX::unpacklo_epi16(tmp4, tmp5);  // a0 a4 b0 b4 a1 a5 b1 b5 | a8 a12 b8 b12 a9 a13 b9 b13
        const __m256i tmp3 = AVX::unpackhi_epi16(tmp4, tmp5);  // a2 a6 b2 b6 a3 a7 b3 b7 | a10 a14 b10 b14 a11 a15 b11 b15

        const __m256i tmp0 = AVX::unpacklo_epi16(tmp2, tmp3);  // a0 a2 a4 a6 b0 b2 b4 b6 | a8 a10 a12 a14 b8 ...
        const __m256i tmp1 = AVX::unpackhi_epi16(tmp2, tmp3);  // a1 a3 a5 a7 b1 b3 b5 b7 | a9 a11 a13 a15 b9 ...

        v0.data() = AVX::unpacklo_epi16(tmp0, tmp1); // a0 a1 a2 a3 a4 a5 a6 a7 | a8 a9 ...
        v1.data() = AVX::unpackhi_epi16(tmp0, tmp1); // b0 b1 b2 b3 b4 b5 b6 b7 | b8 b9 ...
    }/*}}}*/
    template<typename I> static inline void deinterleave(typename V::EntryType const *const data,/*{{{*/
            const I &i, V &v0, V &v1, V &v2)
    {
        using namespace AVX;
        const __m256i tmp0 = avx_cast<__m256i>(_mm256_setr_pd(
            *aliasing_cast<double>(&data[i[0]]), *aliasing_cast<double>(&data[i[1]]),
            *aliasing_cast<double>(&data[i[8]]), *aliasing_cast<double>(&data[i[9]])));
        const __m256i tmp1 = avx_cast<__m256i>(_mm256_setr_pd(
            *aliasing_cast<double>(&data[i[2]]), *aliasing_cast<double>(&data[i[3]]),
            *aliasing_cast<double>(&data[i[10]]), *aliasing_cast<double>(&data[i[11]])));
        const __m256i tmp2 = avx_cast<__m256i>(_mm256_setr_pd(
            *aliasing_cast<double>(&data[i[4]]), *aliasing_cast<double>(&data[i[5]]),
            *aliasing_cast<double>(&data[i[12]]), *aliasing_cast<double>(&data[i[13]])));
        const __m256i tmp3 = avx_cast<__m256i>(_mm256_setr_pd(
            *aliasing_cast<double>(&data[i[6]]), *aliasing_cast<double>(&data[i[7]]),
            *aliasing_cast<double>(&data[i[14]]), *aliasing_cast<double>(&data[i[15]])));
        const __m256i tmp4 = AVX::unpacklo_epi16(tmp0, tmp2); // a0 a4 b0 b4 c0 c4 XX XX | a8 a12 b8 ...
        const __m256i tmp5 = AVX::unpackhi_epi16(tmp0, tmp2); // a1 a5 ...
        const __m256i tmp6 = AVX::unpacklo_epi16(tmp1, tmp3); // a2 a6 ...
        const __m256i tmp7 = AVX::unpackhi_epi16(tmp1, tmp3); // a3 a7 ...

        const __m256i tmp8  = AVX::unpacklo_epi16(tmp4, tmp6); // a0 a2 a4 a6 b0 ...
        const __m256i tmp9  = AVX::unpackhi_epi16(tmp4, tmp6); // c0 c2 c4 c6 XX ...
        const __m256i tmp10 = AVX::unpacklo_epi16(tmp5, tmp7); // a1 a3 a5 a7 b1 ...
        const __m256i tmp11 = AVX::unpackhi_epi16(tmp5, tmp7); // c1 c3 c5 c7 XX ...

        v0.data() = AVX::unpacklo_epi16(tmp8, tmp10); // a0 a1 a2 a3 a4 a5 a6 a7 | a8 ...
        v1.data() = AVX::unpackhi_epi16(tmp8, tmp10);
        v2.data() = AVX::unpacklo_epi16(tmp9, tmp11);
    }/*}}}*/
    template<typename I> static inline void deinterleave(typename V::EntryType const *const data,/*{{{*/
            const I &i, V &v0, V &v1, V &v2, V &v3)
    {
        using namespace AVX;
        const __m256i tmp0 = avx_cast<__m256i>(_mm256_setr_pd(
            *aliasing_cast<double>(&data[i[0]]), *aliasing_cast<double>(&data[i[1]]),
            *aliasing_cast<double>(&data[i[8]]), *aliasing_cast<double>(&data[i[9]])));
        const __m256i tmp1 = avx_cast<__m256i>(_mm256_setr_pd(
            *aliasing_cast<double>(&data[i[2]]), *aliasing_cast<double>(&data[i[3]]),
            *aliasing_cast<double>(&data[i[10]]), *aliasing_cast<double>(&data[i[11]])));
        const __m256i tmp2 = avx_cast<__m256i>(_mm256_setr_pd(
            *aliasing_cast<double>(&data[i[4]]), *aliasing_cast<double>(&data[i[5]]),
            *aliasing_cast<double>(&data[i[12]]), *aliasing_cast<double>(&data[i[13]])));
        const __m256i tmp3 = avx_cast<__m256i>(_mm256_setr_pd(
            *aliasing_cast<double>(&data[i[6]]), *aliasing_cast<double>(&data[i[7]]),
            *aliasing_cast<double>(&data[i[14]]), *aliasing_cast<double>(&data[i[15]])));
        const __m256i tmp4 = AVX::unpacklo_epi16(tmp0, tmp2); // a0 a4 b0 b4 c0 c4 d0 d4 | a8 a12 b8 ...
        const __m256i tmp5 = AVX::unpackhi_epi16(tmp0, tmp2); // a1 a5 ...
        const __m256i tmp6 = AVX::unpacklo_epi16(tmp1, tmp3); // a2 a6 ...
        const __m256i tmp7 = AVX::unpackhi_epi16(tmp1, tmp3); // a3 a7 ...

        const __m256i tmp8  = AVX::unpacklo_epi16(tmp4, tmp6); // a0 a2 a4 a6 b0 ...
        const __m256i tmp9  = AVX::unpackhi_epi16(tmp4, tmp6); // c0 c2 c4 c6 d0 ...
        const __m256i tmp10 = AVX::unpacklo_epi16(tmp5, tmp7); // a1 a3 a5 a7 b1 ...
        const __m256i tmp11 = AVX::unpackhi_epi16(tmp5, tmp7); // c1 c3 c5 c7 d1 ...

        v0.data() = AVX::unpacklo_epi16(tmp8, tmp10); // a0 a1 a2 a3 a4 a5 a6 a7 | a8 ...
        v1.data() = AVX::unpackhi_epi16(tmp8, tmp10);
        v2.data() = AVX::unpacklo_epi16(tmp9, tmp11);
        v3.data() = AVX::unpackhi_epi16(tmp9, tmp11);
    }/*}}}*/
    template<typename I> static inline void deinterleave(typename V::EntryType const *const data,/*{{{*/
            const I &i, V &v0, V &v1, V &v2, V &v3, V &v4)
    {
        using namespace AVX;
        const __m256i a = concat(_mm_loadu_si128(reinterpret_cast<const __m128i *>(&data[i[0]])),
                                 _mm_loadu_si128(reinterpret_cast<const __m128i *>(&data[i[8]])));
        const __m256i b = concat(_mm_loadu_si128(reinterpret_cast<const __m128i *>(&data[i[1]])),
                                 _mm_loadu_si128(reinterpret_cast<const __m128i *>(&data[i[9]])));
        const __m256i c = concat(_mm_loadu_si128(reinterpret_cast<const __m128i *>(&data[i[2]])),
                                 _mm_loadu_si128(reinterpret_cast<const __m128i *>(&data[i[10]])));
        const __m256i d = concat(_mm_loadu_si128(reinterpret_cast<const __m128i *>(&data[i[3]])),
                                 _mm_loadu_si128(reinterpret_cast<const __m128i *>(&data[i[11]])));
        const __m256i e = concat(_mm_loadu_si128(reinterpret_cast<const __m128i *>(&data[i[4]])),
                                 _mm_loadu_si128(reinterpret_cast<const __m128i *>(&data[i[12]])));
        const __m256i f = concat(_mm_loadu_si128(reinterpret_cast<const __m128i *>(&data[i[5]])),
                                 _mm_loadu_si128(reinterpret_cast<const __m128i *>(&data[i[13]])));
        const __m256i g = concat(_mm_loadu_si128(reinterpret_cast<const __m128i *>(&data[i[6]])),
                                 _mm_loadu_si128(reinterpret_cast<const __m128i *>(&data[i[14]])));
        const __m256i h = concat(_mm_loadu_si128(reinterpret_cast<const __m128i *>(&data[i[7]])),
                                 _mm_loadu_si128(reinterpret_cast<const __m128i *>(&data[i[15]])));

        const __m256i tmp2  = AVX::unpacklo_epi16(a, e); // a0 a4 b0 b4 c0 c4 d0 d4 | a8 ...
        const __m256i tmp4  = AVX::unpacklo_epi16(b, f); // a1 a5 b1 b5 c1 c5 d1 d5
        const __m256i tmp3  = AVX::unpacklo_epi16(c, g); // a2 a6 b2 b6 c2 c6 d2 d6
        const __m256i tmp5  = AVX::unpacklo_epi16(d, h); // a3 a7 b3 b7 c3 c7 d3 d7
        const __m256i tmp10 = AVX::unpackhi_epi16(a, e); // e0 e4 f0 f4 g0 g4 h0 h4
        const __m256i tmp11 = AVX::unpackhi_epi16(c, g); // e1 e5 f1 f5 g1 g5 h1 h5
        const __m256i tmp12 = AVX::unpackhi_epi16(b, f); // e2 e6 f2 f6 g2 g6 h2 h6
        const __m256i tmp13 = AVX::unpackhi_epi16(d, h); // e3 e7 f3 f7 g3 g7 h3 h7

        const __m256i tmp0  = AVX::unpacklo_epi16(tmp2, tmp3); // a0 a2 a4 a6 b0 b2 b4 b6 | a8 ...
        const __m256i tmp1  = AVX::unpacklo_epi16(tmp4, tmp5); // a1 a3 a5 a7 b1 b3 b5 b7
        const __m256i tmp6  = AVX::unpackhi_epi16(tmp2, tmp3); // c0 c2 c4 c6 d0 d2 d4 d6
        const __m256i tmp7  = AVX::unpackhi_epi16(tmp4, tmp5); // c1 c3 c5 c7 d1 d3 d5 d7
        const __m256i tmp8  = AVX::unpacklo_epi16(tmp10, tmp11); // e0 e2 e4 e6 f0 f2 f4 f6
        const __m256i tmp9  = AVX::unpacklo_epi16(tmp12, tmp13); // e1 e3 e5 e7 f1 f3 f5 f7

        v0.data() = AVX::unpacklo_epi16(tmp0, tmp1);
        v1.data() = AVX::unpackhi_epi16(tmp0, tmp1);
        v2.data() = AVX::unpacklo_epi16(tmp6, tmp7);
        v3.data() = AVX::unpackhi_epi16(tmp6, tmp7);
        v4.data() = AVX::unpacklo_epi16(tmp8, tmp9);
    }/*}}}*/
    template<typename I> static inline void deinterleave(typename V::EntryType const *const data,/*{{{*/
            const I &i, V &v0, V &v1, V &v2, V &v3, V &v4, V &v5)
    {
        using namespace AVX;
        const __m256i a = concat(_mm_loadu_si128(reinterpret_cast<const __m128i *>(&data[i[0]])),
                                 _mm_loadu_si128(reinterpret_cast<const __m128i *>(&data[i[8]])));
        const __m256i b = concat(_mm_loadu_si128(reinterpret_cast<const __m128i *>(&data[i[1]])),
                                 _mm_loadu_si128(reinterpret_cast<const __m128i *>(&data[i[9]])));
        const __m256i c = concat(_mm_loadu_si128(reinterpret_cast<const __m128i *>(&data[i[2]])),
                                 _mm_loadu_si128(reinterpret_cast<const __m128i *>(&data[i[10]])));
        const __m256i d = concat(_mm_loadu_si128(reinterpret_cast<const __m128i *>(&data[i[3]])),
                                 _mm_loadu_si128(reinterpret_cast<const __m128i *>(&data[i[11]])));
        const __m256i e = concat(_mm_loadu_si128(reinterpret_cast<const __m128i *>(&data[i[4]])),
                                 _mm_loadu_si128(reinterpret_cast<const __m128i *>(&data[i[12]])));
        const __m256i f = concat(_mm_loadu_si128(reinterpret_cast<const __m128i *>(&data[i[5]])),
                                 _mm_loadu_si128(reinterpret_cast<const __m128i *>(&data[i[13]])));
        const __m256i g = concat(_mm_loadu_si128(reinterpret_cast<const __m128i *>(&data[i[6]])),
                                 _mm_loadu_si128(reinterpret_cast<const __m128i *>(&data[i[14]])));
        const __m256i h = concat(_mm_loadu_si128(reinterpret_cast<const __m128i *>(&data[i[7]])),
                                 _mm_loadu_si128(reinterpret_cast<const __m128i *>(&data[i[15]])));

        const __m256i tmp2  = AVX::unpacklo_epi16(a, e); // a0 a4 b0 b4 c0 c4 d0 d4 | a8 ...
        const __m256i tmp4  = AVX::unpacklo_epi16(b, f); // a1 a5 b1 b5 c1 c5 d1 d5
        const __m256i tmp3  = AVX::unpacklo_epi16(c, g); // a2 a6 b2 b6 c2 c6 d2 d6
        const __m256i tmp5  = AVX::unpacklo_epi16(d, h); // a3 a7 b3 b7 c3 c7 d3 d7
        const __m256i tmp10 = AVX::unpackhi_epi16(a, e); // e0 e4 f0 f4 g0 g4 h0 h4
        const __m256i tmp11 = AVX::unpackhi_epi16(c, g); // e1 e5 f1 f5 g1 g5 h1 h5
        const __m256i tmp12 = AVX::unpackhi_epi16(b, f); // e2 e6 f2 f6 g2 g6 h2 h6
        const __m256i tmp13 = AVX::unpackhi_epi16(d, h); // e3 e7 f3 f7 g3 g7 h3 h7

        const __m256i tmp0  = AVX::unpacklo_epi16(tmp2, tmp3); // a0 a2 a4 a6 b0 b2 b4 b6 | a8 ...
        const __m256i tmp1  = AVX::unpacklo_epi16(tmp4, tmp5); // a1 a3 a5 a7 b1 b3 b5 b7
        const __m256i tmp6  = AVX::unpackhi_epi16(tmp2, tmp3); // c0 c2 c4 c6 d0 d2 d4 d6
        const __m256i tmp7  = AVX::unpackhi_epi16(tmp4, tmp5); // c1 c3 c5 c7 d1 d3 d5 d7
        const __m256i tmp8  = AVX::unpacklo_epi16(tmp10, tmp11); // e0 e2 e4 e6 f0 f2 f4 f6
        const __m256i tmp9  = AVX::unpacklo_epi16(tmp12, tmp13); // e1 e3 e5 e7 f1 f3 f5 f7

        v0.data() = AVX::unpacklo_epi16(tmp0, tmp1);
        v1.data() = AVX::unpackhi_epi16(tmp0, tmp1);
        v2.data() = AVX::unpacklo_epi16(tmp6, tmp7);
        v3.data() = AVX::unpackhi_epi16(tmp6, tmp7);
        v4.data() = AVX::unpacklo_epi16(tmp8, tmp9);
        v5.data() = AVX::unpackhi_epi16(tmp8, tmp9);
    }/*}}}*/
    template<typename I> static inline void deinterleave(typename V::EntryType const *const data,/*{{{*/
            const I &i, V &v0, V &v1, V &v2, V &v3, V &v4, V &v5, V &v6)
    {
        using namespace AVX;
        const __m256i a = concat(_mm_loadu_si128(reinterpret_cast<const __m128i *>(&data[i[0]])),
                                 _mm_loadu_si128(reinterpret_cast<const __m128i *>(&data[i[8]])));
        const __m256i b = concat(_mm_loadu_si128(reinterpret_cast<const __m128i *>(&data[i[1]])),
                                 _mm_loadu_si128(reinterpret_cast<const __m128i *>(&data[i[9]])));
        const __m256i c = concat(_mm_loadu_si128(reinterpret_cast<const __m128i *>(&data[i[2]])),
                                 _mm_loadu_si128(reinterpret_cast<const __m128i *>(&data[i[10]])));
        const __m256i d = concat(_mm_loadu_si128(reinterpret_cast<const __m128i *>(&data[i[3]])),
                                 _mm_loadu_si128(reinterpret_cast<const __m128i *>(&data[i[11]])));
        const __m256i e = concat(_mm_loadu_si128(reinterpret_cast<const __m128i *>(&data[i[4]])),
                                 _mm_loadu_si128(reinterpret_cast<const __m128i *>(&data[i[12]])));
        const __m256i f = concat(_mm_loadu_si128(reinterpret_cast<const __m128i *>(&data[i[5]])),
                                 _mm_loadu_si128(reinterpret_cast<const __m128i *>(&data[i[13]])));
        const __m256i g = concat(_mm_loadu_si128(reinterpret_cast<const __m128i *>(&data[i[6]])),
                                 _mm_loadu_si128(reinterpret_cast<const __m128i *>(&data[i[14]])));
        const __m256i h = concat(_mm_loadu_si128(reinterpret_cast<const __m128i *>(&data[i[7]])),
                                 _mm_loadu_si128(reinterpret_cast<const __m128i *>(&data[i[15]])));

        const __m256i tmp2  = AVX::unpacklo_epi16(a, e); // a0 a4 b0 b4 c0 c4 d0 d4 | a8 ...
        const __m256i tmp4  = AVX::unpacklo_epi16(b, f); // a1 a5 b1 b5 c1 c5 d1 d5
        const __m256i tmp3  = AVX::unpacklo_epi16(c, g); // a2 a6 b2 b6 c2 c6 d2 d6
        const __m256i tmp5  = AVX::unpacklo_epi16(d, h); // a3 a7 b3 b7 c3 c7 d3 d7
        const __m256i tmp10 = AVX::unpackhi_epi16(a, e); // e0 e4 f0 f4 g0 g4 h0 h4
        const __m256i tmp11 = AVX::unpackhi_epi16(c, g); // e1 e5 f1 f5 g1 g5 h1 h5
        const __m256i tmp12 = AVX::unpackhi_epi16(b, f); // e2 e6 f2 f6 g2 g6 h2 h6
        const __m256i tmp13 = AVX::unpackhi_epi16(d, h); // e3 e7 f3 f7 g3 g7 h3 h7

        const __m256i tmp0  = AVX::unpacklo_epi16(tmp2, tmp3); // a0 a2 a4 a6 b0 b2 b4 b6 | a8 ...
        const __m256i tmp1  = AVX::unpacklo_epi16(tmp4, tmp5); // a1 a3 a5 a7 b1 b3 b5 b7
        const __m256i tmp6  = AVX::unpackhi_epi16(tmp2, tmp3); // c0 c2 c4 c6 d0 d2 d4 d6
        const __m256i tmp7  = AVX::unpackhi_epi16(tmp4, tmp5); // c1 c3 c5 c7 d1 d3 d5 d7
        const __m256i tmp8  = AVX::unpacklo_epi16(tmp10, tmp11); // e0 e2 e4 e6 f0 f2 f4 f6
        const __m256i tmp9  = AVX::unpacklo_epi16(tmp12, tmp13); // e1 e3 e5 e7 f1 f3 f5 f7
        const __m256i tmp14 = AVX::unpackhi_epi16(tmp10, tmp11); // g0 g2 g4 g6 h0 h2 h4 h6
        const __m256i tmp15 = AVX::unpackhi_epi16(tmp12, tmp13); // g1 g3 g5 g7 h1 h3 h5 h7

        v0.data() = AVX::unpacklo_epi16(tmp0, tmp1);
        v1.data() = AVX::unpackhi_epi16(tmp0, tmp1);
        v2.data() = AVX::unpacklo_epi16(tmp6, tmp7);
        v3.data() = AVX::unpackhi_epi16(tmp6, tmp7);
        v4.data() = AVX::unpacklo_epi16(tmp8, tmp9);
        v5.data() = AVX::unpackhi_epi16(tmp8, tmp9);
        v6.data() = AVX::unpacklo_epi16(tmp14, tmp15);
    }/*}}}*/
    template<typename I> static inline void deinterleave(typename V::EntryType const *const data,/*{{{*/
            const I &i, V &v0, V &v1, V &v2, V &v3, V &v4, V &v5, V &v6, V &v7)
    {
        using namespace AVX;
        const __m256i a = concat(_mm_loadu_si128(reinterpret_cast<const __m128i *>(&data[i[0]])),
                                 _mm_loadu_si128(reinterpret_cast<const __m128i *>(&data[i[8]])));
        const __m256i b = concat(_mm_loadu_si128(reinterpret_cast<const __m128i *>(&data[i[1]])),
                                 _mm_loadu_si128(reinterpret_cast<const __m128i *>(&data[i[9]])));
        const __m256i c = concat(_mm_loadu_si128(reinterpret_cast<const __m128i *>(&data[i[2]])),
                                 _mm_loadu_si128(reinterpret_cast<const __m128i *>(&data[i[10]])));
        const __m256i d = concat(_mm_loadu_si128(reinterpret_cast<const __m128i *>(&data[i[3]])),
                                 _mm_loadu_si128(reinterpret_cast<const __m128i *>(&data[i[11]])));
        const __m256i e = concat(_mm_loadu_si128(reinterpret_cast<const __m128i *>(&data[i[4]])),
                                 _mm_loadu_si128(reinterpret_cast<const __m128i *>(&data[i[12]])));
        const __m256i f = concat(_mm_loadu_si128(reinterpret_cast<const __m128i *>(&data[i[5]])),
                                 _mm_loadu_si128(reinterpret_cast<const __m128i *>(&data[i[13]])));
        const __m256i g = concat(_mm_loadu_si128(reinterpret_cast<const __m128i *>(&data[i[6]])),
                                 _mm_loadu_si128(reinterpret_cast<const __m128i *>(&data[i[14]])));
        const __m256i h = concat(_mm_loadu_si128(reinterpret_cast<const __m128i *>(&data[i[7]])),
                                 _mm_loadu_si128(reinterpret_cast<const __m128i *>(&data[i[15]])));

        const __m256i tmp2  = AVX::unpacklo_epi16(a, e); // a0 a4 b0 b4 c0 c4 d0 d4 | a8 ...
        const __m256i tmp4  = AVX::unpacklo_epi16(b, f); // a1 a5 b1 b5 c1 c5 d1 d5
        const __m256i tmp3  = AVX::unpacklo_epi16(c, g); // a2 a6 b2 b6 c2 c6 d2 d6
        const __m256i tmp5  = AVX::unpacklo_epi16(d, h); // a3 a7 b3 b7 c3 c7 d3 d7
        const __m256i tmp10 = AVX::unpackhi_epi16(a, e); // e0 e4 f0 f4 g0 g4 h0 h4
        const __m256i tmp11 = AVX::unpackhi_epi16(c, g); // e1 e5 f1 f5 g1 g5 h1 h5
        const __m256i tmp12 = AVX::unpackhi_epi16(b, f); // e2 e6 f2 f6 g2 g6 h2 h6
        const __m256i tmp13 = AVX::unpackhi_epi16(d, h); // e3 e7 f3 f7 g3 g7 h3 h7

        const __m256i tmp0  = AVX::unpacklo_epi16(tmp2, tmp3); // a0 a2 a4 a6 b0 b2 b4 b6 | a8 ...
        const __m256i tmp1  = AVX::unpacklo_epi16(tmp4, tmp5); // a1 a3 a5 a7 b1 b3 b5 b7
        const __m256i tmp6  = AVX::unpackhi_epi16(tmp2, tmp3); // c0 c2 c4 c6 d0 d2 d4 d6
        const __m256i tmp7  = AVX::unpackhi_epi16(tmp4, tmp5); // c1 c3 c5 c7 d1 d3 d5 d7
        const __m256i tmp8  = AVX::unpacklo_epi16(tmp10, tmp11); // e0 e2 e4 e6 f0 f2 f4 f6
        const __m256i tmp9  = AVX::unpacklo_epi16(tmp12, tmp13); // e1 e3 e5 e7 f1 f3 f5 f7
        const __m256i tmp14 = AVX::unpackhi_epi16(tmp10, tmp11); // g0 g2 g4 g6 h0 h2 h4 h6
        const __m256i tmp15 = AVX::unpackhi_epi16(tmp12, tmp13); // g1 g3 g5 g7 h1 h3 h5 h7

        v0.data() = AVX::unpacklo_epi16(tmp0, tmp1);
        v1.data() = AVX::unpackhi_epi16(tmp0, tmp1);
        v2.data() = AVX::unpacklo_epi16(tmp6, tmp7);
        v3.data() = AVX::unpackhi_epi16(tmp6, tmp7);
        v4.data() = AVX::unpacklo_epi16(tmp8, tmp9);
        v5.data() = AVX::unpackhi_epi16(tmp8, tmp9);
        v6.data() = AVX::unpacklo_epi16(tmp14, tmp15);
        v7.data() = AVX::unpackhi_epi16(tmp14, tmp15);
    }/*}}}*/
};
template<typename V> struct InterleaveImpl<V, 8, 32> {
    static_assert(sizeof(typename V::value_type) == 4, "");
    template<typename I> static inline void interleave(typename V::EntryType *const data, const I &i,/*{{{*/
            const typename V::AsArg v0, const typename V::AsArg v1)
    {
        using namespace AVX;
        // [0a 1a 0b 1b 0e 1e 0f 1f]:
        const m256 tmp0 = _mm256_unpacklo_ps(avx_cast<m256>(v0.data()), avx_cast<m256>(v1.data()));
        // [0c 1c 0d 1d 0g 1g 0h 1h]:
        const m256 tmp1 = _mm256_unpackhi_ps(avx_cast<m256>(v0.data()), avx_cast<m256>(v1.data()));
        _mm_storel_pi(reinterpret_cast<__m64 *>(&data[i[0]]), lo128(tmp0));
        _mm_storeh_pi(reinterpret_cast<__m64 *>(&data[i[1]]), lo128(tmp0));
        _mm_storel_pi(reinterpret_cast<__m64 *>(&data[i[2]]), lo128(tmp1));
        _mm_storeh_pi(reinterpret_cast<__m64 *>(&data[i[3]]), lo128(tmp1));
        _mm_storel_pi(reinterpret_cast<__m64 *>(&data[i[4]]), hi128(tmp0));
        _mm_storeh_pi(reinterpret_cast<__m64 *>(&data[i[5]]), hi128(tmp0));
        _mm_storel_pi(reinterpret_cast<__m64 *>(&data[i[6]]), hi128(tmp1));
        _mm_storeh_pi(reinterpret_cast<__m64 *>(&data[i[7]]), hi128(tmp1));
    }/*}}}*/
    static inline void interleave(typename V::EntryType *const data, const Common::SuccessiveEntries<2> &i,/*{{{*/
            const typename V::AsArg v0, const typename V::AsArg v1)
    {
        using namespace AVX;
        // [0a 1a 0b 1b 0e 1e 0f 1f]:
        const m256 tmp0 = _mm256_unpacklo_ps(avx_cast<m256>(v0.data()), avx_cast<m256>(v1.data()));
        // [0c 1c 0d 1d 0g 1g 0h 1h]:
        const m256 tmp1 = _mm256_unpackhi_ps(avx_cast<m256>(v0.data()), avx_cast<m256>(v1.data()));
        _mm_storeu_ps(aliasing_cast<float>(&data[i[0]]), lo128(tmp0));
        _mm_storeu_ps(aliasing_cast<float>(&data[i[2]]), lo128(tmp1));
        _mm_storeu_ps(aliasing_cast<float>(&data[i[4]]), hi128(tmp0));
        _mm_storeu_ps(aliasing_cast<float>(&data[i[6]]), hi128(tmp1));
    }/*}}}*/
    // interleave scatter 3 {{{
    template <typename I>
    static inline void interleave(typename V::EntryType *const data, const I &i,
                                  const typename V::AsArg v0, const typename V::AsArg v1,
                                  const typename V::AsArg v2)
    {
        using namespace AVX;
#ifdef Vc_USE_MASKMOV_SCATTER
        // [0a 2a 0b 2b 0e 2e 0f 2f]:
        const m256 tmp0 = _mm256_unpacklo_ps(avx_cast<m256>(v0.data()), avx_cast<m256>(v2.data()));
        // [0c 2c 0d 2d 0g 2g 0h 2h]:
        const m256 tmp1 = _mm256_unpackhi_ps(avx_cast<m256>(v0.data()), avx_cast<m256>(v2.data()));
        // [1a __ 1b __ 1e __ 1f __]:
        const m256 tmp2 = _mm256_unpacklo_ps(avx_cast<m256>(v1.data()), avx_cast<m256>(v1.data()));
        // [1c __ 1d __ 1g __ 1h __]:
        const m256 tmp3 = _mm256_unpackhi_ps(avx_cast<m256>(v1.data()), avx_cast<m256>(v1.data()));
        const m256 tmp4 = _mm256_unpacklo_ps(tmp0, tmp2);
        const m256 tmp5 = _mm256_unpackhi_ps(tmp0, tmp2);
        const m256 tmp6 = _mm256_unpacklo_ps(tmp1, tmp3);
        const m256 tmp7 = _mm256_unpackhi_ps(tmp1, tmp3);
        const m128i mask = _mm_set_epi32(0, -1, -1, -1);
        _mm_maskstore_ps(aliasing_cast<float>(&data[i[0]]), mask, lo128(tmp4));
        _mm_maskstore_ps(aliasing_cast<float>(&data[i[1]]), mask, lo128(tmp5));
        _mm_maskstore_ps(aliasing_cast<float>(&data[i[2]]), mask, lo128(tmp6));
        _mm_maskstore_ps(aliasing_cast<float>(&data[i[3]]), mask, lo128(tmp7));
        _mm_maskstore_ps(aliasing_cast<float>(&data[i[4]]), mask, hi128(tmp4));
        _mm_maskstore_ps(aliasing_cast<float>(&data[i[5]]), mask, hi128(tmp5));
        _mm_maskstore_ps(aliasing_cast<float>(&data[i[6]]), mask, hi128(tmp6));
        _mm_maskstore_ps(aliasing_cast<float>(&data[i[7]]), mask, hi128(tmp7));
#else
        interleave(data, i, v0, v1);
        v2.scatter(data + 2, i);
#endif
    }  // }}}
    // interleave successive 3 {{{
    static inline void interleave(typename V::EntryType *const data,
                                  const Common::SuccessiveEntries<3> &i,
                                  const typename V::AsArg v0_,
                                  const typename V::AsArg v1_,
                                  const typename V::AsArg v2_)
    {
        __m256 v0 = AVX::avx_cast<__m256>(v0_.data());  // a0 a1 a2 a3|a4 a5 a6 a7
        __m256 v1 = AVX::avx_cast<__m256>(v1_.data());  // b0 b1 b2 b3|b4 b5 b6 b7
        __m256 v2 = AVX::avx_cast<__m256>(v2_.data());  // c0 c1 c2 c3|c4 c5 c6 c7

        v0 = _mm256_shuffle_ps(v0, v0, 0x6c);  // a0 a3 a2 a1|a4 a7 a6 a5
        v1 = _mm256_shuffle_ps(v1, v1, 0xb1);  // b1 b0 b3 b2|b5 b4 b7 b6
        v2 = _mm256_shuffle_ps(v2, v2, 0xc6);  // c2 c1 c0 c3|c6 c5 c4 c7

        // a0 b0 c0 a1|c6 a7 b7 c7:
        __m256 w0 = Mem::blend<X0, X1, Y2, X3, Y4, X5, X6, Y7>(
                    Mem::blend<X0, Y1, X2, X3, X4, X5, Y6, X7>(v0, v1), v2);
        // b1 c1 a2 b2|b5 c5 a6 b6:
        __m256 w1 = Mem::blend<X0, Y1, X2, X3, X4, Y5, X6, X7>(
                    Mem::blend<Y0, X1, X2, Y3, Y4, X5, X6, Y7>(v0, v1), v2);
        // c2 a3 b3 c3|a4 b4 c4 a5:
        __m256 w2 = Mem::blend<Y0, X1, X2, Y3, X4, X5, Y6, X7>(
                    Mem::blend<X0, X1, Y2, X3, X4, Y5, X6, X7>(v0, v1), v2);

        // a0 b0 c0 a1|b1 c1 a2 b2:
        _mm256_storeu_ps(aliasing_cast<float>(&data[i[0]]),
                         _mm256_permute2f128_ps(w0, w1, 0x20));
        // c2 a3 b3 c3|a4 b4 c4 a5: w2
        _mm256_storeu_ps(aliasing_cast<float>(&data[i[0]] + 8), w2);
        // b5 c5 a6 b6|c6 a7 b7 c7:
        _mm256_storeu_ps(aliasing_cast<float>(&data[i[0]] + 16),
                         _mm256_permute2f128_ps(w1, w0, 0x31));

    }  //}}}
    // interleave scatter 4 {{{
    template <typename I>
    static inline void interleave(typename V::EntryType *const data, const I &i,
                                  const typename V::AsArg v0, const typename V::AsArg v1,
                                  const typename V::AsArg v2, const typename V::AsArg v3)
    {
        using namespace AVX;
        const __m256 tmp0 =
            _mm256_unpacklo_ps(avx_cast<m256>(v0.data()), avx_cast<m256>(v2.data()));
        const __m256 tmp1 =
            _mm256_unpackhi_ps(avx_cast<m256>(v0.data()), avx_cast<m256>(v2.data()));
        const __m256 tmp2 =
            _mm256_unpacklo_ps(avx_cast<m256>(v1.data()), avx_cast<m256>(v3.data()));
        const __m256 tmp3 =
            _mm256_unpackhi_ps(avx_cast<m256>(v1.data()), avx_cast<m256>(v3.data()));
        const __m256 _04 = _mm256_unpacklo_ps(tmp0, tmp2);
        const __m256 _15 = _mm256_unpackhi_ps(tmp0, tmp2);
        const __m256 _26 = _mm256_unpacklo_ps(tmp1, tmp3);
        const __m256 _37 = _mm256_unpackhi_ps(tmp1, tmp3);
        _mm_storeu_ps(aliasing_cast<float>(&data[i[0]]), lo128(_04));
        _mm_storeu_ps(aliasing_cast<float>(&data[i[1]]), lo128(_15));
        _mm_storeu_ps(aliasing_cast<float>(&data[i[2]]), lo128(_26));
        _mm_storeu_ps(aliasing_cast<float>(&data[i[3]]), lo128(_37));
        _mm_storeu_ps(aliasing_cast<float>(&data[i[4]]), hi128(_04));
        _mm_storeu_ps(aliasing_cast<float>(&data[i[5]]), hi128(_15));
        _mm_storeu_ps(aliasing_cast<float>(&data[i[6]]), hi128(_26));
        _mm_storeu_ps(aliasing_cast<float>(&data[i[7]]), hi128(_37));
    }  // }}}
    // interleave successive 4 {{{
    // same as above except fot the stores, that can be combined to 256-bit stores
    static inline void interleave(typename V::EntryType *const data,
                                  const Common::SuccessiveEntries<4> &i,
                                  const typename V::AsArg v0, const typename V::AsArg v1,
                                  const typename V::AsArg v2, const typename V::AsArg v3)
    {
        using namespace AVX;
        const __m256 tmp0 =
            _mm256_unpacklo_ps(avx_cast<m256>(v0.data()), avx_cast<m256>(v2.data()));
        const __m256 tmp1 =
            _mm256_unpackhi_ps(avx_cast<m256>(v0.data()), avx_cast<m256>(v2.data()));
        const __m256 tmp2 =
            _mm256_unpacklo_ps(avx_cast<m256>(v1.data()), avx_cast<m256>(v3.data()));
        const __m256 tmp3 =
            _mm256_unpackhi_ps(avx_cast<m256>(v1.data()), avx_cast<m256>(v3.data()));
        const __m256 _04 = _mm256_unpacklo_ps(tmp0, tmp2);
        const __m256 _15 = _mm256_unpackhi_ps(tmp0, tmp2);
        const __m256 _26 = _mm256_unpacklo_ps(tmp1, tmp3);
        const __m256 _37 = _mm256_unpackhi_ps(tmp1, tmp3);
        _mm256_storeu_ps(aliasing_cast<float>(&data[i[0]]),
                         _mm256_permute2f128_ps(_04, _15, 0x20));
        _mm256_storeu_ps(aliasing_cast<float>(&data[i[0]] + 8),
                         _mm256_permute2f128_ps(_26, _37, 0x20));
        _mm256_storeu_ps(aliasing_cast<float>(&data[i[0]] + 16),
                         _mm256_permute2f128_ps(_04, _15, 0x31));
        _mm256_storeu_ps(aliasing_cast<float>(&data[i[0]] + 24),
                         _mm256_permute2f128_ps(_26, _37, 0x31));
    }                      // }}}
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
    // deinterleave scatter 2 {{{
    template <typename I>
    static inline void deinterleave(typename V::EntryType const *const data, const I &i,
                                    V &v0, V &v1)
    {
        using namespace AVX;
        const m128  il0 = _mm_loadl_pi(_mm_setzero_ps(), reinterpret_cast<__m64 const *>(&data[i[0]])); // a0 b0
        const m128  il2 = _mm_loadl_pi(_mm_setzero_ps(), reinterpret_cast<__m64 const *>(&data[i[2]])); // a2 b2
        const m128  il4 = _mm_loadl_pi(_mm_setzero_ps(), reinterpret_cast<__m64 const *>(&data[i[4]])); // a4 b4
        const m128  il6 = _mm_loadl_pi(_mm_setzero_ps(), reinterpret_cast<__m64 const *>(&data[i[6]])); // a6 b6
        const m128 il01 = _mm_loadh_pi(             il0, reinterpret_cast<__m64 const *>(&data[i[1]])); // a0 b0 a1 b1
        const m128 il23 = _mm_loadh_pi(             il2, reinterpret_cast<__m64 const *>(&data[i[3]])); // a2 b2 a3 b3
        const m128 il45 = _mm_loadh_pi(             il4, reinterpret_cast<__m64 const *>(&data[i[5]])); // a4 b4 a5 b5
        const m128 il67 = _mm_loadh_pi(             il6, reinterpret_cast<__m64 const *>(&data[i[7]])); // a6 b6 a7 b7

        const m256 tmp2 = concat(il01, il45);
        const m256 tmp3 = concat(il23, il67);

        const m256 tmp0 = _mm256_unpacklo_ps(tmp2, tmp3);
        const m256 tmp1 = _mm256_unpackhi_ps(tmp2, tmp3);

        v0.data() = avx_cast<typename V::VectorType>(_mm256_unpacklo_ps(tmp0, tmp1));
        v1.data() = avx_cast<typename V::VectorType>(_mm256_unpackhi_ps(tmp0, tmp1));
    }  // }}}
    // deinterleave successive 2 {{{
    static inline void deinterleave(typename V::EntryType const *const data,
                                    const Common::SuccessiveEntries<2> &i, V &v0, V &v1)
    {
        using namespace AVX;
        const m256 il0123 = _mm256_loadu_ps(aliasing_cast<float>(&data[i[0]])); // a0 b0 a1 b1 a2 b2 a3 b3
        const m256 il4567 = _mm256_loadu_ps(aliasing_cast<float>(&data[i[4]])); // a4 b4 a5 b5 a6 b6 a7 b7

        const m256 tmp2 = Mem::shuffle128<X0, Y0>(il0123, il4567);
        const m256 tmp3 = Mem::shuffle128<X1, Y1>(il0123, il4567);

        const m256 tmp0 = _mm256_unpacklo_ps(tmp2, tmp3);
        const m256 tmp1 = _mm256_unpackhi_ps(tmp2, tmp3);

        v0.data() = avx_cast<typename V::VectorType>(_mm256_unpacklo_ps(tmp0, tmp1));
        v1.data() = avx_cast<typename V::VectorType>(_mm256_unpackhi_ps(tmp0, tmp1));
    }  // }}}
    // deinterleave scatter 3 {{{
    template <typename I>
    static inline void deinterleave(typename V::EntryType const *const data, const I &i,
                                    V &v0, V &v1, V &v2)
    {
        using namespace AVX;
        const m128  il0 = _mm_loadu_ps(aliasing_cast<float>(&data[i[0]])); // a0 b0 c0 d0
        const m128  il1 = _mm_loadu_ps(aliasing_cast<float>(&data[i[1]])); // a1 b1 c1 d1
        const m128  il2 = _mm_loadu_ps(aliasing_cast<float>(&data[i[2]])); // a2 b2 c2 d2
        const m128  il3 = _mm_loadu_ps(aliasing_cast<float>(&data[i[3]])); // a3 b3 c3 d3
        const m128  il4 = _mm_loadu_ps(aliasing_cast<float>(&data[i[4]])); // a4 b4 c4 d4
        const m128  il5 = _mm_loadu_ps(aliasing_cast<float>(&data[i[5]])); // a5 b5 c5 d5
        const m128  il6 = _mm_loadu_ps(aliasing_cast<float>(&data[i[6]])); // a6 b6 c6 d6
        const m128  il7 = _mm_loadu_ps(aliasing_cast<float>(&data[i[7]])); // a7 b7 c7 d7

        const m256 il04 = concat(il0, il4);
        const m256 il15 = concat(il1, il5);
        const m256 il26 = concat(il2, il6);
        const m256 il37 = concat(il3, il7);
        const m256 ab0246 = _mm256_unpacklo_ps(il04, il26);
        const m256 ab1357 = _mm256_unpacklo_ps(il15, il37);
        const m256 cd0246 = _mm256_unpackhi_ps(il04, il26);
        const m256 cd1357 = _mm256_unpackhi_ps(il15, il37);
        v0.data() = avx_cast<typename V::VectorType>(_mm256_unpacklo_ps(ab0246, ab1357));
        v1.data() = avx_cast<typename V::VectorType>(_mm256_unpackhi_ps(ab0246, ab1357));
        v2.data() = avx_cast<typename V::VectorType>(_mm256_unpacklo_ps(cd0246, cd1357));
    }  // }}}
    // deinterleave successive 3 {{{
    static inline void deinterleave(typename V::EntryType const *const data,
                                    const Common::SuccessiveEntries<3> &i, V &v0, V &v1,
                                    V &v2)
    {
        // 0a 1a 2a 0b 1b 2b 0c 1c
        __m256 in0 = _mm256_loadu_ps(aliasing_cast<float>(&data[i[0]] + 0));
        // 2c 0d 1d 2d 0e 1e 2e 0f
        __m256 in1 = _mm256_loadu_ps(aliasing_cast<float>(&data[i[0]] + 8));
        // 1f 2f 0g 1g 2g 0h 1h 2h
        __m256 in2 = _mm256_loadu_ps(aliasing_cast<float>(&data[i[0]] + 16));

        // swap(v0.hi, v2.lo):
        //      [0a 1a 2a 0b 1f 2f 0g 1g]
        //      [2c 0d 1d 2d 0e 1e 2e 0f]
        //      [1b 2b 0c 1c 2g 0h 1h 2h]
        const __m256 aaabffgg = _mm256_permute2f128_ps(in0, in2, 0x20);
        const __m256 cdddeeef = in1;
        const __m256 bbccghhh = _mm256_permute2f128_ps(in0, in2, 0x31);
        // blend:
        // 0: [a d c b e h g f]
        // 1: [b a d c f e h g]
        // 2: [c b a d g f e h]
        const __m256 x0 = _mm256_blend_ps(
            _mm256_blend_ps(aaabffgg, cdddeeef, 0 + 2 + 0 + 0 + 0x10 + 0 + 0 + 0x80),
            bbccghhh, 0 + 0 + 4 + 0 + 0 + 0x20 + 0 + 0);
        const __m256 x1 = _mm256_blend_ps(
            _mm256_blend_ps(aaabffgg, cdddeeef, 0 + 0 + 4 + 0 + 0 + 0x20 + 0 + 0),
            bbccghhh, 1 + 0 + 0 + 8 + 0 + 0 + 0x40 + 0);
        const __m256 x2 = _mm256_blend_ps(
            _mm256_blend_ps(aaabffgg, cdddeeef, 1 + 0 + 0 + 8 + 0 + 0 + 0x40 + 0),
            bbccghhh, 0 + 2 + 0 + 0 + 0x10 + 0 + 0 + 0x80);
        // 0: [a d c b e h g f] >-perm(0, 3, 2, 1)-> [a b c d e f g h]
        // 1: [b a d c f e h g] >-perm(1, 0, 3, 2)-> [a b c d e f g h]
        // 2: [c b a d g f e h] >-perm(2, 1, 0, 3)-> [a b c d e f g h]
        v0 = AVX::avx_cast<typename V::VectorType>(_mm256_shuffle_ps(x0, x0, 0x6c));
        v1 = AVX::avx_cast<typename V::VectorType>(_mm256_shuffle_ps(x1, x1, 0xb1));
        v2 = AVX::avx_cast<typename V::VectorType>(_mm256_shuffle_ps(x2, x2, 0xc6));
    }  // }}}
    // deinterleave scatter 4 {{{
    template <typename I>
    static inline void deinterleave(typename V::EntryType const *const data, const I &i,
                                    V &v0, V &v1, V &v2, V &v3)
    {
        using namespace AVX;
        const m128  il0 = _mm_loadu_ps(aliasing_cast<float>(&data[i[0]])); // a0 b0 c0 d0
        const m128  il1 = _mm_loadu_ps(aliasing_cast<float>(&data[i[1]])); // a1 b1 c1 d1
        const m128  il2 = _mm_loadu_ps(aliasing_cast<float>(&data[i[2]])); // a2 b2 c2 d2
        const m128  il3 = _mm_loadu_ps(aliasing_cast<float>(&data[i[3]])); // a3 b3 c3 d3
        const m128  il4 = _mm_loadu_ps(aliasing_cast<float>(&data[i[4]])); // a4 b4 c4 d4
        const m128  il5 = _mm_loadu_ps(aliasing_cast<float>(&data[i[5]])); // a5 b5 c5 d5
        const m128  il6 = _mm_loadu_ps(aliasing_cast<float>(&data[i[6]])); // a6 b6 c6 d6
        const m128  il7 = _mm_loadu_ps(aliasing_cast<float>(&data[i[7]])); // a7 b7 c7 d7

        const m256 il04 = concat(il0, il4);
        const m256 il15 = concat(il1, il5);
        const m256 il26 = concat(il2, il6);
        const m256 il37 = concat(il3, il7);
        const m256 ab0246 = _mm256_unpacklo_ps(il04, il26);
        const m256 ab1357 = _mm256_unpacklo_ps(il15, il37);
        const m256 cd0246 = _mm256_unpackhi_ps(il04, il26);
        const m256 cd1357 = _mm256_unpackhi_ps(il15, il37);
        v0.data() = avx_cast<typename V::VectorType>(_mm256_unpacklo_ps(ab0246, ab1357));
        v1.data() = avx_cast<typename V::VectorType>(_mm256_unpackhi_ps(ab0246, ab1357));
        v2.data() = avx_cast<typename V::VectorType>(_mm256_unpacklo_ps(cd0246, cd1357));
        v3.data() = avx_cast<typename V::VectorType>(_mm256_unpackhi_ps(cd0246, cd1357));
    }  // }}}
    // deinterleave successive 4 {{{
    static inline void deinterleave(typename V::EntryType const *const data,
                                    const Common::SuccessiveEntries<4> &i, V &v0, V &v1,
                                    V &v2, V &v3)
    {
        using namespace AVX;
        const __m256 il01 = _mm256_loadu_ps(
            aliasing_cast<float>(&data[i[0]]));  // a0 b0 c0 d0 | a1 b1 c1 d1
        const __m256 il23 = _mm256_loadu_ps(
            aliasing_cast<float>(&data[i[2]]));  // a2 b2 c2 d2 | a3 b3 c3 d3
        const __m256 il45 = _mm256_loadu_ps(
            aliasing_cast<float>(&data[i[4]]));  // a4 b4 c4 d4 | a5 b5 c5 d5
        const __m256 il67 = _mm256_loadu_ps(
            aliasing_cast<float>(&data[i[6]]));  // a6 b6 c6 d6 | a7 b7 c7 d7

        const __m256 il04 = _mm256_permute2f128_ps(il01, il45, 0x20);
        const __m256 il15 = _mm256_permute2f128_ps(il01, il45, 0x31);
        const __m256 il26 = _mm256_permute2f128_ps(il23, il67, 0x20);
        const __m256 il37 = _mm256_permute2f128_ps(il23, il67, 0x31);
        const __m256 ab0246 = _mm256_unpacklo_ps(il04, il26);
        const __m256 ab1357 = _mm256_unpacklo_ps(il15, il37);
        const __m256 cd0246 = _mm256_unpackhi_ps(il04, il26);
        const __m256 cd1357 = _mm256_unpackhi_ps(il15, il37);
        v0.data() = avx_cast<typename V::VectorType>(_mm256_unpacklo_ps(ab0246, ab1357));
        v1.data() = avx_cast<typename V::VectorType>(_mm256_unpackhi_ps(ab0246, ab1357));
        v2.data() = avx_cast<typename V::VectorType>(_mm256_unpacklo_ps(cd0246, cd1357));
        v3.data() = avx_cast<typename V::VectorType>(_mm256_unpackhi_ps(cd0246, cd1357));
    }  // }}}
    template<typename I> static inline void deinterleave(typename V::EntryType const *const data,/*{{{*/
            const I &i, V &v0, V &v1, V &v2, V &v3, V &v4)
    {
        v4.gather(data + 4, i);
        deinterleave(data, i, v0, v1, v2, v3);
    }/*}}}*/
    template<typename I> static inline void deinterleave(typename V::EntryType const *const data,/*{{{*/
            const I &i, V &v0, V &v1, V &v2, V &v3, V &v4, V &v5)
    {
        deinterleave(data, i, v0, v1, v2, v3);
        deinterleave(data + 4, i, v4, v5);
    }/*}}}*/
    static inline void deinterleave(typename V::EntryType const *const data,/*{{{*/
            const Common::SuccessiveEntries<6> &i, V &v0, V &v1, V &v2, V &v3, V &v4, V &v5)
    {
        using namespace AVX;
        const m256 a = _mm256_loadu_ps(aliasing_cast<float>(&data[i[0]]));
        const m256 b = _mm256_loadu_ps(aliasing_cast<float>(&data[i[0] + 1 * V::Size]));
        const m256 c = _mm256_loadu_ps(aliasing_cast<float>(&data[i[0] + 2 * V::Size]));
        const m256 d = _mm256_loadu_ps(aliasing_cast<float>(&data[i[0] + 3 * V::Size]));
        const m256 e = _mm256_loadu_ps(aliasing_cast<float>(&data[i[0] + 4 * V::Size]));
        const m256 f = _mm256_loadu_ps(aliasing_cast<float>(&data[i[0] + 5 * V::Size]));
        const __m256 tmp2 = Mem::shuffle128<X0, Y0>(a, d);
        const __m256 tmp3 = Mem::shuffle128<X1, Y1>(b, e);
        const __m256 tmp4 = Mem::shuffle128<X1, Y1>(a, d);
        const __m256 tmp5 = Mem::shuffle128<X0, Y0>(c, f);
        const __m256 tmp8 = Mem::shuffle128<X0, Y0>(b, e);
        const __m256 tmp9 = Mem::shuffle128<X1, Y1>(c, f);
        const __m256 tmp0 = _mm256_unpacklo_ps(tmp2, tmp3);
        const __m256 tmp1 = _mm256_unpackhi_ps(tmp4, tmp5);
        const __m256 tmp6 = _mm256_unpackhi_ps(tmp2, tmp3);
        const __m256 tmp7 = _mm256_unpacklo_ps(tmp8, tmp9);
        const __m256 tmp10 = _mm256_unpacklo_ps(tmp4, tmp5);
        const __m256 tmp11 = _mm256_unpackhi_ps(tmp8, tmp9);
        v0.data() = avx_cast<typename V::VectorType>(_mm256_unpacklo_ps(tmp0, tmp1));
        v1.data() = avx_cast<typename V::VectorType>(_mm256_unpackhi_ps(tmp0, tmp1));
        v2.data() = avx_cast<typename V::VectorType>(_mm256_unpacklo_ps(tmp6, tmp7));
        v3.data() = avx_cast<typename V::VectorType>(_mm256_unpackhi_ps(tmp6, tmp7));
        v4.data() = avx_cast<typename V::VectorType>(_mm256_unpacklo_ps(tmp10, tmp11));
        v5.data() = avx_cast<typename V::VectorType>(_mm256_unpackhi_ps(tmp10, tmp11));
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
template<typename V> struct InterleaveImpl<V, 4, 32> {
    template <typename I>  // interleave 2 args{{{2
    static inline void interleave(typename V::EntryType *const data, const I &i,
                                  const typename V::AsArg v0, const typename V::AsArg v1)
    {
        using namespace AVX;
        const m256d tmp0 = _mm256_unpacklo_pd(v0.data(), v1.data());
        const m256d tmp1 = _mm256_unpackhi_pd(v0.data(), v1.data());
        _mm_storeu_pd(&data[i[0]], lo128(tmp0));
        _mm_storeu_pd(&data[i[1]], lo128(tmp1));
        _mm_storeu_pd(&data[i[2]], hi128(tmp0));
        _mm_storeu_pd(&data[i[3]], hi128(tmp1));
    }
    template <typename I>  // interleave 3 args{{{2
    static inline void interleave(typename V::EntryType *const data, const I &i,
                                  const typename V::AsArg v0, const typename V::AsArg v1,
                                  const typename V::AsArg v2)
    {
        using namespace AVX;
#ifdef Vc_USE_MASKMOV_SCATTER
        const m256d tmp0 = _mm256_unpacklo_pd(v0.data(), v1.data());
        const m256d tmp1 = _mm256_unpackhi_pd(v0.data(), v1.data());
        const m256d tmp2 = _mm256_unpacklo_pd(v2.data(), v2.data());
        const m256d tmp3 = _mm256_unpackhi_pd(v2.data(), v2.data());

#if defined(Vc_MSVC) && (Vc_MSVC < 170000000 || !defined(_WIN64))
        // MSVC needs to be at Version 2012 before _mm256_set_epi64x works
        const m256i mask = concat(_mm_setallone_si128(), _mm_set_epi32(0, 0, -1, -1));
#else
        const m256i mask = _mm256_set_epi64x(0, -1, -1, -1);
#endif
        _mm256_maskstore_pd(&data[i[0]], mask, Mem::shuffle128<X0, Y0>(tmp0, tmp2));
        _mm256_maskstore_pd(&data[i[1]], mask, Mem::shuffle128<X0, Y0>(tmp1, tmp3));
        _mm256_maskstore_pd(&data[i[2]], mask, Mem::shuffle128<X1, Y1>(tmp0, tmp2));
        _mm256_maskstore_pd(&data[i[3]], mask, Mem::shuffle128<X1, Y1>(tmp1, tmp3));
#else
        interleave(data, i, v0, v1);
        v2.scatter(data + 2, i);
#endif
    }
    template <typename I>  // interleave 4 args{{{2
    static inline void interleave(typename V::EntryType *const data, const I &i,
                                  const typename V::AsArg v0, const typename V::AsArg v1,
                                  const typename V::AsArg v2, const typename V::AsArg v3)
    {
        using namespace AVX;
        // 0a 1a 0c 1c:
        const m256d tmp0 = _mm256_unpacklo_pd(v0.data(), v1.data());
        // 0b 1b 0b 1b:
        const m256d tmp1 = _mm256_unpackhi_pd(v0.data(), v1.data());
        // 2a 3a 2c 3c:
        const m256d tmp2 = _mm256_unpacklo_pd(v2.data(), v3.data());
        // 2b 3b 2b 3b:
        const m256d tmp3 = _mm256_unpackhi_pd(v2.data(), v3.data());
        /* The following might be more efficient once 256-bit stores are not split internally into 2
         * 128-bit stores.
        _mm256_storeu_pd(&data[i[0]], Mem::shuffle128<X0, Y0>(tmp0, tmp2));
        _mm256_storeu_pd(&data[i[1]], Mem::shuffle128<X0, Y0>(tmp1, tmp3));
        _mm256_storeu_pd(&data[i[2]], Mem::shuffle128<X1, Y1>(tmp0, tmp2));
        _mm256_storeu_pd(&data[i[3]], Mem::shuffle128<X1, Y1>(tmp1, tmp3));
        */
        _mm_storeu_pd(&data[i[0]  ], lo128(tmp0));
        _mm_storeu_pd(&data[i[0]+2], lo128(tmp2));
        _mm_storeu_pd(&data[i[1]  ], lo128(tmp1));
        _mm_storeu_pd(&data[i[1]+2], lo128(tmp3));
        _mm_storeu_pd(&data[i[2]  ], hi128(tmp0));
        _mm_storeu_pd(&data[i[2]+2], hi128(tmp2));
        _mm_storeu_pd(&data[i[3]  ], hi128(tmp1));
        _mm_storeu_pd(&data[i[3]+2], hi128(tmp3));
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
        using namespace Vc::AVX;
        const m256d ab02 = concat(_mm_loadu_pd(&data[i[0]]), _mm_loadu_pd(&data[i[2]]));
        const m256d ab13 = concat(_mm_loadu_pd(&data[i[1]]), _mm_loadu_pd(&data[i[3]]));

        v0.data() = _mm256_unpacklo_pd(ab02, ab13);
        v1.data() = _mm256_unpackhi_pd(ab02, ab13);
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
        v4.gather(data + 4, i);
        deinterleave(data, i, v0, v1);
        deinterleave(data + 2, i, v2, v3);
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
        v6.gather(data + 6, i);
        deinterleave(data, i, v0, v1);
        deinterleave(data + 2, i, v2, v3);
        deinterleave(data + 4, i, v4, v5);
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

#endif  // VC_AVX_DETAIL_H_

// vim: foldmethod=marker
