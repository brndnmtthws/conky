/*  This file is part of the Vc library. {{{
Copyright © 2013-2015 Matthias Kretz <kretz@kde.org>

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

#ifndef VC_COMMON_ITERATORS_H_
#define VC_COMMON_ITERATORS_H_

#include <array>
#include <iterator>
#ifdef Vc_MSVC
#include <intrin.h> // for _BitScanForward
#endif  // Vc_MSVC
#include "where.h"
#include "elementreference.h"
#include "macros.h"

namespace Vc_VERSIONED_NAMESPACE
{
namespace Common
{

template<typename _V, typename Flags> class MemoryVector;
template<typename _V, typename Flags> class MemoryVectorIterator;

template <typename V> class Iterator;
template <typename V, bool> class IteratorBase;
template <typename V> class IteratorBase<V, true>
{
public:
    using iterator_category = std::input_iterator_tag;
    using value_type = typename V::value_type;
    using difference_type = int;
    using reference = value_type;
    Vc_ALWAYS_INLINE reference operator*() const { return v()[i()]; }
    Vc_ALWAYS_INLINE reference operator[](difference_type i2) const { return v()[i2]; }

private:
    Vc_INTRINSIC V &v() const { return *static_cast<const Iterator<V> *>(this)->v; }
    Vc_INTRINSIC difference_type i() const
    {
        return static_cast<const Iterator<V> *>(this)->i;
    }
};

template <typename V> class IteratorBase<V, false>
{
public:
    using iterator_category = std::input_iterator_tag;
    using value_type = typename V::value_type;
    using difference_type = int;
    using reference = Vc::Detail::ElementReference<V, IteratorBase>;
    Vc_ALWAYS_INLINE reference operator*() const { return {*v(), i()}; }
    Vc_ALWAYS_INLINE reference operator[](difference_type i2) const { return {*v(), i2}; }

private:
    Vc_INTRINSIC V *v() const { return static_cast<const Iterator<V> *>(this)->v; }
    Vc_INTRINSIC difference_type i() const
    {
        return static_cast<const Iterator<V> *>(this)->i;
    }

    friend reference;
    static Vc_INTRINSIC value_type get(const V &o, int i)
    {
        return o[i];
    }
    template <typename T> static Vc_INTRINSIC void set(V &o, int i, T &&v)
    {
        o[i] = std::forward<T>(v);
    }
};

// class Iterator {{{
template <typename V> class Iterator : public IteratorBase<V, std::is_const<V>::value>
{
    using Base = IteratorBase<V, std::is_const<V>::value>;
    friend Base;

public:
    using typename Base::iterator_category;
    using typename Base::value_type;
    using typename Base::difference_type;
    using pointer = const Iterator *;
    using typename Base::reference;

    constexpr Iterator() = default;
    constexpr Iterator(V &_v, difference_type _i) : v(&_v), i(_i) {}
    // rely on implicit copy constructor/assignment

    Vc_ALWAYS_INLINE pointer operator->() const { return this; }
    using Base::operator*;

    Vc_ALWAYS_INLINE Iterator &operator++()    { ++i; return *this; }
    Vc_ALWAYS_INLINE Iterator  operator++(int) { Iterator tmp = *this; ++i; return tmp; }

    // bidirectional iteration is supported
    Vc_ALWAYS_INLINE Iterator &operator--()    { --i; return *this; }
    Vc_ALWAYS_INLINE Iterator  operator--(int) { Iterator tmp = *this; --i; return tmp; }

    // RandomAccessIterator:
    using Base::operator[];
    Vc_ALWAYS_INLINE Iterator &operator+=(difference_type d) { i += d; return *this; }
    Vc_ALWAYS_INLINE Iterator &operator-=(difference_type d) { i -= d; return *this; }
    Vc_ALWAYS_INLINE Iterator operator+(difference_type d) const { return {*v, i + d}; }
    Vc_ALWAYS_INLINE Iterator operator-(difference_type d) const { return {*v, i - d}; }
    Vc_ALWAYS_INLINE difference_type operator-(const Iterator &rhs) const { return i - rhs.i; }
    friend Vc_ALWAYS_INLINE Iterator operator+(difference_type d, const Iterator &rhs)
    {
        return {*rhs.v, rhs.i + d};
    }

    // InputIterator would not need to test v == rhs.v, but except for `reference` this
    // class implements a complete RandomAccessIterator
    Vc_ALWAYS_INLINE bool operator==(const Iterator<V> &rhs) const { return v == rhs.v && i == rhs.i; }
    Vc_ALWAYS_INLINE bool operator!=(const Iterator<V> &rhs) const { return v == rhs.v && i != rhs.i; }
    Vc_ALWAYS_INLINE bool operator< (const Iterator<V> &rhs) const { return v == rhs.v && i <  rhs.i; }
    Vc_ALWAYS_INLINE bool operator<=(const Iterator<V> &rhs) const { return v == rhs.v && i <= rhs.i; }
    Vc_ALWAYS_INLINE bool operator> (const Iterator<V> &rhs) const { return v == rhs.v && i >  rhs.i; }
    Vc_ALWAYS_INLINE bool operator>=(const Iterator<V> &rhs) const { return v == rhs.v && i >= rhs.i; }

private:
    V *v = nullptr;
    difference_type i = 0;
};/*}}}*/

template <typename V> using ConstIterator = Iterator<const V>;

    class BitmaskIterator/*{{{*/
    {
#ifdef Vc_MSVC
        unsigned long mask;
        unsigned long bit;
#else
        size_t mask;
        size_t bit;
#endif

        void nextBit()
        {
#ifdef Vc_GNU_ASM
            bit = __builtin_ctzl(mask);
#elif defined(Vc_MSVC)
            _BitScanForward(&bit, mask);
#else
#error "Not implemented yet. Please contact vc-devel@compeng.uni-frankfurt.de"
#endif
        }
        void resetLsb()
        {
            // 01100100 - 1 = 01100011
            mask &= (mask - 1);
            /*
#ifdef Vc_GNU_ASM
            __asm__("btr %1,%0" : "+r"(mask) : "r"(bit));
#elif defined(_WIN64)
            _bittestandreset64(&mask, bit);
#elif defined(_WIN32)
            _bittestandreset(&mask, bit);
#else
#error "Not implemented yet. Please contact vc-devel@compeng.uni-frankfurt.de"
#endif
            */
        }
    public:
        BitmaskIterator(decltype(mask) m) : mask(m) { nextBit(); }
        BitmaskIterator(const BitmaskIterator &) = default;
        BitmaskIterator(BitmaskIterator &&) = default;

        Vc_ALWAYS_INLINE size_t operator->() const { return bit; }
        Vc_ALWAYS_INLINE size_t operator*() const { return bit; }

        Vc_ALWAYS_INLINE BitmaskIterator &operator++()    { resetLsb(); nextBit(); return *this; }
        Vc_ALWAYS_INLINE BitmaskIterator  operator++(int) { BitmaskIterator tmp = *this; resetLsb(); nextBit(); return tmp; }

        Vc_ALWAYS_INLINE bool operator==(const BitmaskIterator &rhs) const { return mask == rhs.mask; }
        Vc_ALWAYS_INLINE bool operator!=(const BitmaskIterator &rhs) const { return mask != rhs.mask; }
    };/*}}}*/

template <typename T>
Vc_ALWAYS_INLINE
    enable_if<Traits::is_simd_vector<T>::value || Traits::is_simd_mask<T>::value,
              Iterator<typename std::remove_reference<T>::type>>
    begin(T &&x)
{
    return {std::forward<T>(x), 0};
}

template <typename T>
Vc_ALWAYS_INLINE
    enable_if<Traits::is_simd_vector<T>::value || Traits::is_simd_mask<T>::value,
              Iterator<typename std::remove_reference<T>::type>>
    end(T &&x)
{
    using TT = typename std::decay<T>::type;
    return {std::forward<T>(x), int(TT::size())};
}

template <typename T>
Vc_ALWAYS_INLINE enable_if<
    Traits::is_simd_mask<T>::value || Traits::is_simd_vector<T>::value, ConstIterator<T>>
cbegin(const T &v)
{
    return {v, 0};
}

template <typename T>
Vc_ALWAYS_INLINE enable_if<
    Traits::is_simd_mask<T>::value || Traits::is_simd_vector<T>::value, ConstIterator<T>>
cend(const T &v)
{
    return {v, int(T::size())};
}

template<typename M> Vc_ALWAYS_INLINE BitmaskIterator begin(const WhereImpl::WhereMask<M> &w)
{
    return w.mask.toInt();
}

template<typename M> Vc_ALWAYS_INLINE BitmaskIterator end(const WhereImpl::WhereMask<M> &)
{
    return 0;
}

template<typename V, typename Flags, typename T> Vc_ALWAYS_INLINE MemoryVectorIterator<V, Flags>
    makeIterator(T *mem, Flags)
{
    return new(mem) MemoryVector<V, Flags>;
}

template<typename V, typename Flags, typename T> Vc_ALWAYS_INLINE MemoryVectorIterator<const V, Flags>
    makeIterator(const T *mem, Flags)
{
    return new(const_cast<T *>(mem)) MemoryVector<const V, Flags>;
}

template<typename V, typename Flags, typename FlagsX> Vc_ALWAYS_INLINE MemoryVectorIterator<V, Flags>
    makeIterator(MemoryVector<V, FlagsX> &mv, Flags)
{
    return new(&mv) MemoryVector<V, Flags>;
}

template<typename V, typename Flags, typename FlagsX> Vc_ALWAYS_INLINE MemoryVectorIterator<const V, Flags>
    makeIterator(MemoryVector<const V, FlagsX> &mv, Flags)
{
    return new(&mv) MemoryVector<const V, Flags>;
}

}  // namespace Common

using Common::begin;
using Common::end;
using Common::cbegin;
using Common::cend;
using Common::makeIterator;
}  // namespace Vc

#endif // VC_COMMON_ITERATORS_H_

// vim: foldmethod=marker
