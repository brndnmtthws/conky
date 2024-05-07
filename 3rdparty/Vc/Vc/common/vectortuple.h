/*  This file is part of the Vc library. {{{
Copyright © 2012-2015 Matthias Kretz <kretz@kde.org>

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

#ifndef VC_COMMON_VECTORTUPLE_H_
#define VC_COMMON_VECTORTUPLE_H_

#include "transpose.h"
#include "macros.h"

namespace Vc_VERSIONED_NAMESPACE
{
namespace Common
{

template<size_t StructSize, typename V, typename I, bool Readonly = true> struct InterleavedMemoryReadAccess;

template <int Length, typename V> class VectorReferenceArray
{
    typedef typename V::EntryType T;
    typedef V &Vc_RESTRICT Reference;
    std::array<V * Vc_RESTRICT, Length> r;

    typedef make_index_sequence<Length> IndexSequence;

    template <typename VV, std::size_t... Indexes>
    constexpr VectorReferenceArray<Length + 1, VV> appendOneReference(
        VV &a, index_sequence<Indexes...>) const
    {
        return {*r[Indexes]..., a};
    }

    template <typename A, std::size_t... Indexes>
    Vc_INTRINSIC void callDeinterleave(const A &access, index_sequence<Indexes...>) const
    {
        access.deinterleave(*r[Indexes]...);
    }

public:
    template <typename... Us, typename = enable_if<(sizeof...(Us) == Length)>>
    constexpr VectorReferenceArray(Us &&... args)
        : r{{std::addressof(std::forward<Us>(args))...}}
    {
    }

    template <typename VV, typename = enable_if<!std::is_const<V>::value &&
                                                std::is_same<VV, V>::value>>
    Vc_DEPRECATED("build the tuple with Vc::tie instead") constexpr VectorReferenceArray<
        Length + 1, V>
    operator,(VV &a) const &&
    {
        return appendOneReference(a, IndexSequence());
    }

    Vc_DEPRECATED("build the tuple with Vc::tie instead") constexpr VectorReferenceArray<
        Length + 1, const V>
    operator,(const V &a) const &&
    {
        return appendOneReference(a, IndexSequence());
    }

    template <size_t StructSize, typename I, bool RO>
    Vc_ALWAYS_INLINE enable_if<(Length <= StructSize), void> operator=(
        const InterleavedMemoryReadAccess<StructSize, V, I, RO> &access) &&
    {
        callDeinterleave(access, IndexSequence());
    }

    template <size_t StructSize, typename I, bool RO>
    enable_if<(Length > StructSize), void> operator=(
        const InterleavedMemoryReadAccess<StructSize, V, I, RO> &access) && =
        delete;  //("You are trying to extract more data from the struct than it has");

    template <typename... Inputs> void operator=(TransposeProxy<Inputs...> &&proxy) &&
    {
        transpose_impl(TransposeTag<Length, sizeof...(Inputs)>(), &r[0], proxy);
    }

    template <typename T, typename IndexVector, typename Scale, bool Flag>
    void operator=(SubscriptOperation<T, IndexVector, Scale, Flag> &&sub) &&
    {
        const auto &args = std::move(sub).gatherArguments();
        //const IndexVector args.indexes;
        //const T *const args.address;
        Common::InterleavedMemoryReadAccess<1, V, Traits::decay<decltype(args.indexes)>>
            deinterleaver(args.address, args.indexes);
        callDeinterleave(deinterleaver, IndexSequence());
    }

    Vc_ALWAYS_INLINE Reference operator[](std::size_t i) { return *r[i]; }
};

}  // namespace Common

template <typename T, typename Abi>
Vc_DEPRECATED("build the tuple with Vc::tie instead")
constexpr Common::VectorReferenceArray<2, Vc::Vector<T, Abi>>
operator,(Vc::Vector<T, Abi> &a, Vc::Vector<T, Abi> &b)
{
    return {a, b};
}

template <typename T, typename Abi>
Vc_DEPRECATED("build the tuple with Vc::tie instead")
constexpr Common::VectorReferenceArray<2, const Vc::Vector<T, Abi>>
operator,(const Vc::Vector<T, Abi> &a, const Vc::Vector<T, Abi> &b)
{
    return {a, b};
}

template <typename V, typename... Vs>
constexpr Common::VectorReferenceArray<sizeof...(Vs) + 1,
                                       typename std::remove_reference<V>::type>
    tie(V &&a, Vs &&... b)
{
    return {std::forward<V>(a), std::forward<Vs>(b)...};
}

}  // namespace Vc

#endif // VC_COMMON_VECTORTUPLE_H_
