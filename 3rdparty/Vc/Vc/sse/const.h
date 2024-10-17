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

#ifndef VC_SSE_CONST_H_
#define VC_SSE_CONST_H_

#include "const_data.h"
#include "vector.h"
#include "macros.h"

namespace Vc_VERSIONED_NAMESPACE
{
namespace SSE
{
    template<typename T> struct Const
    {
        typedef Vector<T> V;
        typedef Mask<T> M;
        enum Constants { Stride = 16 / sizeof(T) };

        static Vc_ALWAYS_INLINE Vc_CONST V _pi_4()        { return load(&c_trig<T>::data[0 * Stride]); }
        static Vc_ALWAYS_INLINE Vc_CONST V _pi_4_hi()     { return load(&c_trig<T>::data[1 * Stride]); }
        static Vc_ALWAYS_INLINE Vc_CONST V _pi_4_rem1()   { return load(&c_trig<T>::data[2 * Stride]); }
        static Vc_ALWAYS_INLINE Vc_CONST V _pi_4_rem2()   { return load(&c_trig<T>::data[3 * Stride]); }
        static Vc_ALWAYS_INLINE Vc_CONST V _1_16()        { return load(&c_trig<T>::data[4 * Stride]); }
        static Vc_ALWAYS_INLINE Vc_CONST V _16()          { return load(&c_trig<T>::data[5 * Stride]); }

        static Vc_ALWAYS_INLINE Vc_CONST V atanP(int i)    { return load(&c_trig<T>::data[(12 + i) * Stride]); }
        static Vc_ALWAYS_INLINE Vc_CONST V atanQ(int i)    { return load(&c_trig<T>::data[(17 + i) * Stride]); }
        static Vc_ALWAYS_INLINE Vc_CONST V atanThrsHi()    { return load(&c_trig<T>::data[22 * Stride]); }
        static Vc_ALWAYS_INLINE Vc_CONST V atanThrsLo()    { return load(&c_trig<T>::data[23 * Stride]); }
        static Vc_ALWAYS_INLINE Vc_CONST V _pi_2_rem()     { return load(&c_trig<T>::data[24 * Stride]); }
        static Vc_ALWAYS_INLINE Vc_CONST V lossThreshold() { return load(&c_trig<T>::data[8 * Stride]); }
        static Vc_ALWAYS_INLINE Vc_CONST V _4_pi()         { return load(&c_trig<T>::data[9 * Stride]); }
        static Vc_ALWAYS_INLINE Vc_CONST V _pi_2()         { return load(&c_trig<T>::data[10 * Stride]); }
        static Vc_ALWAYS_INLINE Vc_CONST V _pi()           { return load(&c_trig<T>::data[11 * Stride]); }
        static Vc_ALWAYS_INLINE Vc_CONST V asinCoeff0(int i) { return load(&c_trig<T>::data[(28 + i) * Stride]); }
        static Vc_ALWAYS_INLINE Vc_CONST V asinCoeff1(int i) { return load(&c_trig<T>::data[(33 + i) * Stride]); }
        static Vc_ALWAYS_INLINE Vc_CONST V asinCoeff2(int i) { return load(&c_trig<T>::data[(37 + i) * Stride]); }
        static Vc_ALWAYS_INLINE Vc_CONST V asinCoeff3(int i) { return load(&c_trig<T>::data[(43 + i) * Stride]); }
        static Vc_ALWAYS_INLINE Vc_CONST V smallAsinInput()  { return load(&c_trig<T>::data[25 * Stride]); }
        static Vc_ALWAYS_INLINE Vc_CONST V largeAsinInput()  { return load(&c_trig<T>::data[26 * Stride]); }

        static Vc_ALWAYS_INLINE Vc_CONST M exponentMask() { return M(load(c_log<T>::d(1)).data()); }
        static Vc_ALWAYS_INLINE Vc_CONST V _1_2()         { return load(c_log<T>::d(18)); }
        static Vc_ALWAYS_INLINE Vc_CONST V _1_sqrt2()     { return load(c_log<T>::d(15)); }
        static Vc_ALWAYS_INLINE Vc_CONST V P(int i)       { return load(c_log<T>::d(2 + i)); }
        static Vc_ALWAYS_INLINE Vc_CONST V Q(int i)       { return load(c_log<T>::d(8 + i)); }
        static Vc_ALWAYS_INLINE Vc_CONST V min()          { return load(c_log<T>::d(14)); }
        static Vc_ALWAYS_INLINE Vc_CONST V ln2_small()    { return load(c_log<T>::d(17)); }
        static Vc_ALWAYS_INLINE Vc_CONST V ln2_large()    { return load(c_log<T>::d(16)); }
        static Vc_ALWAYS_INLINE Vc_CONST V neginf()       { return load(c_log<T>::d(13)); }
        static Vc_ALWAYS_INLINE Vc_CONST V log10_e()      { return load(c_log<T>::d(19)); }
        static Vc_ALWAYS_INLINE Vc_CONST V log2_e()       { return load(c_log<T>::d(20)); }

        static Vc_ALWAYS_INLINE_L Vc_CONST_L V highMask()         Vc_ALWAYS_INLINE_R Vc_CONST_R;
        static Vc_ALWAYS_INLINE_L Vc_CONST_L V highMask(int bits) Vc_ALWAYS_INLINE_R Vc_CONST_R;
    private:
        static Vc_ALWAYS_INLINE_L Vc_CONST_L V load(const T *mem) Vc_ALWAYS_INLINE_R Vc_CONST_R;
    };
    template<typename T> Vc_ALWAYS_INLINE Vc_CONST Vector<T> Const<T>::load(const T *mem) { return V(mem); }

    template <> Vc_ALWAYS_INLINE Vc_CONST Vector<float> Const<float>::highMask()
    {
        return Vector<float>(reinterpret_cast<const float *>(&c_general::highMaskFloat));
    }
    template <> Vc_ALWAYS_INLINE Vc_CONST Vector<double> Const<double>::highMask()
    {
        return Vector<double>(
            reinterpret_cast<const double *>(&c_general::highMaskDouble));
    }
    template <> Vc_ALWAYS_INLINE Vc_CONST Vector<float> Const<float>::highMask(int bits)
    {
        return _mm_castsi128_ps(_mm_slli_epi32(_mm_setallone_si128(), bits));
    }
    template <> Vc_ALWAYS_INLINE Vc_CONST Vector<double> Const<double>::highMask(int bits)
    {
        return _mm_castsi128_pd(_mm_slli_epi64(_mm_setallone_si128(), bits));
    }
}  // namespace SSE
}  // namespace Vc

#endif // VC_SSE_CONST_H_
