/*  This file is part of the Vc library. {{{
Copyright © 2012-2015 Matthias Kretz <kretz@kde.org>

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

#ifndef VC_SSE_CONST_DATA_H_
#define VC_SSE_CONST_DATA_H_

#include "../common/data.h"
#include "macros.h"

namespace Vc_VERSIONED_NAMESPACE
{
namespace SSE
{

alignas(16) extern const unsigned int   _IndexesFromZero4[4];
alignas(16) extern const unsigned short _IndexesFromZero8[8];
alignas(16) extern const unsigned char  _IndexesFromZero16[16];

struct c_general
{
    alignas(64) static const int absMaskFloat[4];
    alignas(16) static const unsigned int signMaskFloat[4];
    alignas(16) static const unsigned int highMaskFloat[4];
    alignas(16) static const short minShort[8];

    alignas(16) static const unsigned short one16[8];
    alignas(16) static const unsigned int one32[4];
    alignas(16) static const float oneFloat[4];

    alignas(16) static const unsigned long long highMaskDouble[2];
    alignas(16) static const double oneDouble[2];
    alignas(16) static const long long absMaskDouble[2];
    alignas(16) static const unsigned long long signMaskDouble[2];
    alignas(16) static const unsigned long long frexpMask[2];
};

template<typename T> struct c_trig
{
    alignas(64) static const T data[];
};
#ifndef Vc_MSVC
template <> alignas(64) const float c_trig<float>::data[];
template <> alignas(64) const double c_trig<double>::data[];
#endif

template<typename T> struct c_log
{
    enum VectorSize { Size = 16 / sizeof(T) };
    static Vc_ALWAYS_INLINE Vc_CONST const float *d(int i) { return reinterpret_cast<const  float *>(&data[i * Size]); }
    alignas(64) static const unsigned int data[21 * Size];
};
#ifndef Vc_MSVC
template<> alignas(64) const unsigned int c_log<float>::data[21 * 4];
#endif

template<> struct c_log<double>
{
    enum VectorSize { Size = 16 / sizeof(double) };
    static Vc_ALWAYS_INLINE Vc_CONST const double *d(int i) { return reinterpret_cast<const double *>(&data[i * Size]); }
    alignas(64) static const unsigned long long data[21 * Size];
};

}  // namespace SSE
}  // namespace Vc

#endif // VC_SSE_CONST_DATA_H_
