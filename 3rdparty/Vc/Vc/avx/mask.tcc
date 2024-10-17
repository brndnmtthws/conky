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

namespace Vc_VERSIONED_NAMESPACE
{
// store {{{1
template <typename T>
template <typename Flags>
Vc_INTRINSIC void Mask<T, VectorAbi::Avx>::store(bool *mem, Flags f) const
{
    Detail::mask_store<Size>(dataI(), mem, f);
}

// load {{{1
template <typename T>
template <typename Flags>
Vc_INTRINSIC void Mask<T, VectorAbi::Avx>::load(const bool *mem, Flags f)
{
    d.v() = AVX::avx_cast<VectorType>(Detail::mask_load<VectorTypeF, Size>(mem, f));
}

// operator[] {{{1
#ifdef Vc_IMPL_AVX2
template <>
Vc_INTRINSIC Vc_PURE bool AVX2::Mask<int16_t>::get(const AVX2::Mask<int16_t> &m,
                                                   int index) noexcept
{
    return m.shiftMask() & (1 << 2 * index);
}
template <>
Vc_INTRINSIC Vc_PURE bool AVX2::Mask<uint16_t>::get(const AVX2::Mask<uint16_t> &m,
                                                    int index) noexcept
{
    return m.shiftMask() & (1 << 2 * index);
}
#endif
// operator== {{{1
template <> Vc_INTRINSIC Vc_PURE bool AVX2::double_m::operator==(const AVX2::double_m &rhs) const
{ return Detail::movemask(dataD()) == Detail::movemask(rhs.dataD()); }
#ifdef Vc_IMPL_AVX2
template <> Vc_INTRINSIC Vc_PURE bool AVX2::short_m::operator==(const AVX2::short_m &rhs) const
{ return Detail::movemask(dataI()) == Detail::movemask(rhs.dataI()); }
template <> Vc_INTRINSIC Vc_PURE bool AVX2::ushort_m::operator==(const AVX2::ushort_m &rhs) const
{ return Detail::movemask(dataI()) == Detail::movemask(rhs.dataI()); }
#endif

// isFull, isNotEmpty, isEmpty, isMix specializations{{{1
template <typename T> Vc_INTRINSIC bool Mask<T, VectorAbi::Avx>::isFull() const {
    if (sizeof(T) == 8) {
        return 0 != Detail::testc(dataD(), Detail::allone<VectorTypeD>());
    } else if (sizeof(T) == 4) {
        return 0 != Detail::testc(dataF(), Detail::allone<VectorTypeF>());
    } else {
        return 0 != Detail::testc(dataI(), Detail::allone<VectorTypeI>());
    }
}

template <typename T> Vc_INTRINSIC bool Mask<T, VectorAbi::Avx>::isNotEmpty() const {
    if (sizeof(T) == 8) {
        return 0 == Detail::testz(dataD(), dataD());
    } else if (sizeof(T) == 4) {
        return 0 == Detail::testz(dataF(), dataF());
    } else {
        return 0 == Detail::testz(dataI(), dataI());
    }
}

template <typename T> Vc_INTRINSIC bool Mask<T, VectorAbi::Avx>::isEmpty() const {
    if (sizeof(T) == 8) {
        return 0 != Detail::testz(dataD(), dataD());
    } else if (sizeof(T) == 4) {
        return 0 != Detail::testz(dataF(), dataF());
    } else {
        return 0 != Detail::testz(dataI(), dataI());
    }
}

template <typename T> Vc_INTRINSIC bool Mask<T, VectorAbi::Avx>::isMix() const {
    if (sizeof(T) == 8) {
        return 0 != Detail::testnzc(dataD(), Detail::allone<VectorTypeD>());
    } else if (sizeof(T) == 4) {
        return 0 != Detail::testnzc(dataF(), Detail::allone<VectorTypeF>());
    } else {
        return 0 != Detail::testnzc(dataI(), Detail::allone<VectorTypeI>());
    }
}

// generate {{{1
template <typename M, typename G>
Vc_INTRINSIC M generate_impl(G &&gen, std::integral_constant<int, 4 + 32>)
{
    return _mm256_setr_epi64x(
        gen(0) ? 0xffffffffffffffffull : 0, gen(1) ? 0xffffffffffffffffull : 0,
        gen(2) ? 0xffffffffffffffffull : 0, gen(3) ? 0xffffffffffffffffull : 0);
}
template <typename M, typename G>
Vc_INTRINSIC M generate_impl(G &&gen, std::integral_constant<int, 8 + 32>)
{
    return _mm256_setr_epi32(gen(0) ? 0xfffffffful : 0, gen(1) ? 0xfffffffful : 0,
                             gen(2) ? 0xfffffffful : 0, gen(3) ? 0xfffffffful : 0,
                             gen(4) ? 0xfffffffful : 0, gen(5) ? 0xfffffffful : 0,
                             gen(6) ? 0xfffffffful : 0, gen(7) ? 0xfffffffful : 0);
}
template <typename M, typename G>
Vc_INTRINSIC M generate_impl(G &&gen, std::integral_constant<int, 16 + 32>)
{
    return _mm256_setr_epi16(gen(0) ? 0xfffful : 0, gen(1) ? 0xfffful : 0,
                             gen(2) ? 0xfffful : 0, gen(3) ? 0xfffful : 0,
                             gen(4) ? 0xfffful : 0, gen(5) ? 0xfffful : 0,
                             gen(6) ? 0xfffful : 0, gen(7) ? 0xfffful : 0,
                             gen(8) ? 0xfffful : 0, gen(9) ? 0xfffful : 0,
                             gen(10) ? 0xfffful : 0, gen(11) ? 0xfffful : 0,
                             gen(12) ? 0xfffful : 0, gen(13) ? 0xfffful : 0,
                             gen(14) ? 0xfffful : 0, gen(15) ? 0xfffful : 0);
}
template <typename T>
template <typename G>
Vc_INTRINSIC AVX2::Mask<T> Mask<T, VectorAbi::Avx>::generate(G &&gen)
{
    return generate_impl<AVX2::Mask<T>>(std::forward<G>(gen),
                                  std::integral_constant<int, Size + sizeof(Storage)>());
}
// shifted {{{1
template <typename T> Vc_INTRINSIC Vc_PURE AVX2::Mask<T> Mask<T, VectorAbi::Avx>::shifted(int amount) const
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
    case  17: return Detail::shifted< 17>(dataI());
    case  18: return Detail::shifted< 18>(dataI());
    case  19: return Detail::shifted< 19>(dataI());
    case  20: return Detail::shifted< 20>(dataI());
    case  21: return Detail::shifted< 21>(dataI());
    case  22: return Detail::shifted< 22>(dataI());
    case  23: return Detail::shifted< 23>(dataI());
    case  24: return Detail::shifted< 24>(dataI());
    case  25: return Detail::shifted< 25>(dataI());
    case  26: return Detail::shifted< 26>(dataI());
    case  27: return Detail::shifted< 27>(dataI());
    case  28: return Detail::shifted< 28>(dataI());
    case  29: return Detail::shifted< 29>(dataI());
    case  30: return Detail::shifted< 30>(dataI());
    case  31: return Detail::shifted< 31>(dataI());
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
    case -17: return Detail::shifted<-17>(dataI());
    case -18: return Detail::shifted<-18>(dataI());
    case -19: return Detail::shifted<-19>(dataI());
    case -20: return Detail::shifted<-20>(dataI());
    case -21: return Detail::shifted<-21>(dataI());
    case -22: return Detail::shifted<-22>(dataI());
    case -23: return Detail::shifted<-23>(dataI());
    case -24: return Detail::shifted<-24>(dataI());
    case -25: return Detail::shifted<-25>(dataI());
    case -26: return Detail::shifted<-26>(dataI());
    case -27: return Detail::shifted<-27>(dataI());
    case -28: return Detail::shifted<-28>(dataI());
    case -29: return Detail::shifted<-29>(dataI());
    case -30: return Detail::shifted<-30>(dataI());
    case -31: return Detail::shifted<-31>(dataI());
    }
    return Zero();
}
// }}}1

/*
template<> Vc_INTRINSIC AVX2::Mask< 4, 32> &AVX2::Mask< 4, 32>::operator=(const std::array<bool, 4> &values) {
    static_assert(sizeof(bool) == 1, "Vc expects bool to have a sizeof 1 Byte");
    unsigned int x = *reinterpret_cast<const unsigned int *>(values.data());
    x *= 0xffu;
    __m128i y = _mm_cvtsi32_si128(x); //  4 Bytes
    y = _mm_unpacklo_epi8(y, y);    //  8 Bytes
    y = _mm_unpacklo_epi16(y, y);   // 16 Bytes
    d.v() = AVX::avx_cast<__m256>(AVX::concat(_mm_unpacklo_epi32(y, y), _mm_unpackhi_epi32(y, y)));
    return *this;
}
template<> Vc_INTRINSIC AVX2::Mask< 8, 32> &AVX2::Mask< 8, 32>::operator=(const std::array<bool, 8> &values) {
    static_assert(sizeof(bool) == 1, "Vc expects bool to have a sizeof 1 Byte");
    unsigned long long x = *reinterpret_cast<const unsigned long long *>(values.data());
    x *= 0xffull;
    __m128i y = _mm_cvtsi64_si128(x); //  8 Bytes
    y = _mm_unpacklo_epi8(y, y);   // 16 Bytes
    d.v() = AVX::avx_cast<__m256>(AVX::concat(_mm_unpacklo_epi16(y, y), _mm_unpackhi_epi16(y, y)));
    return *this;
}
template<> Vc_INTRINSIC AVX2::Mask< 8, 16> &AVX2::Mask< 8, 16>::operator=(const std::array<bool, 8> &values) {
    static_assert(sizeof(bool) == 1, "Vc expects bool to have a sizeof 1 Byte");
    unsigned long long x = *reinterpret_cast<const unsigned long long *>(values.data());
    x *= 0xffull;
    __m128i y = _mm_cvtsi64_si128(x); //  8 Bytes
    d.v() = AVX::avx_cast<__m128>(_mm_unpacklo_epi8(y, y));
    return *this;
}
template<> Vc_INTRINSIC AVX2::Mask<16, 16> &AVX2::Mask<16, 16>::operator=(const std::array<bool, 16> &values) {
    static_assert(sizeof(bool) == 1, "Vc expects bool to have a sizeof 1 Byte");
    __m128i x = _mm_loadu_si128(reinterpret_cast<const __m128i *>(values.data()));
    d.v() = _mm_andnot_ps(AVX::_mm_setallone_ps(), AVX::avx_cast<__m128>(_mm_sub_epi8(x, _mm_set1_epi8(1))));
    return *this;
}

template<> Vc_INTRINSIC AVX2::Mask< 4, 32>::operator std::array<bool, 4>() const {
    static_assert(sizeof(bool) == 1, "Vc expects bool to have a sizeof 1 Byte");
    __m128i x = _mm_packs_epi32(AVX::lo128(dataI()), AVX::hi128(dataI())); // 64bit -> 32bit
    x = _mm_packs_epi32(x, x); // 32bit -> 16bit
    x = _mm_srli_epi16(x, 15);
    x = _mm_packs_epi16(x, x); // 16bit ->  8bit
    std::array<bool, 4> r;
    asm volatile("vmovd %1,%0" : "=m"(*r.data()) : "x"(x));
    return r;
}
template<> Vc_INTRINSIC AVX2::Mask< 8, 32>::operator std::array<bool, 8>() const {
    static_assert(sizeof(bool) == 1, "Vc expects bool to have a sizeof 1 Byte");
    __m128i x = _mm_packs_epi32(AVX::lo128(dataI()), AVX::hi128(dataI())); // 32bit -> 16bit
    x = _mm_srli_epi16(x, 15);
    x = _mm_packs_epi16(x, x); // 16bit ->  8bit
    std::array<bool, 8> r;
    asm volatile("vmovq %1,%0" : "=m"(*r.data()) : "x"(x));
    return r;
}
template<> Vc_INTRINSIC AVX2::Mask< 8, 16>::operator std::array<bool, 8>() const {
    static_assert(sizeof(bool) == 1, "Vc expects bool to have a sizeof 1 Byte");
    __m128i x = _mm_srli_epi16(dataI(), 15);
    x = _mm_packs_epi16(x, x); // 16bit ->  8bit
    std::array<bool, 8> r;
    asm volatile("vmovq %1,%0" : "=m"(*r.data()) : "x"(x));
    return r;
}
template<> Vc_INTRINSIC AVX2::Mask<16, 16>::operator std::array<bool, 16>() const {
    static_assert(sizeof(bool) == 1, "Vc expects bool to have a sizeof 1 Byte");
    __m128 x = _mm_and_ps(d.v(), AVX::avx_cast<__m128>(_mm_set1_epi32(0x01010101)));
    std::array<bool, 16> r;
    asm volatile("vmovups %1,%0" : "=m"(*r.data()) : "x"(x));
    return r;
}
*/

}

// vim: foldmethod=marker
