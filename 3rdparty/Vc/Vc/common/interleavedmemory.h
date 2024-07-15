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

#ifndef VC_COMMON_INTERLEAVEDMEMORY_H_
#define VC_COMMON_INTERLEAVEDMEMORY_H_

#include "macros.h"

namespace Vc_VERSIONED_NAMESPACE
{
namespace Common
{
/**
 * \internal
 */
template<typename V, typename I, bool Readonly> struct InterleavedMemoryAccessBase
{
    // Partial specialization doesn't work for functions without partial specialization of the whole
    // class. Therefore we capture the contents of InterleavedMemoryAccessBase in a macro to easily
    // copy it into its specializations.
    typedef typename std::conditional<
        Readonly, typename std::add_const<typename V::EntryType>::type,
        typename V::EntryType>::type T;
    typedef typename V::AsArg VArg;
    typedef T Ta Vc_MAY_ALIAS;
    const I m_indexes;
    Ta *const m_data;

    Vc_ALWAYS_INLINE InterleavedMemoryAccessBase(typename I::AsArg indexes, Ta *data)
        : m_indexes(indexes), m_data(data)
    {
    }

    // implementations of the following are in {scalar,sse,avx}/detail.h
    template <typename... Vs> Vc_INTRINSIC void deinterleave(Vs &&... vs) const
    {
        Impl::deinterleave(m_data, m_indexes, std::forward<Vs>(vs)...);
    }

protected:
    using Impl = Vc::Detail::InterleaveImpl<V, V::Size, sizeof(V)>;

    template <typename T, std::size_t... Indexes>
    Vc_INTRINSIC void callInterleave(T &&a, index_sequence<Indexes...>)
    {
        Impl::interleave(m_data, m_indexes, a[Indexes]...);
    }
};

/**
 * \internal
 */
// delay execution of the deinterleaving gather until operator=
template <size_t StructSize, typename V, typename I = typename V::IndexType,
          bool Readonly>
struct InterleavedMemoryReadAccess : public InterleavedMemoryAccessBase<V, I, Readonly>
{
    typedef InterleavedMemoryAccessBase<V, I, Readonly> Base;
    typedef typename Base::Ta Ta;

    Vc_ALWAYS_INLINE InterleavedMemoryReadAccess(Ta *data, typename I::AsArg indexes)
        : Base(StructSize == 1u
                   ? indexes
                   : StructSize == 2u
                         ? indexes << 1
                         : StructSize == 4u
                               ? indexes << 2
                               : StructSize == 8u
                                     ? indexes << 3
                                     : StructSize == 16u ? indexes << 4
                                                         : indexes * I(int(StructSize)),
               data)
    {
    }

    template <typename T, std::size_t... Indexes>
    Vc_ALWAYS_INLINE T deinterleave_unpack(index_sequence<Indexes...>) const
    {
        T r;
        Base::Impl::deinterleave(this->m_data, this->m_indexes, std::get<Indexes>(r)...);
        return r;
    }

    template <typename T,
              typename = enable_if<(std::is_default_constructible<T>::value &&
                                    std::is_same<V, Traits::decay<decltype(std::get<0>(
                                                        std::declval<T &>()))>>::value)>>
    Vc_ALWAYS_INLINE operator T() const
    {
        return deinterleave_unpack<T>(make_index_sequence<std::tuple_size<T>::value>());
    }
};

///\internal Runtime check (NDEBUG) for asserting unique indexes.
template<typename I> struct CheckIndexesUnique
{
#ifdef NDEBUG
    static Vc_INTRINSIC void test(const I &) {}
#else
    static void test(const I &indexes)
    {
        const I test = indexes.sorted();
        Vc_ASSERT(I::Size == 1 || (test == test.rotated(1)).isEmpty())
    }
#endif
};
///\internal For SuccessiveEntries there can never be a problem.
template<size_t S> struct CheckIndexesUnique<SuccessiveEntries<S> >
{
    static Vc_INTRINSIC void test(const SuccessiveEntries<S> &) {}
};

/**
 * \internal
 */
template <size_t StructSize, typename V, typename I = typename V::IndexType>
struct InterleavedMemoryAccess : public InterleavedMemoryReadAccess<StructSize, V, I, false>
{
    typedef InterleavedMemoryAccessBase<V, I, false> Base;
    typedef typename Base::Ta Ta;

    Vc_ALWAYS_INLINE InterleavedMemoryAccess(Ta *data, typename I::AsArg indexes)
        : InterleavedMemoryReadAccess<StructSize, V, I, false>(data, indexes)
    {
        CheckIndexesUnique<I>::test(indexes);
    }

    template <int N> Vc_ALWAYS_INLINE void operator=(VectorReferenceArray<N, V> &&rhs)
    {
        static_assert(N <= StructSize,
                      "You_are_trying_to_scatter_more_data_into_the_struct_than_it_has");
        this->callInterleave(std::move(rhs), make_index_sequence<N>());
    }
    template <int N> Vc_ALWAYS_INLINE void operator=(VectorReferenceArray<N, const V> &&rhs)
    {
        static_assert(N <= StructSize,
                      "You_are_trying_to_scatter_more_data_into_the_struct_than_it_has");
        this->callInterleave(std::move(rhs), make_index_sequence<N>());
    }
};

/**
 * Wraps a pointer to memory with convenience functions to access it via vectors.
 *
 * \param S The type of the struct.
 * \param V The type of the vector to be returned when read. This should reflect the type of the
 * members inside the struct.
 *
 * \see operator[]
 * \ingroup Containers
 * \headerfile interleavedmemory.h <Vc/Memory>
 */
template<typename S, typename V> class InterleavedMemoryWrapper
{
    typedef typename std::conditional<std::is_const<S>::value,
                                      const typename V::EntryType,
                                      typename V::EntryType>::type T;
    typedef typename V::IndexType I;
    typedef typename V::AsArg VArg;
    typedef const I &IndexType;
    static constexpr std::size_t StructSize = sizeof(S) / sizeof(T);
    using ReadAccess = InterleavedMemoryReadAccess<StructSize, V>;
    using Access =
        typename std::conditional<std::is_const<T>::value, ReadAccess,
                                  InterleavedMemoryAccess<StructSize, V>>::type;
    using ReadSuccessiveEntries =
        InterleavedMemoryReadAccess<StructSize, V, SuccessiveEntries<StructSize>>;
    using AccessSuccessiveEntries = typename std::conditional<
        std::is_const<T>::value, ReadSuccessiveEntries,
        InterleavedMemoryAccess<StructSize, V, SuccessiveEntries<StructSize>>>::type;
    typedef T Ta Vc_MAY_ALIAS;
    Ta *const m_data;

    static_assert(StructSize * sizeof(T) == sizeof(S),
                  "InterleavedMemoryAccess_does_not_support_packed_structs");

public:
    /**
     * Constructs the wrapper object.
     *
     * \param s A pointer to a C-array.
     */
    Vc_ALWAYS_INLINE InterleavedMemoryWrapper(S *s)
        : m_data(reinterpret_cast<Ta *>(s))
    {
    }

    /**
     * Interleaved scatter/gather access.
     *
     * Assuming you have a struct of floats and a vector of \p indexes into the array, this function
     * can be used to access the struct entries as vectors using the minimal number of store or load
     * instructions.
     *
     * \param indexes Vector of indexes that determine the gather locations.
     *
     * \return A special (magic) object that executes the loads and deinterleave on assignment to a
     * vector tuple.
     *
     * Example:
     * \code
     * struct Foo {
     *   float x, y, z;
     * };
     *
     * void fillWithBar(Foo *_data, uint_v indexes)
     * {
     *   Vc::InterleavedMemoryWrapper<Foo, float_v> data(_data);
     *   const float_v x = bar(1);
     *   const float_v y = bar(2);
     *   const float_v z = bar(3);
     *   data[indexes] = (x, y, z);
     *   // it's also possible to just store a subset at the front of the struct:
     *   data[indexes] = (x, y);
     *   // if you want to store a single entry, use scatter:
     *   z.scatter(_data, &Foo::x, indexes);
     * }
     *
     * float_v normalizeStuff(Foo *_data, uint_v indexes)
     * {
     *   Vc::InterleavedMemoryWrapper<Foo, float_v> data(_data);
     *   float_v x, y, z;
     *   (x, y, z) = data[indexes];
     *   // it is also possible to just load a subset from the front of the struct:
     *   // (x, y) = data[indexes];
     *   return Vc::sqrt(x * x + y * y + z * z);
     * }
     * \endcode
     *
     * You may think of the gather operation (or scatter as the inverse) like this:
\verbatim
             Memory: {x0 y0 z0 x1 y1 z1 x2 y2 z2 x3 y3 z3 x4 y4 z4 x5 y5 z5 x6 y6 z6 x7 y7 z7 x8 y8 z8}
            indexes: [5, 0, 1, 7]
Result in (x, y, z): ({x5 x0 x1 x7}, {y5 y0 y1 y7}, {z5 z0 z1 z7})
\endverbatim
     *
     * \warning If \p indexes contains non-unique entries on scatter, the result is undefined. If
     * \c NDEBUG is not defined the implementation will assert that the \p indexes entries are unique.
     */
    template <typename IT>
    Vc_ALWAYS_INLINE enable_if<!std::is_convertible<IT, size_t>::value &&
                                   std::is_convertible<IT, IndexType>::value &&
                                   !std::is_const<S>::value,
                               Access>
    operator[](IT indexes)
    {
        return Access(m_data, indexes);
    }

    /// const overload (gathers only) of the above function
    Vc_ALWAYS_INLINE ReadAccess operator[](IndexType indexes) const
    {
        return ReadAccess(m_data, indexes);
    }

    /// alias of the above function
    Vc_ALWAYS_INLINE ReadAccess gather(IndexType indexes) const { return operator[](indexes); }

    /**
     * Interleaved access.
     *
     * This function is an optimization of the function above, for cases where the index vector
     * contains consecutive values. It will load \p V::Size consecutive entries from memory and
     * deinterleave them into Vc vectors.
     *
     * \param first The first of \p V::Size indizes to be accessed.
     *
     * \return A special (magic) object that executes the loads and deinterleave on assignment to a
     * vector tuple.
     *
     * Example:
     * \code
     * struct Foo {
     *   float x, y, z;
     * };
     *
     * void foo(Foo *_data)
     * {
     *   Vc::InterleavedMemoryWrapper<Foo, float_v> data(_data);
     *   for (size_t i = 0; i < 32U; i += float_v::Size) {
     *     float_v x, y, z;
     *     (x, y, z) = data[i];
     *     // now:
     *     // x = { _data[i].x, _data[i + 1].x, _data[i + 2].x, ... }
     *     // y = { _data[i].y, _data[i + 1].y, _data[i + 2].y, ... }
     *     // z = { _data[i].z, _data[i + 1].z, _data[i + 2].z, ... }
     *     ...
     *   }
     * }
     * \endcode
     */
    Vc_ALWAYS_INLINE ReadSuccessiveEntries operator[](size_t first) const
    {
        return ReadSuccessiveEntries(m_data, first);
    }

    Vc_ALWAYS_INLINE AccessSuccessiveEntries operator[](size_t first)
    {
        return AccessSuccessiveEntries(m_data, first);
    }

    //Vc_ALWAYS_INLINE Access scatter(I indexes, VArg v0, VArg v1);
};
}  // namespace Common

using Common::InterleavedMemoryWrapper;

/**
 * Creates an adapter around a given array of structure (AoS) that enables optimized loads
 * + deinterleaving operations / interleaving operations + stores for vector access (using
 * \p V).
 *
 * \tparam V The `Vc::Vector<T>` type to use per element of the structure.
 * \param s A pointer to an array of structures containing data members of type `T`.
 *
 * \see Vc::Common::InterleavedMemoryWrapper
 *
 * \todo Support destructuring via structured bindings.
 */
template <typename V, typename S>
inline Common::InterleavedMemoryWrapper<S, V> make_interleave_wrapper(S *s)
{
    return Common::InterleavedMemoryWrapper<S, V>(s);
}
}  // namespace Vc

#endif // VC_COMMON_INTERLEAVEDMEMORY_H_
