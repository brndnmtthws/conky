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

#ifndef VC_SSE_DEINTERLEAVE_H_
#define VC_SSE_DEINTERLEAVE_H_

#include "macros.h"

namespace Vc_VERSIONED_NAMESPACE
{
namespace Detail
{
template <typename A>
inline void deinterleave(SSE::float_v &, SSE::float_v &, const float *, A);
template <typename A>
inline void deinterleave(SSE::float_v &, SSE::float_v &, const short *, A);
template <typename A>
inline void deinterleave(SSE::float_v &, SSE::float_v &, const ushort *, A);
template <typename A>
inline void deinterleave(SSE::double_v &, SSE::double_v &, const double *, A);
template <typename A>
inline void deinterleave(SSE::int_v &, SSE::int_v &, const int *, A);
template <typename A>
inline void deinterleave(SSE::int_v &, SSE::int_v &, const short *, A);
template <typename A>
inline void deinterleave(SSE::uint_v &, SSE::uint_v &, const uint *, A);
template <typename A>
inline void deinterleave(SSE::uint_v &, SSE::uint_v &, const ushort *, A);
template <typename A>
inline void deinterleave(SSE::short_v &, SSE::short_v &, const short *, A);
template <typename A>
inline void deinterleave(SSE::ushort_v &, SSE::ushort_v &, const ushort *, A);

Vc_ALWAYS_INLINE_L void prefetchForOneRead(const void *addr, VectorAbi::Sse) Vc_ALWAYS_INLINE_R;
Vc_ALWAYS_INLINE_L void prefetchForModify(const void *addr, VectorAbi::Sse) Vc_ALWAYS_INLINE_R;
Vc_ALWAYS_INLINE_L void prefetchClose(const void *addr, VectorAbi::Sse) Vc_ALWAYS_INLINE_R;
Vc_ALWAYS_INLINE_L void prefetchMid(const void *addr, VectorAbi::Sse) Vc_ALWAYS_INLINE_R;
Vc_ALWAYS_INLINE_L void prefetchFar(const void *addr, VectorAbi::Sse) Vc_ALWAYS_INLINE_R;
}  // namespace Detail
}  // namespace Vc

#include "deinterleave.tcc"
#include "prefetches.tcc"

#endif // VC_SSE_DEINTERLEAVE_H_
