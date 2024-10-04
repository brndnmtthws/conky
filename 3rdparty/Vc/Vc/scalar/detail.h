/*  This file is part of the Vc library. {{{
Copyright © 2016 Matthias Kretz <kretz@kde.org>

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

#ifndef VC_SCALAR_DETAIL_H_
#define VC_SCALAR_DETAIL_H_

#include "../common/detail.h"
#include "macros.h"

namespace Vc_VERSIONED_NAMESPACE
{
namespace Detail
{
//InterleaveImpl{{{1
template<typename V, size_t Size, size_t VSize> struct InterleaveImpl;
template<typename V, size_t VSize> struct InterleaveImpl<V, 1, VSize> {
    template <typename I>  // interleave 2 args {{{2
    static inline void interleave(typename V::EntryType *const data, const I &i,
                                  const typename V::AsArg v0, const typename V::AsArg v1)
    {
        data[i[0] + 0] = v0.data();
        data[i[0] + 1] = v1.data();
    }
    template <typename I>  // interleave 3 args {{{2
    static inline void interleave(typename V::EntryType *const data, const I &i,
                                  const typename V::AsArg v0, const typename V::AsArg v1,
                                  const typename V::AsArg v2)
    {
        data[i[0] + 0] = v0.data();
        data[i[0] + 1] = v1.data();
        data[i[0] + 2] = v2.data();
    }
    template <typename I>  // interleave 4 args {{{2
    static inline void interleave(typename V::EntryType *const data, const I &i,
                                  const typename V::AsArg v0, const typename V::AsArg v1,
                                  const typename V::AsArg v2, const typename V::AsArg v3)
    {
        data[i[0] + 0] = v0.data();
        data[i[0] + 1] = v1.data();
        data[i[0] + 2] = v2.data();
        data[i[0] + 3] = v3.data();
    }
    template <typename I>  // interleave 5 args {{{2
    static inline void interleave(typename V::EntryType *const data, const I &i,
                                  const typename V::AsArg v0, const typename V::AsArg v1,
                                  const typename V::AsArg v2, const typename V::AsArg v3,
                                  const typename V::AsArg v4)
    {
        interleave(data, i, v0, v1, v2, v3);
        data[i[0] + 4] = v4.data();
    }
    template <typename I>  // interleave 6 args {{{2
    static inline void interleave(typename V::EntryType *const data, const I &i,
                                  const typename V::AsArg v0, const typename V::AsArg v1,
                                  const typename V::AsArg v2, const typename V::AsArg v3,
                                  const typename V::AsArg v4, const typename V::AsArg v5)
    {
        interleave(data, i, v0, v1, v2, v3);
        interleave(data + 4, i, v4, v5);
    }
    template <typename I>  // interleave 7 args {{{2
    static inline void interleave(typename V::EntryType *const data, const I &i,
                                  const typename V::AsArg v0, const typename V::AsArg v1,
                                  const typename V::AsArg v2, const typename V::AsArg v3,
                                  const typename V::AsArg v4, const typename V::AsArg v5,
                                  const typename V::AsArg v6)
    {
        interleave(data, i, v0, v1, v2, v3);
        interleave(data + 4, i, v4, v5, v6);
    }
    template <typename I>  // interleave 8 args {{{2
    static inline void interleave(typename V::EntryType *const data, const I &i,
                                  const typename V::AsArg v0, const typename V::AsArg v1,
                                  const typename V::AsArg v2, const typename V::AsArg v3,
                                  const typename V::AsArg v4, const typename V::AsArg v5,
                                  const typename V::AsArg v6, const typename V::AsArg v7)
    {
        interleave(data, i, v0, v1, v2, v3);
        interleave(data + 4, i, v4, v5, v6, v7);
    }
    //}}}2
    template <typename I>  // deinterleave 2 args{{{2
    static inline void deinterleave(typename V::EntryType const *const data, const I &i,
                                    V &v0, V &v1)
    {
        v0.data() = data[i[0] + 0];
        v1.data() = data[i[0] + 1];
    }
    template <typename I>  // deinterleave 3 args{{{2
    static inline void deinterleave(typename V::EntryType const *const data, const I &i,
                                    V &v0, V &v1, V &v2)
    {
        v0.data() = data[i[0] + 0];
        v1.data() = data[i[0] + 1];
        v2.data() = data[i[0] + 2];
    }
    template <typename I>  // deinterleave 4 args{{{2
    static inline void deinterleave(typename V::EntryType const *const data, const I &i,
                                    V &v0, V &v1, V &v2, V &v3)
    {
        v0.data() = data[i[0] + 0];
        v1.data() = data[i[0] + 1];
        v2.data() = data[i[0] + 2];
        v3.data() = data[i[0] + 3];
    }
    template <typename I>  // deinterleave 5 args{{{2
    static inline void deinterleave(typename V::EntryType const *const data, const I &i,
                                    V &v0, V &v1, V &v2, V &v3, V &v4)
    {
        deinterleave(data, i, v0, v1, v2, v3);
        v4.data() = data[i[0] + 4];
    }
    template <typename I>  // deinterleave 6 args{{{2
    static inline void deinterleave(typename V::EntryType const *const data, const I &i,
                                    V &v0, V &v1, V &v2, V &v3, V &v4, V &v5)
    {
        deinterleave(data, i, v0, v1, v2, v3);
        deinterleave(data + 4, i, v4, v5);
    }
    template <typename I>  // deinterleave 7 args{{{2
    static inline void deinterleave(typename V::EntryType const *const data, const I &i,
                                    V &v0, V &v1, V &v2, V &v3, V &v4, V &v5, V &v6)
    {
        deinterleave(data, i, v0, v1, v2, v3);
        deinterleave(data + 4, i, v4, v5, v6);
    }
    template <typename I>  // deinterleave 8 args{{{2
    static inline void deinterleave(typename V::EntryType const *const data, const I &i,
                                    V &v0, V &v1, V &v2, V &v3, V &v4, V &v5, V &v6,
                                    V &v7)
    {
        deinterleave(data, i, v0, v1, v2, v3);
        deinterleave(data + 4, i, v4, v5, v6, v7);
    }
};
//}}}1
}  // namespace Detail
}  // namespace Vc

#endif  // VC_SCALAR_DETAIL_H_

// vim: foldmethod=marker
