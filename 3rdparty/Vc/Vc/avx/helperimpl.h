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

#ifndef VC_AVX_HELPERIMPL_H_
#define VC_AVX_HELPERIMPL_H_

#include "../sse/helperimpl.h"
#include "macros.h"

namespace Vc_VERSIONED_NAMESPACE
{
namespace Detail
{
template <typename A>
inline void deinterleave(AVX2::float_v &, AVX2::float_v &, const float *, A);
template <typename A>
inline void deinterleave(AVX2::float_v &, AVX2::float_v &, const short *, A);
template <typename A>
inline void deinterleave(AVX2::float_v &, AVX2::float_v &, const ushort *, A);
template <typename A>
inline void deinterleave(AVX2::double_v &, AVX2::double_v &, const double *, A);
template <typename A>
inline void deinterleave(AVX2::int_v &, AVX2::int_v &, const int *, A);
template <typename A>
inline void deinterleave(AVX2::int_v &, AVX2::int_v &, const short *, A);
template <typename A>
inline void deinterleave(AVX2::uint_v &, AVX2::uint_v &, const uint *, A);
template <typename A>
inline void deinterleave(AVX2::uint_v &, AVX2::uint_v &, const ushort *, A);
template <typename A>
inline void deinterleave(AVX2::short_v &, AVX2::short_v &, const short *, A);
template <typename A>
inline void deinterleave(AVX2::ushort_v &, AVX2::ushort_v &, const ushort *, A);

template <typename T, typename M, typename A>
Vc_ALWAYS_INLINE_L void deinterleave(AVX2::Vector<T> &Vc_RESTRICT a,
                                     AVX2::Vector<T> &Vc_RESTRICT b,
                                     AVX2::Vector<T> &Vc_RESTRICT c,
                                     const M *Vc_RESTRICT memory,
                                     A align) Vc_ALWAYS_INLINE_R;
template <typename T, typename M, typename A>
Vc_ALWAYS_INLINE_L void deinterleave(AVX2::Vector<T> &Vc_RESTRICT a,
                                     AVX2::Vector<T> &Vc_RESTRICT b,
                                     AVX2::Vector<T> &Vc_RESTRICT c,
                                     AVX2::Vector<T> &Vc_RESTRICT d,
                                     const M *Vc_RESTRICT memory,
                                     A align) Vc_ALWAYS_INLINE_R;
template <typename T, typename M, typename A>
Vc_ALWAYS_INLINE_L void deinterleave(AVX2::Vector<T> &Vc_RESTRICT a,
                                     AVX2::Vector<T> &Vc_RESTRICT b,
                                     AVX2::Vector<T> &Vc_RESTRICT c,
                                     AVX2::Vector<T> &Vc_RESTRICT d,
                                     AVX2::Vector<T> &Vc_RESTRICT e,
                                     const M *Vc_RESTRICT memory,
                                     A align) Vc_ALWAYS_INLINE_R;
template <typename T, typename M, typename A>
Vc_ALWAYS_INLINE_L void deinterleave(
    AVX2::Vector<T> &Vc_RESTRICT a, AVX2::Vector<T> &Vc_RESTRICT b,
    AVX2::Vector<T> &Vc_RESTRICT c, AVX2::Vector<T> &Vc_RESTRICT d,
    AVX2::Vector<T> &Vc_RESTRICT e, AVX2::Vector<T> &Vc_RESTRICT f,
    const M *Vc_RESTRICT memory, A align) Vc_ALWAYS_INLINE_R;
template <typename T, typename M, typename A>
Vc_ALWAYS_INLINE_L void deinterleave(
    AVX2::Vector<T> &Vc_RESTRICT a, AVX2::Vector<T> &Vc_RESTRICT b,
    AVX2::Vector<T> &Vc_RESTRICT c, AVX2::Vector<T> &Vc_RESTRICT d,
    AVX2::Vector<T> &Vc_RESTRICT e, AVX2::Vector<T> &Vc_RESTRICT f,
    AVX2::Vector<T> &Vc_RESTRICT g, AVX2::Vector<T> &Vc_RESTRICT h,
    const M *Vc_RESTRICT memory, A align) Vc_ALWAYS_INLINE_R;

Vc_ALWAYS_INLINE void prefetchForOneRead(const void *addr, VectorAbi::Avx)
{
    prefetchForOneRead(addr, VectorAbi::Sse());
}
Vc_ALWAYS_INLINE void prefetchForModify(const void *addr, VectorAbi::Avx)
{
    prefetchForModify(addr, VectorAbi::Sse());
}
Vc_ALWAYS_INLINE void prefetchClose(const void *addr, VectorAbi::Avx)
{
    prefetchClose(addr, VectorAbi::Sse());
}
Vc_ALWAYS_INLINE void prefetchMid(const void *addr, VectorAbi::Avx)
{
    prefetchMid(addr, VectorAbi::Sse());
}
Vc_ALWAYS_INLINE void prefetchFar(const void *addr, VectorAbi::Avx)
{
    prefetchFar(addr, VectorAbi::Sse());
}
}  // namespace Detail
}  // namespace Vc

#include "deinterleave.tcc"

#endif // VC_AVX_HELPERIMPL_H_
