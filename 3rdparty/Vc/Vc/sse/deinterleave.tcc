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
namespace SSE
{

inline void deinterleave(Vector<float> &a, Vector<float> &b)
{
    const __m128 tmp0 = _mm_unpacklo_ps(a.data(), b.data());
    const __m128 tmp1 = _mm_unpackhi_ps(a.data(), b.data());
    a.data() = _mm_unpacklo_ps(tmp0, tmp1);
    b.data() = _mm_unpackhi_ps(tmp0, tmp1);
}

inline void deinterleave(Vector<float> &a, Vector<float> &b, Vector<short>::AsArg tmp)
{
    a.data() = _mm_cvtepi32_ps(_mm_srai_epi32(_mm_slli_epi32(tmp.data(), 16), 16));
    b.data() = _mm_cvtepi32_ps(_mm_srai_epi32(tmp.data(), 16));
}

inline void deinterleave(Vector<float> &a, Vector<float> &b, Vector<unsigned short>::AsArg tmp)
{
    a.data() = _mm_cvtepi32_ps(_mm_srli_epi32(_mm_slli_epi32(tmp.data(), 16), 16));
    b.data() = _mm_cvtepi32_ps(_mm_srli_epi32(tmp.data(), 16));
}

inline void deinterleave(Vector<double> &a, Vector<double> &b)
{
    __m128d tmp = _mm_unpacklo_pd(a.data(), b.data());
    b.data() = _mm_unpackhi_pd(a.data(), b.data());
    a.data() = tmp;
}

inline void deinterleave(Vector<int> &a, Vector<int> &b)
{
    const __m128i tmp0 = _mm_unpacklo_epi32(a.data(), b.data());
    const __m128i tmp1 = _mm_unpackhi_epi32(a.data(), b.data());
    a.data() = _mm_unpacklo_epi32(tmp0, tmp1);
    b.data() = _mm_unpackhi_epi32(tmp0, tmp1);
}

inline void deinterleave(Vector<unsigned int> &a, Vector<unsigned int> &b)
{
    const __m128i tmp0 = _mm_unpacklo_epi32(a.data(), b.data());
    const __m128i tmp1 = _mm_unpackhi_epi32(a.data(), b.data());
    a.data() = _mm_unpacklo_epi32(tmp0, tmp1);
    b.data() = _mm_unpackhi_epi32(tmp0, tmp1);
}

inline void deinterleave(Vector<short> &a, Vector<short> &b)
{
    __m128i tmp0 = _mm_unpacklo_epi16(a.data(), b.data()); // a0 a4 b0 b4 a1 a5 b1 b5
    __m128i tmp1 = _mm_unpackhi_epi16(a.data(), b.data()); // a2 a6 b2 b6 a3 a7 b3 b7
    __m128i tmp2 = _mm_unpacklo_epi16(tmp0, tmp1); // a0 a2 a4 a6 b0 b2 b4 b6
    __m128i tmp3 = _mm_unpackhi_epi16(tmp0, tmp1); // a1 a3 a5 a7 b1 b3 b5 b7
    a.data() = _mm_unpacklo_epi16(tmp2, tmp3);
    b.data() = _mm_unpackhi_epi16(tmp2, tmp3);
}

inline void deinterleave(Vector<unsigned short> &a, Vector<unsigned short> &b)
{
    __m128i tmp0 = _mm_unpacklo_epi16(a.data(), b.data()); // a0 a4 b0 b4 a1 a5 b1 b5
    __m128i tmp1 = _mm_unpackhi_epi16(a.data(), b.data()); // a2 a6 b2 b6 a3 a7 b3 b7
    __m128i tmp2 = _mm_unpacklo_epi16(tmp0, tmp1); // a0 a2 a4 a6 b0 b2 b4 b6
    __m128i tmp3 = _mm_unpackhi_epi16(tmp0, tmp1); // a1 a3 a5 a7 b1 b3 b5 b7
    a.data() = _mm_unpacklo_epi16(tmp2, tmp3);
    b.data() = _mm_unpackhi_epi16(tmp2, tmp3);
}

inline void deinterleave(Vector<int> &a, Vector<int> &b, Vector<short>::AsArg tmp)
{
    a.data() = _mm_srai_epi32(_mm_slli_epi32(tmp.data(), 16), 16);
    b.data() = _mm_srai_epi32(tmp.data(), 16);
}

inline void deinterleave(Vector<unsigned int> &a, Vector<unsigned int> &b, Vector<unsigned short>::AsArg tmp)
{
    a.data() = _mm_srli_epi32(_mm_slli_epi32(tmp.data(), 16), 16);
    b.data() = _mm_srli_epi32(tmp.data(), 16);
}

}
}
namespace Vc_VERSIONED_NAMESPACE
{
namespace Detail
{
template<typename A> inline void deinterleave(
        SSE::float_v &a, SSE::float_v &b, const float *m, A align)
{
    a.load(m, align);
    b.load(m + SSE::float_v::Size, align);
    Vc::SSE::deinterleave(a, b);
}

template<typename A> inline void deinterleave(
        SSE::float_v &a, SSE::float_v &b, const short *m, A align)
{
    SSE::short_v tmp(m, align);
    Vc::SSE::deinterleave(a, b, tmp);
}

template<typename A> inline void deinterleave(
        SSE::float_v &a, SSE::float_v &b, const unsigned short *m, A align)
{
    SSE::ushort_v tmp(m, align);
    Vc::SSE::deinterleave(a, b, tmp);
}

template<typename A> inline void deinterleave(
        SSE::double_v &a, SSE::double_v &b, const double *m, A align)
{
    a.load(m, align);
    b.load(m + SSE::double_v::Size, align);
    Vc::SSE::deinterleave(a, b);
}

template<typename A> inline void deinterleave(
        SSE::int_v &a, SSE::int_v &b, const int *m, A align)
{
    a.load(m, align);
    b.load(m + SSE::int_v::Size, align);
    Vc::SSE::deinterleave(a, b);
}

template<typename A> inline void deinterleave(
        SSE::int_v &a, SSE::int_v &b, const short *m, A align)
{
    SSE::short_v tmp(m, align);
    Vc::SSE::deinterleave(a, b, tmp);
}

template<typename A> inline void deinterleave(
        SSE::uint_v &a, SSE::uint_v &b, const unsigned int *m, A align)
{
    a.load(m, align);
    b.load(m + SSE::uint_v::Size, align);
    Vc::SSE::deinterleave(a, b);
}

template<typename A> inline void deinterleave(
        SSE::uint_v &a, SSE::uint_v &b, const unsigned short *m, A align)
{
    SSE::ushort_v tmp(m, align);
    Vc::SSE::deinterleave(a, b, tmp);
}

template<typename A> inline void deinterleave(
        SSE::short_v &a, SSE::short_v &b, const short *m, A align)
{
    a.load(m, align);
    b.load(m + SSE::short_v::Size, align);
    Vc::SSE::deinterleave(a, b);
}

template<typename A> inline void deinterleave(
        SSE::ushort_v &a, SSE::ushort_v &b, const unsigned short *m, A align)
{
    a.load(m, align);
    b.load(m + SSE::ushort_v::Size, align);
    Vc::SSE::deinterleave(a, b);
}
}  // namespace Detail
}  // namespace Vc
