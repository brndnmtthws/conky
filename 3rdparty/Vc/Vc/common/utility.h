/*  This file is part of the Vc library. {{{
Copyright © 2014-2015 Matthias Kretz <kretz@kde.org>

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

#ifndef VC_COMMON_UTILITY_H_
#define VC_COMMON_UTILITY_H_

#include "macros.h"

namespace Vc_VERSIONED_NAMESPACE
{
namespace Common
{
/**
 * \internal
 * Returns the next power of 2 larger than or equal to \p x.
 */
template <size_t x, bool = (x & (x - 1)) == 0> struct NextPowerOfTwo;
template <size_t x>
struct NextPowerOfTwo<x, true> : public std::integral_constant<size_t, x> {
};
template <size_t x>
struct NextPowerOfTwo<x, false>
    : public std::integral_constant<
          size_t, NextPowerOfTwo<(x | (x >> 1) | (x >> 2) | (x >> 5)) + 1>::value> {
};

/**
 * \internal
 * Enforce an upper bound to an alignment value. This is necessary because some compilers
 * implement such an upper bound and emit a warning if it is encountered.
 */
template <size_t A>
struct BoundedAlignment : public std::integral_constant<size_t,
#if defined Vc_MSVC || defined Vc_GCC
                                                        ((A - 1) &
#ifdef Vc_MSVC
                                                         31
#elif defined __AVX__
                                                         255
#else
                                                         127
#endif
                                                         ) + 1
#else
                                                        A
#endif
                                                        > {
};

/**
 * \internal
 * Returns the size of the left/first SimdArray member.
 */
template <std::size_t N> static constexpr std::size_t left_size()
{
    return Common::NextPowerOfTwo<(N + 1) / 2>::value;
}
/**
 * \internal
 * Returns the size of the right/second SimdArray member.
 */
template <std::size_t N> static constexpr std::size_t right_size()
{
    return N - left_size<N>();
}

}  // namespace Common
}  // namespace Vc

#endif  // VC_COMMON_UTILITY_H_

// vim: foldmethod=marker
