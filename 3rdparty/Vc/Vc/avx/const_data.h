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

#ifndef VC_AVX_CONST_DATA_H_
#define VC_AVX_CONST_DATA_H_

#include "../common/data.h"
#include "macros.h"

namespace Vc_VERSIONED_NAMESPACE
{
namespace AVX
{

alignas(64) extern const unsigned int   _IndexesFromZero32[ 8];
alignas(16) extern const unsigned short _IndexesFromZero16[16];
alignas(16) extern const unsigned char  _IndexesFromZero8 [32];

struct alignas(64) c_general
{
    static const float oneFloat;
    static const unsigned int absMaskFloat[2];
    static const unsigned int signMaskFloat[2];
    static const unsigned int highMaskFloat;
    static const unsigned short minShort[2];
    static const unsigned short one16[2];
    static const float _2power31;
    static const double oneDouble;
    static const unsigned long long frexpMask;
    static const unsigned long long highMaskDouble;
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
    typedef float floatAlias Vc_MAY_ALIAS;
    static Vc_ALWAYS_INLINE float d(int i) { return *reinterpret_cast<const floatAlias *>(&data[i]); }
    alignas(64) static const unsigned int data[21];
};
#ifndef Vc_MSVC
template<> alignas(64) const unsigned int c_log<float>::data[21];
#endif

template<> struct c_log<double>
{
    enum VectorSize { Size = 16 / sizeof(double) };
    typedef double doubleAlias Vc_MAY_ALIAS;
    static Vc_ALWAYS_INLINE double d(int i) { return *reinterpret_cast<const doubleAlias *>(&data[i]); }
    alignas(64) static const unsigned long long data[21];
};

}  // namespace AVX
}  // namespace Vc

namespace Vc_VERSIONED_NAMESPACE
{
namespace AVX2
{
    using AVX::_IndexesFromZero8;
    using AVX::_IndexesFromZero16;
    using AVX::_IndexesFromZero32;
    using AVX::c_general;
    using AVX::c_trig;
    using AVX::c_log;
}  // namespace AVX2
}  // namespace Vc

#endif // VC_AVX_CONST_DATA_H_
