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

#ifndef VC_AVX_CONST_H_
#define VC_AVX_CONST_H_

#include <cstddef>
#include "types.h"
#include "const_data.h"
#include "macros.h"

namespace Vc_VERSIONED_NAMESPACE
{
namespace AVX
{
    template<typename T> struct IndexesFromZeroData;
    template<> struct IndexesFromZeroData<int> {
        static Vc_ALWAYS_INLINE Vc_CONST const int *address() { return reinterpret_cast<const int *>(&_IndexesFromZero32[0]); }
    };
    template<> struct IndexesFromZeroData<unsigned int> {
        static Vc_ALWAYS_INLINE Vc_CONST const unsigned int *address() { return &_IndexesFromZero32[0]; }
    };
    template<> struct IndexesFromZeroData<short> {
        static Vc_ALWAYS_INLINE Vc_CONST const short *address() { return reinterpret_cast<const short *>(&_IndexesFromZero16[0]); }
    };
    template<> struct IndexesFromZeroData<unsigned short> {
        static Vc_ALWAYS_INLINE Vc_CONST const unsigned short *address() { return &_IndexesFromZero16[0]; }
    };
    template<> struct IndexesFromZeroData<signed char> {
        static Vc_ALWAYS_INLINE Vc_CONST const signed char *address() { return reinterpret_cast<const signed char *>(&_IndexesFromZero8[0]); }
    };
    template<> struct IndexesFromZeroData<char> {
        static Vc_ALWAYS_INLINE Vc_CONST const char *address() { return reinterpret_cast<const char *>(&_IndexesFromZero8[0]); }
    };
    template<> struct IndexesFromZeroData<unsigned char> {
        static Vc_ALWAYS_INLINE Vc_CONST const unsigned char *address() { return &_IndexesFromZero8[0]; }
    };

    template<typename _T> struct Const
    {
        typedef Vector<_T> V;
        typedef typename V::EntryType T;
        typedef typename V::Mask M;

        static Vc_ALWAYS_INLINE Vc_CONST V _pi_4()        { return V(c_trig<T>::data[0]); }
        static Vc_ALWAYS_INLINE Vc_CONST V _pi_4_hi()     { return V(c_trig<T>::data[1]); }
        static Vc_ALWAYS_INLINE Vc_CONST V _pi_4_rem1()   { return V(c_trig<T>::data[2]); }
        static Vc_ALWAYS_INLINE Vc_CONST V _pi_4_rem2()   { return V(c_trig<T>::data[3]); }
        static Vc_ALWAYS_INLINE Vc_CONST V _1_16()        { return V(c_trig<T>::data[4]); }
        static Vc_ALWAYS_INLINE Vc_CONST V _16()          { return V(c_trig<T>::data[5]); }

        static Vc_ALWAYS_INLINE Vc_CONST V atanP(int i)      { return V(c_trig<T>::data[(12 + i)]); }
        static Vc_ALWAYS_INLINE Vc_CONST V atanQ(int i)      { return V(c_trig<T>::data[(17 + i)]); }
        static Vc_ALWAYS_INLINE Vc_CONST V atanThrsHi()      { return V(c_trig<T>::data[22]); }
        static Vc_ALWAYS_INLINE Vc_CONST V atanThrsLo()      { return V(c_trig<T>::data[23]); }
        static Vc_ALWAYS_INLINE Vc_CONST V _pi_2_rem()       { return V(c_trig<T>::data[24]); }
        static Vc_ALWAYS_INLINE Vc_CONST V lossThreshold()   { return V(c_trig<T>::data[8]); }
        static Vc_ALWAYS_INLINE Vc_CONST V _4_pi()           { return V(c_trig<T>::data[9]); }
        static Vc_ALWAYS_INLINE Vc_CONST V _pi_2()           { return V(c_trig<T>::data[10]); }
        static Vc_ALWAYS_INLINE Vc_CONST V _pi()             { return V(c_trig<T>::data[11]); }
        static Vc_ALWAYS_INLINE Vc_CONST V asinCoeff0(int i) { return V(c_trig<T>::data[(28 + i)]); }
        static Vc_ALWAYS_INLINE Vc_CONST V asinCoeff1(int i) { return V(c_trig<T>::data[(33 + i)]); }
        static Vc_ALWAYS_INLINE Vc_CONST V asinCoeff2(int i) { return V(c_trig<T>::data[(37 + i)]); }
        static Vc_ALWAYS_INLINE Vc_CONST V asinCoeff3(int i) { return V(c_trig<T>::data[(43 + i)]); }
        static Vc_ALWAYS_INLINE Vc_CONST V smallAsinInput()  { return V(c_trig<T>::data[25]); }
        static Vc_ALWAYS_INLINE Vc_CONST V largeAsinInput()  { return V(c_trig<T>::data[26]); }

        static Vc_ALWAYS_INLINE Vc_CONST M exponentMask() { return M(V(c_log<T>::d(1)).data()); }
        static Vc_ALWAYS_INLINE Vc_CONST V _1_2()         { return V(c_log<T>::d(18)); }
        static Vc_ALWAYS_INLINE Vc_CONST V _1_sqrt2()     { return V(c_log<T>::d(15)); }
        static Vc_ALWAYS_INLINE Vc_CONST V P(int i)       { return V(c_log<T>::d(2 + i)); }
        static Vc_ALWAYS_INLINE Vc_CONST V Q(int i)       { return V(c_log<T>::d(8 + i)); }
        static Vc_ALWAYS_INLINE Vc_CONST V min()          { return V(c_log<T>::d(14)); }
        static Vc_ALWAYS_INLINE Vc_CONST V ln2_small()    { return V(c_log<T>::d(17)); }
        static Vc_ALWAYS_INLINE Vc_CONST V ln2_large()    { return V(c_log<T>::d(16)); }
        static Vc_ALWAYS_INLINE Vc_CONST V neginf()       { return V(c_log<T>::d(13)); }
        static Vc_ALWAYS_INLINE Vc_CONST V log10_e()      { return V(c_log<T>::d(19)); }
        static Vc_ALWAYS_INLINE Vc_CONST V log2_e()       { return V(c_log<T>::d(20)); }

        static Vc_ALWAYS_INLINE_L Vc_CONST_L V highMask() Vc_ALWAYS_INLINE_R Vc_CONST_R;
        static Vc_ALWAYS_INLINE_L Vc_CONST_L V highMask(int bits) Vc_ALWAYS_INLINE_R Vc_CONST_R;
    };

    template <> Vc_ALWAYS_INLINE Vc_CONST Vector<float> Const<float>::highMask()
    {
        return _mm256_broadcast_ss(
            reinterpret_cast<const float *>(&c_general::highMaskFloat));
    }
    template <> Vc_ALWAYS_INLINE Vc_CONST Vector<double> Const<double>::highMask()
    {
        return _mm256_broadcast_sd(
            reinterpret_cast<const double *>(&c_general::highMaskDouble));
    }
    template <> Vc_ALWAYS_INLINE Vc_CONST Vector<float> Const<float>::highMask(int bits)
    {
#ifdef Vc_IMPL_AVX2
#if defined Vc_ICC || defined Vc_MSVC
        __m256i allone = _mm256_set1_epi64x(~0);
#else
        auto allone = ~__m256i();
#endif
        return _mm256_castsi256_ps(_mm256_slli_epi32(allone, bits));
#else
        __m128 tmp = _mm_castsi128_ps(_mm_slli_epi32(_mm_setallone_si128(), bits));
        return concat(tmp, tmp);
#endif
    }
    template <> Vc_ALWAYS_INLINE Vc_CONST Vector<double> Const<double>::highMask(int bits)
    {
#ifdef Vc_IMPL_AVX2
#if defined Vc_ICC || defined Vc_MSVC
        __m256i allone = _mm256_set1_epi64x(~0);
#else
        auto allone = ~__m256i();
#endif
        return _mm256_castsi256_pd(_mm256_slli_epi64(allone, bits));
#else
        __m128d tmp = _mm_castsi128_pd(_mm_slli_epi64(_mm_setallone_si128(), bits));
        return concat(tmp, tmp);
#endif
    }
}  // namespace AVX

namespace AVX2
{
using AVX::IndexesFromZeroData;
using AVX::Const;
}  // namespace AVX2
}  // namespace Vc

#endif // VC_AVX_CONST_H_
