/*  This file is part of the Vc library. {{{
Copyright © 2009-2015 Matthias Kretz <kretz@kde.org>

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

#ifndef VC_COMMON_MEMORYBASE_H_
#define VC_COMMON_MEMORYBASE_H_

#include <assert.h>
#include <type_traits>
#include <iterator>
#include "macros.h"

namespace Vc_VERSIONED_NAMESPACE
{
namespace Common
{

#define Vc_MEM_OPERATOR_EQ(op) \
        template<typename T> \
        Vc_ALWAYS_INLINE enable_if_mutable<T, MemoryVector &> operator op##=(const T &x) { \
            const V v = value() op x; \
            v.store(&m_data[0], Flags()); \
            return *this; \
        }
/*dox{{{*/
/**
 * Helper class for the Memory::vector(size_t) class of functions.
 *
 * You will never need to directly make use of this class. It is an implementation detail of the
 * Memory API.
 *
 * \headerfile memorybase.h <Vc/Memory>
 *//*}}}*/
template<typename _V, typename Flags> class MemoryVector/*{{{*/
{
    typedef typename std::remove_cv<_V>::type V;

    template<typename T, typename R> using enable_if_mutable =
        typename std::enable_if<std::is_same<T, T>::value && !std::is_const<_V>::value, R>::type;

    using EntryType =
        typename std::conditional<std::is_const<_V>::value, const typename V::EntryType,
                                  typename V::EntryType>::type;
    typedef typename V::Mask Mask;

    EntryType m_data[V::Size];

public:
        // It is important that neither initialization nor cleanup is done as MemoryVector aliases
        // other memory
        Vc_INTRINSIC MemoryVector() = default;

        // disable copies because this type is supposed to alias the data in a Memory object,
        // nothing else
        MemoryVector(const MemoryVector &) = delete;
        MemoryVector(MemoryVector &&) = delete;
        // Do not disable MemoryVector &operator=(const MemoryVector &) = delete; because it is
        // covered nicely by the operator= below.

        //! \internal
        Vc_ALWAYS_INLINE Vc_PURE V value() const { return V(&m_data[0], Flags()); }

        /**
         * Cast to \p V operator.
         *
         * This function allows to assign this object to any object of type \p V.
         */
        Vc_ALWAYS_INLINE Vc_PURE operator V() const { return value(); }

        template<typename T>
        Vc_ALWAYS_INLINE enable_if_mutable<T, MemoryVector &> operator=(const T &x) {
            V v;
            v = x;
            v.store(&m_data[0], Flags());
            return *this;
        }

        Vc_ALL_BINARY(Vc_MEM_OPERATOR_EQ);
        Vc_ALL_ARITHMETICS(Vc_MEM_OPERATOR_EQ);

    Vc_ALWAYS_INLINE EntryType &operator[](size_t i) { return m_data[i]; }
    Vc_ALWAYS_INLINE const EntryType &operator[](size_t i) const { return m_data[i]; }
};

template<typename _V, typename Flags> class MemoryVectorIterator
{
    typedef typename std::remove_cv<_V>::type V;

    template<typename T, typename R> using enable_if_mutable =
        typename std::enable_if<std::is_same<T, T>::value && !std::is_const<_V>::value, R>::type;

    using iterator_traits = std::iterator_traits<MemoryVector<_V, Flags> *>;

    MemoryVector<_V, Flags> *d;
public:
    typedef typename iterator_traits::difference_type difference_type;
    typedef typename iterator_traits::value_type value_type;
    typedef typename iterator_traits::pointer pointer;
    typedef typename iterator_traits::reference reference;
    typedef typename iterator_traits::iterator_category iterator_category;

    constexpr MemoryVectorIterator(MemoryVector<_V, Flags> *dd) : d(dd) {}
    constexpr MemoryVectorIterator(const MemoryVectorIterator &) = default;
    constexpr MemoryVectorIterator(MemoryVectorIterator &&) = default;
    Vc_ALWAYS_INLINE MemoryVectorIterator &operator=(const MemoryVectorIterator &) = default;

    Vc_ALWAYS_INLINE void *orderBy() const { return d; }

    Vc_ALWAYS_INLINE difference_type operator-(const MemoryVectorIterator &rhs) const { return d - rhs.d; }
    Vc_ALWAYS_INLINE reference operator[](size_t i) const { return d[i]; }
    Vc_ALWAYS_INLINE reference operator*() const { return *d; }
    Vc_ALWAYS_INLINE pointer operator->() const { return d; }
    Vc_ALWAYS_INLINE MemoryVectorIterator &operator++() { ++d; return *this; }
    Vc_ALWAYS_INLINE MemoryVectorIterator operator++(int) { MemoryVectorIterator r(*this); ++d; return r; }
    Vc_ALWAYS_INLINE MemoryVectorIterator &operator--() { --d; return *this; }
    Vc_ALWAYS_INLINE MemoryVectorIterator operator--(int) { MemoryVectorIterator r(*this); --d; return r; }
    Vc_ALWAYS_INLINE MemoryVectorIterator &operator+=(size_t n) { d += n; return *this; }
    Vc_ALWAYS_INLINE MemoryVectorIterator &operator-=(size_t n) { d -= n; return *this; }
    Vc_ALWAYS_INLINE MemoryVectorIterator operator+(size_t n) const { return MemoryVectorIterator(d + n); }
    Vc_ALWAYS_INLINE MemoryVectorIterator operator-(size_t n) const { return MemoryVectorIterator(d - n); }
};

template<typename V, typename FlagsL, typename FlagsR>
Vc_ALWAYS_INLINE bool operator==(const MemoryVectorIterator<V, FlagsL> &l, const MemoryVectorIterator<V, FlagsR> &r)
{
    return l.orderBy() == r.orderBy();
}
template<typename V, typename FlagsL, typename FlagsR>
Vc_ALWAYS_INLINE bool operator!=(const MemoryVectorIterator<V, FlagsL> &l, const MemoryVectorIterator<V, FlagsR> &r)
{
    return l.orderBy() != r.orderBy();
}
template<typename V, typename FlagsL, typename FlagsR>
Vc_ALWAYS_INLINE bool operator>=(const MemoryVectorIterator<V, FlagsL> &l, const MemoryVectorIterator<V, FlagsR> &r)
{
    return l.orderBy() >= r.orderBy();
}
template<typename V, typename FlagsL, typename FlagsR>
Vc_ALWAYS_INLINE bool operator<=(const MemoryVectorIterator<V, FlagsL> &l, const MemoryVectorIterator<V, FlagsR> &r)
{
    return l.orderBy() <= r.orderBy();
}
template<typename V, typename FlagsL, typename FlagsR>
Vc_ALWAYS_INLINE bool operator> (const MemoryVectorIterator<V, FlagsL> &l, const MemoryVectorIterator<V, FlagsR> &r)
{
    return l.orderBy() >  r.orderBy();
}
template<typename V, typename FlagsL, typename FlagsR>
Vc_ALWAYS_INLINE bool operator< (const MemoryVectorIterator<V, FlagsL> &l, const MemoryVectorIterator<V, FlagsR> &r)
{
    return l.orderBy() <  r.orderBy();
}
/*}}}*/
#undef Vc_MEM_OPERATOR_EQ

#define Vc_VPH_OPERATOR(op)                                                              \
    template <typename V1, typename Flags1, typename V2, typename Flags2>                \
    decltype(std::declval<V1>() op std::declval<V2>()) operator op(                      \
        const MemoryVector<V1, Flags1> &x, const MemoryVector<V2, Flags2> &y)            \
    {                                                                                    \
        return x.value() op y.value();                                                   \
    }
Vc_ALL_ARITHMETICS(Vc_VPH_OPERATOR);
Vc_ALL_BINARY     (Vc_VPH_OPERATOR);
Vc_ALL_COMPARES   (Vc_VPH_OPERATOR);
#undef Vc_VPH_OPERATOR

template<typename V, typename Parent, typename Flags = Prefetch<>> class MemoryRange/*{{{*/
{
    Parent *m_parent;
    size_t m_first;
    size_t m_last;

public:
    MemoryRange(Parent *p, size_t firstIndex, size_t lastIndex)
        : m_parent(p), m_first(firstIndex), m_last(lastIndex)
    {}

    MemoryVectorIterator<V, Flags> begin() const { return &m_parent->vector(m_first   , Flags()); }
    MemoryVectorIterator<V, Flags> end() const   { return &m_parent->vector(m_last + 1, Flags()); }
};/*}}}*/
template<typename V, typename Parent, int Dimension, typename RowMemory> class MemoryDimensionBase;
template<typename V, typename Parent, typename RowMemory> class MemoryDimensionBase<V, Parent, 1, RowMemory> // {{{1
{
    private:
        Parent *p() { return static_cast<Parent *>(this); }
        const Parent *p() const { return static_cast<const Parent *>(this); }
    public:
        /**
         * The type of the scalar entries in the array.
         */
        typedef typename V::EntryType EntryType;

        /**
         * Returns a pointer to the start of the allocated memory.
         */
        Vc_ALWAYS_INLINE Vc_PURE       EntryType *entries()       { return &p()->m_mem[0]; }
        /// Const overload of the above function.
        Vc_ALWAYS_INLINE Vc_PURE const EntryType *entries() const { return &p()->m_mem[0]; }

        /**
         * Returns the \p i-th scalar value in the memory.
         */
        Vc_ALWAYS_INLINE Vc_PURE EntryType &scalar(size_t i) { return entries()[i]; }
        /// Const overload of the above function.
        Vc_ALWAYS_INLINE Vc_PURE const EntryType scalar(size_t i) const { return entries()[i]; }

#ifdef DOXYGEN
        /**
         * Cast operator to the scalar type. This allows to use the object very much like a standard
         * C array.
         */
        Vc_ALWAYS_INLINE Vc_PURE operator       EntryType*()       { return entries(); }
        /// Const overload of the above function.
        Vc_ALWAYS_INLINE Vc_PURE operator const EntryType*() const { return entries(); }
#else
        // The above conversion operator allows implicit conversion to bool. To prohibit this
        // conversion we use SFINAE to allow only conversion to EntryType* and void*.
        template <typename T,
                  typename std::enable_if<
                      std::is_same<typename std::remove_const<T>::type, EntryType *>::value ||
                          std::is_same<typename std::remove_const<T>::type, void *>::value,
                      int>::type = 0>
        Vc_ALWAYS_INLINE Vc_PURE operator T()
        {
            return entries();
        }
        template <typename T,
                  typename std::enable_if<std::is_same<T, const EntryType *>::value ||
                                              std::is_same<T, const void *>::value,
                                          int>::type = 0>
        Vc_ALWAYS_INLINE Vc_PURE operator T() const
        {
            return entries();
        }
#endif

        /**
         *
         */
        template<typename Flags>
        Vc_ALWAYS_INLINE MemoryRange<V, Parent, Flags> range(size_t firstIndex, size_t lastIndex, Flags) {
            return MemoryRange<V, Parent, Flags>(p(), firstIndex, lastIndex);
        }
        Vc_ALWAYS_INLINE MemoryRange<V, Parent> range(size_t firstIndex, size_t lastIndex) {
            return MemoryRange<V, Parent>(p(), firstIndex, lastIndex);
        }
        template<typename Flags>
        Vc_ALWAYS_INLINE MemoryRange<const V, Parent, Flags> range(size_t firstIndex, size_t lastIndex, Flags) const {
            return MemoryRange<const V, Parent, Flags>(p(), firstIndex, lastIndex);
        }
        Vc_ALWAYS_INLINE MemoryRange<const V, Parent> range(size_t firstIndex, size_t lastIndex) const {
            return MemoryRange<const V, Parent>(p(), firstIndex, lastIndex);
        }

        /**
         * Returns the \p i-th scalar value in the memory.
         */
        Vc_ALWAYS_INLINE EntryType &operator[](size_t i) { return entries()[i]; }
        /// Const overload of the above function.
        Vc_ALWAYS_INLINE const EntryType &operator[](size_t i) const { return entries()[i]; }

        /**
         * Uses a vector gather to combine the entries at the indexes in \p i into the returned
         * vector object.
         *
         * \param i  An integer vector. It determines the entries to be gathered.
         * \returns  A vector object. Modification of this object will not modify the values in
         *           memory.
         *
         * \warning  The API of this function might change in future versions of Vc to additionally
         *           support scatters.
         */
        template<typename IndexT> Vc_ALWAYS_INLINE Vc_PURE V operator[](Vector<IndexT> i) const
        {
            return V(entries(), i);
        }
};
template<typename V, typename Parent, typename RowMemory> class MemoryDimensionBase<V, Parent, 2, RowMemory> // {{{1
{
    private:
        Parent *p() { return static_cast<Parent *>(this); }
        const Parent *p() const { return static_cast<const Parent *>(this); }
    public:
        /**
         * The type of the scalar entries in the array.
         */
        typedef typename V::EntryType EntryType;

        static constexpr size_t rowCount() { return Parent::RowCount; }

        /**
         * Returns a pointer to the start of the allocated memory.
         */
        Vc_ALWAYS_INLINE Vc_PURE       EntryType *entries(size_t x = 0)       { return &p()->m_mem[x][0]; }
        /// Const overload of the above function.
        Vc_ALWAYS_INLINE Vc_PURE const EntryType *entries(size_t x = 0) const { return &p()->m_mem[x][0]; }

        /**
         * Returns the \p i,j-th scalar value in the memory.
         */
        Vc_ALWAYS_INLINE Vc_PURE EntryType &scalar(size_t i, size_t j) { return entries(i)[j]; }
        /// Const overload of the above function.
        Vc_ALWAYS_INLINE Vc_PURE const EntryType scalar(size_t i, size_t j) const { return entries(i)[j]; }

        /**
         * Returns the \p i-th row in the memory.
         */
        Vc_ALWAYS_INLINE Vc_PURE RowMemory &operator[](size_t i) {
            return p()->m_mem[i];
        }
        /// Const overload of the above function.
        Vc_ALWAYS_INLINE Vc_PURE const RowMemory &operator[](size_t i) const {
            return p()->m_mem[i];
        }

        /**
         * \return the number of rows in the array.
         *
         * \note This function can be eliminated by an optimizing compiler.
         */
        Vc_ALWAYS_INLINE Vc_PURE size_t rowsCount() const { return p()->rowsCount(); }
};

//dox{{{1
/**
 * \headerfile memorybase.h <Vc/Memory>
 *
 * Common interface to all Memory classes, independent of allocation on the stack or heap.
 *
 * \param V The vector type you want to operate on. (e.g. float_v or uint_v)
 * \param Parent This type is the complete type of the class that derives from MemoryBase.
 * \param Dimension The number of dimensions the implementation provides.
 * \param RowMemory Class to be used to work on a single row.
 */
template<typename V, typename Parent, int Dimension, typename RowMemory> class MemoryBase : public MemoryDimensionBase<V, Parent, Dimension, RowMemory> //{{{1
{
    static_assert((V::size() * sizeof(typename V::EntryType)) % V::MemoryAlignment == 0,
                  "Vc::Memory can only be used for data-parallel types storing a number "
                  "of values that's a multiple of the memory alignment.");

    private:
        Parent *p() { return static_cast<Parent *>(this); }
        const Parent *p() const { return static_cast<const Parent *>(this); }

        template <class Flags>
        using vector_reference = MayAlias<MemoryVector<V, Flags>> &;
        template <class Flags>
        using const_vector_reference = const MayAlias<MemoryVector<const V, Flags>> &;

    public:
        /**
         * The type of the scalar entries in the array.
         */
        typedef typename V::EntryType EntryType;

        /**
         * \return the number of scalar entries in the array. This function is optimized away
         * if a constant size array is used.
         */
        Vc_ALWAYS_INLINE Vc_PURE size_t entriesCount() const { return p()->entriesCount(); }
        /**
         * \return the number of vector entries that span the array. This function is optimized away
         * if a constant size array is used.
         */
        Vc_ALWAYS_INLINE Vc_PURE size_t vectorsCount() const { return p()->vectorsCount(); }

        using MemoryDimensionBase<V, Parent, Dimension, RowMemory>::entries;
        using MemoryDimensionBase<V, Parent, Dimension, RowMemory>::scalar;

        /**
         * Return a (vectorized) iterator to the start of this memory object.
         */
        template<typename Flags = AlignedTag>
        Vc_ALWAYS_INLINE MemoryVectorIterator<      V, Flags> begin(Flags flags = Flags())       { return &firstVector(flags); }
        //! const overload of the above
        template<typename Flags = AlignedTag>
        Vc_ALWAYS_INLINE MemoryVectorIterator<const V, Flags> begin(Flags flags = Flags()) const { return &firstVector(flags); }

        /**
         * Return a (vectorized) iterator to the end of this memory object.
         */
        template<typename Flags = AlignedTag>
        Vc_ALWAYS_INLINE MemoryVectorIterator<      V, Flags>   end(Flags flags = Flags())       { return &lastVector(flags) + 1; }
        //! const overload of the above
        template<typename Flags = AlignedTag>
        Vc_ALWAYS_INLINE MemoryVectorIterator<const V, Flags>   end(Flags flags = Flags()) const { return &lastVector(flags) + 1; }

        /**
         * \param i Selects the offset, where the vector should be read.
         *
         * \return a smart object to wrap the \p i-th vector in the memory.
         *
         * The return value can be used as any other vector object. I.e. you can substitute
         * something like
         * \code
         * float_v a = ..., b = ...;
         * a += b;
         * \endcode
         * with
         * \code
         * mem.vector(i) += b;
         * \endcode
         *
         * This function ensures that only \em aligned loads and stores are used. Thus it only allows to
         * access memory at fixed strides. If access to known offsets from the aligned vectors is
         * needed the vector(size_t, int) function can be used.
         */
        template <typename Flags = AlignedTag>
        Vc_ALWAYS_INLINE Vc_PURE
            typename std::enable_if<!std::is_convertible<Flags, int>::value,
                                    vector_reference<Flags>>::type
            vector(size_t i, Flags = Flags())
        {
            return *aliasing_cast<MemoryVector<V, Flags>>(&entries()[i * V::Size]);
        }
        /** \brief Const overload of the above function
         *
         * \param i Selects the offset, where the vector should be read.
         *
         * \return a smart object to wrap the \p i-th vector in the memory.
         */
        template <typename Flags = AlignedTag>
        Vc_ALWAYS_INLINE Vc_PURE
            typename std::enable_if<!std::is_convertible<Flags, int>::value,
                                    const_vector_reference<Flags>>::type
            vector(size_t i, Flags = Flags()) const
        {
            return *aliasing_cast<MemoryVector<const V, Flags>>(&entries()[i * V::Size]);
        }

        /**
         * \return a smart object to wrap the vector starting from the \p i-th scalar entry in the memory.
         *
         * Example:
         * \code
         * Memory<float_v, N> mem;
         * mem.setZero();
         * for (int i = 0; i < mem.entriesCount(); i += float_v::Size) {
         *     mem.vectorAt(i) += b;
         * }
         * \endcode
         *
         * \param i      Specifies the scalar entry from where the vector will be loaded/stored. I.e. the
         * values scalar(i), scalar(i + 1), ..., scalar(i + V::Size - 1) will be read/overwritten.
         *
         * \param flags  You must take care to determine whether an unaligned load/store is
         * required. Per default an unaligned load/store is used. If \p i is a multiple of \c V::Size
         * you may want to pass Vc::Aligned here.
         */
        template <typename Flags = UnalignedTag>
        Vc_ALWAYS_INLINE Vc_PURE vector_reference<Flags> vectorAt(size_t i,
                                                                  Flags flags = Flags())
        {
            return *aliasing_cast<MemoryVector<V, Flags>>(&entries()[i]);
        }
        /** \brief Const overload of the above function
         *
         * \return a smart object to wrap the vector starting from the \p i-th scalar entry in the memory.
         *
         * \param i      Specifies the scalar entry from where the vector will be loaded/stored. I.e. the
         * values scalar(i), scalar(i + 1), ..., scalar(i + V::Size - 1) will be read/overwritten.
         *
         * \param flags  You must take care to determine whether an unaligned load/store is
         * required. Per default an unaligned load/store is used. If \p i is a multiple of \c V::Size
         * you may want to pass Vc::Aligned here.
         */
        template <typename Flags = UnalignedTag>
        Vc_ALWAYS_INLINE Vc_PURE const_vector_reference<Flags> vectorAt(
            size_t i, Flags flags = Flags()) const
        {
            return *aliasing_cast<MemoryVector<const V, Flags>>(&entries()[i]);
        }

        /**
         * \return a smart object to wrap the \p i-th vector + \p shift in the memory.
         *
         * This function ensures that only \em unaligned loads and stores are used.
         * It allows to access memory at any location aligned to the entry type.
         *
         * \param i Selects the memory location of the i-th vector. Thus if \p V::Size == 4 and
         *          \p i is set to 3 the base address for the load/store will be the 12th entry
         *          (same as \p &mem[12]).
         * \param shift Shifts the base address determined by parameter \p i by \p shift many
         *              entries. Thus \p vector(3, 1) for \p V::Size == 4 will load/store the
         *              13th - 16th entries (same as \p &mem[13]).
         *
         * \note Any shift value is allowed as long as you make sure it stays within bounds of the
         * allocated memory. Shift values that are a multiple of \p V::Size will \em not result in
         * aligned loads. You have to use the above vector(size_t) function for aligned loads
         * instead.
         *
         * \note Thus a simple way to access vectors randomly is to set \p i to 0 and use \p shift as the
         * parameter to select the memory address:
         * \code
         * // don't use:
         * mem.vector(i / V::Size, i % V::Size) += 1;
         * // instead use:
         * mem.vector(0, i) += 1;
         * \endcode
         */
        template <typename ShiftT, typename Flags = decltype(Unaligned)>
        Vc_ALWAYS_INLINE Vc_PURE typename std::enable_if<
            std::is_convertible<ShiftT, int>::value,
            vector_reference<decltype(std::declval<Flags>() | Unaligned)>>::type
        vector(size_t i, ShiftT shift, Flags = Flags())
        {
            return *aliasing_cast<
                MemoryVector<V, decltype(std::declval<Flags>() | Unaligned)>>(
                &entries()[i * V::Size + shift]);
        }
        /// Const overload of the above function.
        template <typename ShiftT, typename Flags = decltype(Unaligned)>
        Vc_ALWAYS_INLINE Vc_PURE typename std::enable_if<
            std::is_convertible<ShiftT, int>::value,
            const_vector_reference<decltype(std::declval<Flags>() | Unaligned)>>::type
        vector(size_t i, ShiftT shift, Flags = Flags()) const
        {
            return *aliasing_cast<
                MemoryVector<const V, decltype(std::declval<Flags>() | Unaligned)>>(
                &entries()[i * V::Size + shift]);
        }

        /**
         * \return the first vector in the allocated memory.
         *
         * This function is simply a shorthand for vector(0).
         */
        template <typename Flags = AlignedTag>
        Vc_ALWAYS_INLINE Vc_PURE vector_reference<Flags> firstVector(Flags f = Flags())
        {
            return vector(0, f);
        }
        /// Const overload of the above function.
        template <typename Flags = AlignedTag>
        Vc_ALWAYS_INLINE Vc_PURE const_vector_reference<Flags> firstVector(
            Flags f = Flags()) const
        {
            return vector(0, f);
        }

        /**
         * \return the last vector in the allocated memory.
         *
         * This function is simply a shorthand for vector(vectorsCount() - 1).
         */
        template <typename Flags = AlignedTag>
        Vc_ALWAYS_INLINE Vc_PURE vector_reference<Flags> lastVector(Flags f = Flags())
        {
            return vector(vectorsCount() - 1, f);
        }
        /// Const overload of the above function.
        template <typename Flags = AlignedTag>
        Vc_ALWAYS_INLINE Vc_PURE const_vector_reference<Flags> lastVector(
            Flags f = Flags()) const
        {
            return vector(vectorsCount() - 1, f);
        }

        Vc_ALWAYS_INLINE Vc_PURE V gather(const unsigned char  *indexes) const { return V(entries(), typename V::IndexType(indexes, Vc::Unaligned)); }
        Vc_ALWAYS_INLINE Vc_PURE V gather(const unsigned short *indexes) const { return V(entries(), typename V::IndexType(indexes, Vc::Unaligned)); }
        Vc_ALWAYS_INLINE Vc_PURE V gather(const unsigned int   *indexes) const { return V(entries(), typename V::IndexType(indexes, Vc::Unaligned)); }
        Vc_ALWAYS_INLINE Vc_PURE V gather(const unsigned long  *indexes) const { return V(entries(), typename V::IndexType(indexes, Vc::Unaligned)); }

        /**
         * Zero the whole memory area.
         */
        Vc_ALWAYS_INLINE void setZero() {
            V zero(Vc::Zero);
            for (size_t i = 0; i < vectorsCount(); ++i) {
                vector(i) = zero;
            }
        }

        /**
         * Assign a value to all vectors in the array.
         */
        template<typename U>
        Vc_ALWAYS_INLINE Parent &operator=(U &&x) {
            for (size_t i = 0; i < vectorsCount(); ++i) {
                vector(i) = std::forward<U>(x);
            }
        }

        /**
         * (Inefficient) shorthand to add up two arrays.
         */
        template<typename P2, typename RM>
        inline Parent &operator+=(const MemoryBase<V, P2, Dimension, RM> &rhs) {
            assert(vectorsCount() == rhs.vectorsCount());
            for (size_t i = 0; i < vectorsCount(); ++i) {
                vector(i) += rhs.vector(i);
            }
            return static_cast<Parent &>(*this);
        }

        /**
         * (Inefficient) shorthand to subtract two arrays.
         */
        template<typename P2, typename RM>
        inline Parent &operator-=(const MemoryBase<V, P2, Dimension, RM> &rhs) {
            assert(vectorsCount() == rhs.vectorsCount());
            for (size_t i = 0; i < vectorsCount(); ++i) {
                vector(i) -= rhs.vector(i);
            }
            return static_cast<Parent &>(*this);
        }

        /**
         * (Inefficient) shorthand to multiply two arrays.
         */
        template<typename P2, typename RM>
        inline Parent &operator*=(const MemoryBase<V, P2, Dimension, RM> &rhs) {
            assert(vectorsCount() == rhs.vectorsCount());
            for (size_t i = 0; i < vectorsCount(); ++i) {
                vector(i) *= rhs.vector(i);
            }
            return static_cast<Parent &>(*this);
        }

        /**
         * (Inefficient) shorthand to divide two arrays.
         */
        template<typename P2, typename RM>
        inline Parent &operator/=(const MemoryBase<V, P2, Dimension, RM> &rhs) {
            assert(vectorsCount() == rhs.vectorsCount());
            for (size_t i = 0; i < vectorsCount(); ++i) {
                vector(i) /= rhs.vector(i);
            }
            return static_cast<Parent &>(*this);
        }

        /**
         * (Inefficient) shorthand to add a value to an array.
         */
        inline Parent &operator+=(EntryType rhs) {
            V v(rhs);
            for (size_t i = 0; i < vectorsCount(); ++i) {
                vector(i) += v;
            }
            return static_cast<Parent &>(*this);
        }

        /**
         * (Inefficient) shorthand to subtract a value from an array.
         */
        inline Parent &operator-=(EntryType rhs) {
            V v(rhs);
            for (size_t i = 0; i < vectorsCount(); ++i) {
                vector(i) -= v;
            }
            return static_cast<Parent &>(*this);
        }

        /**
         * (Inefficient) shorthand to multiply a value to an array.
         */
        inline Parent &operator*=(EntryType rhs) {
            V v(rhs);
            for (size_t i = 0; i < vectorsCount(); ++i) {
                vector(i) *= v;
            }
            return static_cast<Parent &>(*this);
        }

        /**
         * (Inefficient) shorthand to divide an array with a value.
         */
        inline Parent &operator/=(EntryType rhs) {
            V v(rhs);
            for (size_t i = 0; i < vectorsCount(); ++i) {
                vector(i) /= v;
            }
            return static_cast<Parent &>(*this);
        }

        /**
         * (Inefficient) shorthand compare equality of two arrays.
         */
        template<typename P2, typename RM>
        inline bool operator==(const MemoryBase<V, P2, Dimension, RM> &rhs) const {
            assert(vectorsCount() == rhs.vectorsCount());
            for (size_t i = 0; i < vectorsCount(); ++i) {
                if (!(V(vector(i)) == V(rhs.vector(i))).isFull()) {
                    return false;
                }
            }
            return true;
        }

        /**
         * (Inefficient) shorthand compare two arrays.
         */
        template<typename P2, typename RM>
        inline bool operator!=(const MemoryBase<V, P2, Dimension, RM> &rhs) const {
            assert(vectorsCount() == rhs.vectorsCount());
            for (size_t i = 0; i < vectorsCount(); ++i) {
                if (!(V(vector(i)) == V(rhs.vector(i))).isEmpty()) {
                    return false;
                }
            }
            return true;
        }

        /**
         * (Inefficient) shorthand compare two arrays.
         */
        template<typename P2, typename RM>
        inline bool operator<(const MemoryBase<V, P2, Dimension, RM> &rhs) const {
            assert(vectorsCount() == rhs.vectorsCount());
            for (size_t i = 0; i < vectorsCount(); ++i) {
                if (!(V(vector(i)) < V(rhs.vector(i))).isFull()) {
                    return false;
                }
            }
            return true;
        }

        /**
         * (Inefficient) shorthand compare two arrays.
         */
        template<typename P2, typename RM>
        inline bool operator<=(const MemoryBase<V, P2, Dimension, RM> &rhs) const {
            assert(vectorsCount() == rhs.vectorsCount());
            for (size_t i = 0; i < vectorsCount(); ++i) {
                if (!(V(vector(i)) <= V(rhs.vector(i))).isFull()) {
                    return false;
                }
            }
            return true;
        }

        /**
         * (Inefficient) shorthand compare two arrays.
         */
        template<typename P2, typename RM>
        inline bool operator>(const MemoryBase<V, P2, Dimension, RM> &rhs) const {
            assert(vectorsCount() == rhs.vectorsCount());
            for (size_t i = 0; i < vectorsCount(); ++i) {
                if (!(V(vector(i)) > V(rhs.vector(i))).isFull()) {
                    return false;
                }
            }
            return true;
        }

        /**
         * (Inefficient) shorthand compare two arrays.
         */
        template<typename P2, typename RM>
        inline bool operator>=(const MemoryBase<V, P2, Dimension, RM> &rhs) const {
            assert(vectorsCount() == rhs.vectorsCount());
            for (size_t i = 0; i < vectorsCount(); ++i) {
                if (!(V(vector(i)) >= V(rhs.vector(i))).isFull()) {
                    return false;
                }
            }
            return true;
        }
};

namespace Detail
{
template <typename V,
          typename ParentL,
          typename ParentR,
          int Dimension,
          typename RowMemoryL,
          typename RowMemoryR>
inline void copyVectors(MemoryBase<V, ParentL, Dimension, RowMemoryL> &dst,
                        const MemoryBase<V, ParentR, Dimension, RowMemoryR> &src)
{
    const size_t vectorsCount = dst.vectorsCount();
    size_t i = 3;
    for (; i < vectorsCount; i += 4) {
        const V tmp3 = src.vector(i - 3);
        const V tmp2 = src.vector(i - 2);
        const V tmp1 = src.vector(i - 1);
        const V tmp0 = src.vector(i - 0);
        dst.vector(i - 3) = tmp3;
        dst.vector(i - 2) = tmp2;
        dst.vector(i - 1) = tmp1;
        dst.vector(i - 0) = tmp0;
    }
    for (i -= 3; i < vectorsCount; ++i) {
        dst.vector(i) = src.vector(i);
    }
}
} // namespace Detail

}  // namespace Common
}  // namespace Vc

#endif // VC_COMMON_MEMORYBASE_H_

// vim: foldmethod=marker
