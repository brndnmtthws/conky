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

#ifndef VC_COMMON_SET_H_
#define VC_COMMON_SET_H_

#include "macros.h"
namespace Vc_VERSIONED_NAMESPACE
{
namespace
{
    static Vc_INTRINSIC Vc_CONST __m128i set(unsigned short x0, unsigned short x1, unsigned short x2, unsigned short x3,
            unsigned short x4, unsigned short x5, unsigned short x6, unsigned short x7)
    {
#if defined(Vc_GNU_ASM)
#if 0 // defined(__x86_64__)
        // it appears that the 32bit variant is always faster
        __m128i r;
        unsigned long long tmp0 = x3; tmp0 = (tmp0 << 16) | x2;
        unsigned long long tmp1 = x1; tmp1 = (tmp1 << 16) | x0;
        asm("vmovq %1,%0" : "=x"(r) : "r"((tmp0 << 32) | tmp1));
        unsigned long long tmp2 = x7; tmp2 = (tmp2 << 16) | x6;
        unsigned long long tmp3 = x5; tmp3 = (tmp3 << 16) | x4;
        asm("vpinsrq $1,%1,%0,%0" : "+x"(r) : "r"((tmp2 << 32) | tmp3));
        return r;
#elif defined(Vc_USE_VEX_CODING)
        __m128i r0, r1;
        unsigned int tmp0 = x1; tmp0 = (tmp0 << 16) | x0;
        unsigned int tmp1 = x3; tmp1 = (tmp1 << 16) | x2;
        unsigned int tmp2 = x5; tmp2 = (tmp2 << 16) | x4;
        unsigned int tmp3 = x7; tmp3 = (tmp3 << 16) | x6;
        asm("vmovd %1,%0" : "=x"(r0) : "r"(tmp0));
        asm("vpinsrd $1,%1,%0,%0" : "+x"(r0) : "r"(tmp1));
        asm("vmovd %1,%0" : "=x"(r1) : "r"(tmp2));
        asm("vpinsrd $1,%1,%0,%0" : "+x"(r1) : "r"(tmp3));
        asm("vpunpcklqdq %1,%0,%0" : "+x"(r0) : "x"(r1));
        return r0;
#else
        __m128i r0, r1;
        unsigned int tmp0 = x1; tmp0 = (tmp0 << 16) | x0;
        unsigned int tmp1 = x3; tmp1 = (tmp1 << 16) | x2;
        unsigned int tmp2 = x5; tmp2 = (tmp2 << 16) | x4;
        unsigned int tmp3 = x7; tmp3 = (tmp3 << 16) | x6;
        asm("movd %1,%0" : "=x"(r0) : "r"(tmp0));
        asm("pinsrd $1,%1,%0" : "+x"(r0) : "r"(tmp1));
        asm("movd %1,%0" : "=x"(r1) : "r"(tmp2));
        asm("pinsrd $1,%1,%0" : "+x"(r1) : "r"(tmp3));
        asm("punpcklqdq %1,%0" : "+x"(r0) : "x"(r1));
        return r0;
#endif
#else
        unsigned int tmp0 = x1; tmp0 = (tmp0 << 16) | x0;
        unsigned int tmp1 = x3; tmp1 = (tmp1 << 16) | x2;
        unsigned int tmp2 = x5; tmp2 = (tmp2 << 16) | x4;
        unsigned int tmp3 = x7; tmp3 = (tmp3 << 16) | x6;
        return _mm_setr_epi32(tmp0, tmp1, tmp2, tmp3);
#endif
    }
    static Vc_INTRINSIC Vc_CONST __m128i set(short x0, short x1, short x2, short x3, short x4, short x5, short x6, short x7)
    {
        return set(static_cast<unsigned short>(x0), static_cast<unsigned short>(x1), static_cast<unsigned short>(x2),
                static_cast<unsigned short>(x3), static_cast<unsigned short>(x4), static_cast<unsigned short>(x5),
                static_cast<unsigned short>(x6), static_cast<unsigned short>(x7));
    }
}  // anonymous namespace
}  // namespace Vc

#endif // VC_COMMON_SET_H_
