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

#ifndef VC_COMMON_DEINTERLEAVE_H_
#define VC_COMMON_DEINTERLEAVE_H_

#include "macros.h"

namespace Vc_VERSIONED_NAMESPACE
{

/**
 * \ingroup Vectors
 *
 * \deprecated Turn to InterleavedMemoryWrapper for a more flexible and complete solution.
 *
 * Loads two vectors of values from an interleaved array.
 *
 * \param a, b The vectors to load the values from memory into.
 * \param memory The memory location where to read the next 2 * V::Size values from
 * \param align Either pass Vc::Aligned or Vc::Unaligned. It defaults to Vc::Aligned if nothing is
 * specified.
 *
 * If you store your data as
 * \code
 * struct { float x, y; } m[1000];
 * \endcode
 * then the deinterleave function allows you to read \p Size concurrent x and y values like this:
 * \code
 * Vc::float_v x, y;
 * Vc::deinterleave(&x, &y, &m[10], Vc::Unaligned);
 * \endcode
 * This code will load m[10], m[12], m[14], ... into \p x and m[11], m[13], m[15], ... into \p y.
 *
 * The deinterleave function supports the following type combinations:
\verbatim
  V \  M | float | double | ushort | short | uint | int
=========|=======|========|========|=======|======|=====
 float_v |   X   |        |    X   |   X   |      |
---------|-------|--------|--------|-------|------|-----
double_v |       |    X   |        |       |      |
---------|-------|--------|--------|-------|------|-----
   int_v |       |        |        |   X   |      |  X
---------|-------|--------|--------|-------|------|-----
  uint_v |       |        |    X   |       |   X  |
---------|-------|--------|--------|-------|------|-----
 short_v |       |        |        |   X   |      |
---------|-------|--------|--------|-------|------|-----
ushort_v |       |        |    X   |       |      |
\endverbatim
 */
template<typename V, typename M, typename A> Vc_ALWAYS_INLINE void deinterleave(V *a, V *b,
        const M *memory, A align)
{
    Detail::deinterleave(*a, *b, memory, align);
}

// documented as default for align above
template<typename V, typename M> Vc_ALWAYS_INLINE void deinterleave(V *a, V *b,
        const M *memory)
{
    Detail::deinterleave(*a, *b, memory, Aligned);
}

}  // namespace Vc

#endif // VC_COMMON_DEINTERLEAVE_H_
