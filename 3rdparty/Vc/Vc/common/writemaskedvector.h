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

#ifndef VC_COMMON_WRITEMASKEDVECTOR_H_
#define VC_COMMON_WRITEMASKEDVECTOR_H_

#include <utility>
#include "macros.h"

namespace Vc_VERSIONED_NAMESPACE
{
namespace Common
{

template <typename V, typename M = typename V::Mask> class WriteMaskedVector
{
    static_assert(
        V::Size == M::Size,
        "incorrect use of Vc::Common::WriteMaskedVector<V, M>. V and M must have the same «Size».");

public:
    typedef M Mask;
    static constexpr size_t Size = V::Size;

    Vc_FREE_STORE_OPERATORS_ALIGNED(alignof(Mask));

    // implicit (allows {vec, mask} in places where WriteMaskedVector is expected)
    Vc_INTRINSIC WriteMaskedVector(V &v, const Mask &k) : mask(k), vec(v)
    {
    }

    // prefix
    Vc_INTRINSIC V &operator++()
    {
        V one = V::One();
        one.setZeroInverted(mask);
        return vec += one;
    }
    Vc_INTRINSIC V &operator--()
    {
        V one = V::One();
        one.setZeroInverted(mask);
        return vec -= one;
    }

    // postfix
    Vc_INTRINSIC V operator++(int)
    {
        V ret(vec);
        operator++();
        return ret;
    }
    Vc_INTRINSIC V operator--(int)
    {
        V ret(vec);
        operator--();
        return ret;
    }

#define Vc_OPERATOR_(op)                                                                 \
    template <typename U> Vc_ALWAYS_INLINE void operator op##=(U &&x)                    \
    {                                                                                    \
        operator=(static_cast<V>(vec op std::forward<U>(x)));                            \
    }
    Vc_ALL_BINARY(Vc_OPERATOR_);
    Vc_ALL_ARITHMETICS(Vc_OPERATOR_);
    Vc_ALL_SHIFTS(Vc_OPERATOR_);
#undef Vc_OPERATOR_

    Vc_ALWAYS_INLINE void operator=(const V &x)
    {
        vec.assign(x, mask);
    }

    template <typename T, typename I, typename S>
    Vc_ALWAYS_INLINE void operator=(SubscriptOperation<T, I, S, true> &&x)
    {
        vec.gather(std::move(x).gatherArguments(), mask);
    }

    template <typename F> Vc_INTRINSIC void call(const F &f) const
    {
        return vec.call(f, mask);
    }
    template <typename F> Vc_INTRINSIC V apply(const F &f) const
    {
        return vec.apply(f, mask);
    }
    template <typename F> Vc_INTRINSIC void call(F &&f) const
    {
        return vec.call(std::forward<F>(f), mask);
    }
    template <typename F> Vc_INTRINSIC V apply(F &&f) const
    {
        return vec.apply(std::forward<F>(f), mask);
    }

private:
#ifdef Vc_ICC
    // If ICC gets a by-value copy of Mask here, it'll generate a lot of superfluous
    // stack-register copies.
    const Mask &mask;
#else
    // If Clang gets a const-ref Mask here, it'll miscompile some of the masked assignment
    // statements.
    const Mask mask;
#endif
    V &vec;
};
}  // namespace Common
}  // namespace Vc

#endif // VC_COMMON_WRITEMASKEDVECTOR_H_
