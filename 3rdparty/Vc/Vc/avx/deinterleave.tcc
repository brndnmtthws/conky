/*  This file is part of the Vc library. {{{
Copyright © 2010-2015 Matthias Kretz <kretz@kde.org>

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
namespace AVX2
{

inline void deinterleave(double_v &Vc_RESTRICT a, double_v &Vc_RESTRICT b, double_v &Vc_RESTRICT c)
{   // estimated latency (AVX): 4.5 cycles
    const m256d tmp0 = Mem::shuffle128<X0, Y1>(a.data(), b.data());
    const m256d tmp1 = Mem::shuffle128<X1, Y0>(a.data(), c.data());
    const m256d tmp2 = Mem::shuffle128<X0, Y1>(b.data(), c.data());
    a.data() = Mem::shuffle<X0, Y1, X2, Y3>(tmp0, tmp1);
    b.data() = Mem::shuffle<X1, Y0, X3, Y2>(tmp0, tmp2);
    c.data() = Mem::shuffle<X0, Y1, X2, Y3>(tmp1, tmp2);
}

inline void deinterleave(float_v &Vc_RESTRICT a, float_v &Vc_RESTRICT b, float_v &Vc_RESTRICT c)
{
    //                               abc   abc abc
    // a = [a0 b0 c0 a1 b1 c1 a2 b2] 332 = 211+121
    // b = [c2 a3 b3 c3 a4 b4 c4 a5] 323 = 112+211
    // c = [b5 c5 a6 b6 c6 a7 b7 c7] 233 = 121+112
    const m256 ac0 = Mem::shuffle128<X0, Y0>(a.data(), c.data()); // a0 b0 c0 a1 b5 c5 a6 b6
    const m256 ac1 = Mem::shuffle128<X1, Y1>(a.data(), c.data()); // b1 c1 a2 b2 c6 a7 b7 c7

    m256 tmp0 = Mem::blend<X0, Y1, X2, X3, Y4, X5, X6, Y7>( ac0, b.data());
           tmp0 = Mem::blend<X0, X1, Y2, X3, X4, Y5, X6, X7>(tmp0,      ac1); // a0 a3 a2 a1 a4 a7 a6 a5
    m256 tmp1 = Mem::blend<X0, X1, Y2, X3, X4, Y5, X6, X7>( ac0, b.data());
           tmp1 = Mem::blend<Y0, X1, X2, Y3, X4, X5, Y6, X7>(tmp1,      ac1); // b1 b0 b3 b2 b5 b4 b7 b6
    m256 tmp2 = Mem::blend<Y0, X1, X2, Y3, X4, X5, Y6, X7>( ac0, b.data());
           tmp2 = Mem::blend<X0, Y1, X2, X3, Y4, X5, X6, Y7>(tmp2,      ac1); // c2 c1 c0 c3 c6 c5 c4 c7

    a.data() = Mem::permute<X0, X3, X2, X1>(tmp0);
    b.data() = Mem::permute<X1, X0, X3, X2>(tmp1);
    c.data() = Mem::permute<X2, X1, X0, X3>(tmp2);
}

inline void deinterleave(int_v &Vc_RESTRICT a, int_v &Vc_RESTRICT b, int_v &Vc_RESTRICT c)
{
    deinterleave(reinterpret_cast<float_v &>(a), reinterpret_cast<float_v &>(b),
            reinterpret_cast<float_v &>(c));
}

inline void deinterleave(uint_v &Vc_RESTRICT a, uint_v &Vc_RESTRICT b, uint_v &Vc_RESTRICT c)
{
    deinterleave(reinterpret_cast<float_v &>(a), reinterpret_cast<float_v &>(b),
            reinterpret_cast<float_v &>(c));
}

inline void deinterleave(Vector<short> &Vc_RESTRICT , Vector<short> &Vc_RESTRICT ,
        Vector<short> &Vc_RESTRICT )
{
    return;
    /* TODO:
    //                               abc   abc abc
    // a = [a0 b0 c0 a1 b1 c1 a2 b2] 332 = 211+121
    // b = [c2 a3 b3 c3 a4 b4 c4 a5] 323 = 112+211
    // c = [b5 c5 a6 b6 c6 a7 b7 c7] 233 = 121+112
    m128i ac0 = _mm_unpacklo_epi64(a.data(), c.data()); // a0 b0 c0 a1 b5 c5 a6 b6
    m128i ac1 = _mm_unpackhi_epi64(a.data(), c.data()); // b1 c1 a2 b2 c6 a7 b7 c7

    m128i tmp0 = Mem::blend<X0, Y1, X2, X3, Y4, X5, X6, Y7>( ac0, b.data());
            tmp0 = Mem::blend<X0, X1, Y2, X3, X4, Y5, X6, X7>(tmp0,      ac1); // a0 a3 a2 a1 a4 a7 a6 a5
    m128i tmp1 = Mem::blend<X0, X1, Y2, X3, X4, Y5, X6, X7>( ac0, b.data());
            tmp1 = Mem::blend<Y0, X1, X2, Y3, X4, X5, Y6, X7>(tmp1,      ac1); // b1 b0 b3 b2 b5 b4 b7 b6
    m128i tmp2 = Mem::blend<Y0, X1, X2, Y3, X4, X5, Y6, X7>( ac0, b.data());
            tmp2 = Mem::blend<X0, Y1, X2, X3, Y4, X5, X6, Y7>(tmp2,      ac1); // c2 c1 c0 c3 c6 c5 c4 c7

    a.data() = Mem::permuteHi<X4, X7, X6, X5>(Mem::permuteLo<X0, X3, X2, X1>(tmp0));
    b.data() = Mem::permuteHi<X5, X4, X7, X6>(Mem::permuteLo<X1, X0, X3, X2>(tmp1));
    c.data() = Mem::permuteHi<X6, X5, X4, X7>(Mem::permuteLo<X2, X1, X0, X3>(tmp2));
    */
}

inline void deinterleave(Vector<unsigned short> &Vc_RESTRICT a, Vector<unsigned short> &Vc_RESTRICT b,
        Vector<unsigned short> &Vc_RESTRICT c)
{
    deinterleave(reinterpret_cast<Vector<short> &>(a), reinterpret_cast<Vector<short> &>(b),
            reinterpret_cast<Vector<short> &>(c));
}

inline void deinterleave(Vector<float> &a, Vector<float> &b)
{
    // a7 a6 a5 a4 a3 a2 a1 a0
    // b7 b6 b5 b4 b3 b2 b1 b0
    const m256 tmp0 = Reg::permute128<Y0, X0>(a.data(), b.data()); // b3 b2 b1 b0 a3 a2 a1 a0
    const m256 tmp1 = Reg::permute128<Y1, X1>(a.data(), b.data()); // b7 b6 b5 b4 a7 a6 a5 a4

    const m256 tmp2 = _mm256_unpacklo_ps(tmp0, tmp1); // b5 b1 b4 b0 a5 a1 a4 a0
    const m256 tmp3 = _mm256_unpackhi_ps(tmp0, tmp1); // b7 b3 b6 b2 a7 a3 a6 a2

    a.data() = _mm256_unpacklo_ps(tmp2, tmp3); // b6 b4 b2 b0 a6 a4 a2 a0
    b.data() = _mm256_unpackhi_ps(tmp2, tmp3); // b7 b5 b3 b1 a7 a5 a3 a1
}

inline void deinterleave(Vector<short> &a, // a0 b0 a1 b1 a2 b2 a3 b3 | a4 b4 a5 ...
                         Vector<short> &b) // a8 b8 a9 ...
{
    auto v0 = Mem::shuffle128<X0, Y0>(a.data(), b.data());
    auto v1 = Mem::shuffle128<X1, Y1>(a.data(), b.data());
    auto v2 = AVX::unpacklo_epi16(v0, v1); // a0 a4 ...
    auto v3 = AVX::unpackhi_epi16(v0, v1); // a2 a6 ...
    v0 = AVX::unpacklo_epi16(v2, v3); // a0 a2 ...
    v1 = AVX::unpackhi_epi16(v2, v3); // a1 a3 ...
    a.data() = AVX::unpacklo_epi16(v0, v1); // a0 a1 ...
    b.data() = AVX::unpackhi_epi16(v0, v1); // b0 b1 ...
}

inline void deinterleave(Vector<ushort> &a, Vector<ushort> &b)
{
    auto v0 = Mem::shuffle128<X0, Y0>(a.data(), b.data());
    auto v1 = Mem::shuffle128<X1, Y1>(a.data(), b.data());
    auto v2 = AVX::unpacklo_epi16(v0, v1); // a0 a4 ...
    auto v3 = AVX::unpackhi_epi16(v0, v1); // a2 a6 ...
    v0 = AVX::unpacklo_epi16(v2, v3); // a0 a2 ...
    v1 = AVX::unpackhi_epi16(v2, v3); // a1 a3 ...
    a.data() = AVX::unpacklo_epi16(v0, v1); // a0 a1 ...
    b.data() = AVX::unpackhi_epi16(v0, v1); // b0 b1 ...
}

}  // namespace AVX2
namespace Detail
{
template <typename Flags>
inline void deinterleave(AVX2::float_v &a, AVX2::float_v &b, const float *m, Flags align)
{
    a.load(m, align);
    b.load(m + AVX2::float_v::Size, align);
    Vc::AVX2::deinterleave(a, b);
}

template <typename Flags>
inline void deinterleave(AVX2::float_v &a, AVX2::float_v &b, const short *m, Flags f)
{
    using namespace Vc::AVX2;
    const auto tmp = Detail::load32(m, f);
    a.data() =
        _mm256_cvtepi32_ps(concat(_mm_srai_epi32(_mm_slli_epi32(lo128(tmp), 16), 16),
                                  _mm_srai_epi32(_mm_slli_epi32(hi128(tmp), 16), 16)));
    b.data() = _mm256_cvtepi32_ps(
        concat(_mm_srai_epi32(lo128(tmp), 16), _mm_srai_epi32(hi128(tmp), 16)));
}

template <typename Flags>
inline void deinterleave(AVX2::float_v &a, AVX2::float_v &b, const unsigned short *m, Flags f)
{
    using namespace Vc::AVX2;
    const auto tmp = Detail::load32(m, f);
    a.data() = _mm256_cvtepi32_ps(
        concat(_mm_blend_epi16(lo128(tmp), _mm_setzero_si128(), 0xaa),
               _mm_blend_epi16(hi128(tmp), _mm_setzero_si128(), 0xaa)));
    b.data() = _mm256_cvtepi32_ps(
        concat(_mm_srli_epi32(lo128(tmp), 16), _mm_srli_epi32(hi128(tmp), 16)));
}

template <typename Flags>
inline void deinterleave(AVX2::double_v &a, AVX2::double_v &b, const double *m, Flags align)
{
    using namespace Vc::AVX2;

    a.load(m, align);
    b.load(m + AVX2::double_v::Size, align);

    m256d tmp0 = Mem::shuffle128<Vc::X0, Vc::Y0>(a.data(), b.data());  // b1 b0 a1 a0
    m256d tmp1 = Mem::shuffle128<Vc::X1, Vc::Y1>(a.data(), b.data());  // b3 b2 a3 a2

    a.data() = _mm256_unpacklo_pd(tmp0, tmp1);  // b2 b0 a2 a0
    b.data() = _mm256_unpackhi_pd(tmp0, tmp1);  // b3 b1 a3 a1
}

template <typename Flags>
inline void deinterleave(AVX2::int_v &a, AVX2::int_v &b, const int *m, Flags align)
{
    using namespace AVX;
    a.load(m, align);
    b.load(m + AVX2::int_v::Size, align);

    const m256 tmp0 = avx_cast<m256>(Mem::shuffle128<Vc::X0, Vc::Y0>(a.data(), b.data()));
    const m256 tmp1 = avx_cast<m256>(Mem::shuffle128<Vc::X1, Vc::Y1>(a.data(), b.data()));

    const m256 tmp2 = _mm256_unpacklo_ps(tmp0, tmp1); // b5 b1 b4 b0 a5 a1 a4 a0
    const m256 tmp3 = _mm256_unpackhi_ps(tmp0, tmp1); // b7 b3 b6 b2 a7 a3 a6 a2

    a.data() = avx_cast<m256i>(_mm256_unpacklo_ps(tmp2, tmp3)); // b6 b4 b2 b0 a6 a4 a2 a0
    b.data() = avx_cast<m256i>(_mm256_unpackhi_ps(tmp2, tmp3)); // b7 b5 b3 b1 a7 a5 a3 a1
}

template <typename Flags>
inline void deinterleave(AVX2::int_v &a, AVX2::int_v &b, const short *m, Flags f)
{
    using namespace Vc::AVX;
    const AVX2::short_v tmp0(m, f);
    const m256i tmp = tmp0.data();
    a.data() = concat(
                _mm_srai_epi32(_mm_slli_epi32(lo128(tmp), 16), 16),
                _mm_srai_epi32(_mm_slli_epi32(hi128(tmp), 16), 16));
    b.data() = concat(
                _mm_srai_epi32(lo128(tmp), 16),
                _mm_srai_epi32(hi128(tmp), 16));
}

template <typename Flags>
inline void deinterleave(AVX2::uint_v &a, AVX2::uint_v &b, const unsigned int *m, Flags align)
{
    using namespace AVX;
    a.load(m, align);
    b.load(m + AVX2::uint_v::Size, align);

    const m256 tmp0 = avx_cast<m256>(Mem::shuffle128<Vc::X0, Vc::Y0>(a.data(), b.data()));
    const m256 tmp1 = avx_cast<m256>(Mem::shuffle128<Vc::X1, Vc::Y1>(a.data(), b.data()));

    const m256 tmp2 = _mm256_unpacklo_ps(tmp0, tmp1); // b5 b1 b4 b0 a5 a1 a4 a0
    const m256 tmp3 = _mm256_unpackhi_ps(tmp0, tmp1); // b7 b3 b6 b2 a7 a3 a6 a2

    a.data() = avx_cast<m256i>(_mm256_unpacklo_ps(tmp2, tmp3)); // b6 b4 b2 b0 a6 a4 a2 a0
    b.data() = avx_cast<m256i>(_mm256_unpackhi_ps(tmp2, tmp3)); // b7 b5 b3 b1 a7 a5 a3 a1
}

template <typename Flags>
inline void deinterleave(AVX2::uint_v &a, AVX2::uint_v &b, const unsigned short *m, Flags f)
{
    using namespace Vc::AVX;
    const AVX2::ushort_v tmp0(m, f);
    const m256i tmp = tmp0.data();
    a.data() = concat(
                _mm_srai_epi32(_mm_slli_epi32(lo128(tmp), 16), 16),
                _mm_srai_epi32(_mm_slli_epi32(hi128(tmp), 16), 16));
    b.data() = concat(
                _mm_srai_epi32(lo128(tmp), 16),
                _mm_srai_epi32(hi128(tmp), 16));
}

template <typename Flags>
inline void deinterleave(AVX2::short_v &a, AVX2::short_v &b, const short *m, Flags align)
{
    a.load(m, align);
    b.load(m + AVX2::short_v::Size, align);
    Vc::AVX2::deinterleave(a, b);
}

template <typename Flags>
inline void deinterleave(AVX2::ushort_v &a, AVX2::ushort_v &b, const unsigned short *m, Flags align)
{
    a.load(m, align);
    b.load(m + AVX2::ushort_v::Size, align);
    Vc::AVX2::deinterleave(a, b);
}

// only support M == V::EntryType -> no specialization
template <typename T, typename M, typename Flags>
Vc_ALWAYS_INLINE void deinterleave(AVX2::Vector<T> &Vc_RESTRICT a,
                                   AVX2::Vector<T> &Vc_RESTRICT b,
                                   AVX2::Vector<T> &Vc_RESTRICT c,
                                   const M *Vc_RESTRICT memory, Flags align)
{
    using V = AVX2::Vector<T>;
    a.load(&memory[0 * V::Size], align);
    b.load(&memory[1 * V::Size], align);
    c.load(&memory[2 * V::Size], align);
    Vc::AVX2::deinterleave(a, b, c);
}

}  // namespace Detail
}  // namespace Vc
