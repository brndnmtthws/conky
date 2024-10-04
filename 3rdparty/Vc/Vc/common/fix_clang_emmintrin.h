/*{{{
    Copyright (C) 2013-2015 Matthias Kretz <kretz@kde.org>

    Permission to use, copy, modify, and distribute this software
    and its documentation for any purpose and without fee is hereby
    granted, provided that the above copyright notice appear in all
    copies and that both that the copyright notice and this
    permission notice and warranty disclaimer appear in supporting
    documentation, and that the name of the author not be used in
    advertising or publicity pertaining to distribution of the
    software without specific, written prior permission.

    The author disclaim all warranties with regard to this
    software, including all implied warranties of merchantability
    and fitness.  In no event shall the author be liable for any
    special, indirect or consequential damages or any damages
    whatsoever resulting from loss of use, data or profits, whether
    in an action of contract, negligence or other tortious action,
    arising out of or in connection with the use or performance of
    this software.

}}}*/

#ifndef VC_COMMON_FIX_CLANG_EMMINTRIN_H_
#define VC_COMMON_FIX_CLANG_EMMINTRIN_H_

#include "../global.h"

#if (defined Vc_CLANG && Vc_CLANG < 0x30700) || (defined Vc_APPLECLANG && Vc_APPLECLANG < 0x70000)

#ifdef _mm_slli_si128
#undef _mm_slli_si128
#define _mm_slli_si128(a, count) __extension__ ({ \
  (__m128i)__builtin_ia32_pslldqi128((__m128i)(a), (count)*8); })
#endif

#ifdef _mm_srli_si128
#undef _mm_srli_si128
#define _mm_srli_si128(a, count) __extension__ ({ \
  (__m128i)__builtin_ia32_psrldqi128((__m128i)(a), (count)*8); })
#endif

#ifdef _mm_shuffle_epi32
#undef _mm_shuffle_epi32
#define _mm_shuffle_epi32(a, imm) __extension__ ({ \
  (__m128i)__builtin_shufflevector((__v4si)(__m128i)(a), (__v4si) _mm_set1_epi32(0), \
                                   (imm) & 0x3, ((imm) & 0xc) >> 2, \
                                   ((imm) & 0x30) >> 4, ((imm) & 0xc0) >> 6); })
#endif

#ifdef _mm_shufflelo_epi16
#undef _mm_shufflelo_epi16
#define _mm_shufflelo_epi16(a, imm) __extension__ ({ \
  (__m128i)__builtin_shufflevector((__v8hi)(__m128i)(a), (__v8hi) _mm_set1_epi16(0), \
                                   (imm) & 0x3, ((imm) & 0xc) >> 2, \
                                   ((imm) & 0x30) >> 4, ((imm) & 0xc0) >> 6, \
                                   4, 5, 6, 7); })
#endif

#ifdef _mm_shufflehi_epi16
#undef _mm_shufflehi_epi16
#define _mm_shufflehi_epi16(a, imm) __extension__ ({ \
  (__m128i)__builtin_shufflevector((__v8hi)(__m128i)(a), (__v8hi) _mm_set1_epi16(0), \
                                   0, 1, 2, 3, \
                                   4 + (((imm) & 0x03) >> 0), \
                                   4 + (((imm) & 0x0c) >> 2), \
                                   4 + (((imm) & 0x30) >> 4), \
                                   4 + (((imm) & 0xc0) >> 6)); })
#endif

#ifdef _mm_shuffle_pd
#undef _mm_shuffle_pd
#define _mm_shuffle_pd(a, b, i) __extension__ ({ \
  __builtin_shufflevector((__m128d)(a), (__m128d)(b), (i) & 1, (((i) & 2) >> 1) + 2); })
#endif

#endif // Vc_CLANG || Vc_APPLECLANG

#endif // VC_COMMON_FIX_CLANG_EMMINTRIN_H_
