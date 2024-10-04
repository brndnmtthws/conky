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

#ifndef VC_COMMON_BITSCANINTRINSICS_H_
#define VC_COMMON_BITSCANINTRINSICS_H_

#if defined(Vc_GCC) || defined(Vc_CLANG) || defined(Vc_APPLECLANG)
#include <x86intrin.h>
#  ifndef _bit_scan_forward
#    define _bit_scan_forward(x) __builtin_ctz(x)
#include "macros.h"
static Vc_ALWAYS_INLINE Vc_CONST int _Vc_bit_scan_reverse_asm(unsigned int x) {
    int r;
    __asm__("bsr %1,%0" : "=r"(r) : "X"(x));
    return r;
}
#    define _bit_scan_reverse(x) _Vc_bit_scan_reverse_asm(x)
#  endif
#elif defined(_WIN32)
#include <intrin.h>
static inline __forceinline unsigned long _bit_scan_forward(unsigned long x) {
	unsigned long index;
	_BitScanForward(&index, x);
	return index;
}
static inline __forceinline unsigned long _bit_scan_reverse(unsigned long x) {
	unsigned long index;
	_BitScanReverse(&index, x);
	return index;
}
#elif defined(Vc_ICC)
// for all I know ICC supports the _bit_scan_* intrinsics
#else
// just assume the compiler can do it
#endif


#endif // VC_COMMON_BITSCANINTRINSICS_H_
