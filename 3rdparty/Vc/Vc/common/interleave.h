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

#ifndef VC_COMMON_INTERLEAVE_H_
#define VC_COMMON_INTERLEAVE_H_

#include "macros.h"

namespace Vc_VERSIONED_NAMESPACE
{
/** \ingroup Utilities
 Interleaves the entries from \p a and \p b into two vectors of the same type. The order
 in the returned vector contains the elements `a[0], b[0], a[1], b[1], a[2], b[2], a[3],
 b[3], ...`.

Example:
\code
Vc::SimdArray<int, 4> a = { 1, 2, 3, 4 };
Vc::SimdArray<int, 4> b = { 9, 8, 7, 6 };
std::tie(a, b) = Vc::interleave(a, b);
std::cout << a << b;
// prints:
// <1 9 2 8><3 7 4 6>
\endcode

 \param a input vector whose data will appear at even indexes in the output
 \param b input vector whose data will appear at odd indexes in the output
 \return two vectors with data from \p a and \p b interleaved
 */
template <typename V, typename = enable_if<Traits::is_simd_vector<V>::value>>
std::pair<V, V> interleave(const V &a, const V &b)
{
    return {a.interleaveLow(b), a.interleaveHigh(b)};
}
}  // namespace Vc

#endif  // VC_COMMON_INTERLEAVE_H_

// vim: foldmethod=marker
