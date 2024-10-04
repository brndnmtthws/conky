/*  This file is part of the Vc library. {{{
Copyright © 2015 Matthias Kretz <kretz@kde.org>

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

#ifndef VC_COMMON_ALIGNEDBASE_H_
#define VC_COMMON_ALIGNEDBASE_H_

#include "types.h"
#include "macros.h"

namespace Vc_VERSIONED_NAMESPACE
{
namespace Detail
{
/**\internal
 * Break the recursion of the function below.
 */
template <typename T> constexpr T max(T a) { return a; }
/**\internal
 * \returns the maximum of all specified arguments.
 */
template <typename T, typename... Ts> constexpr T max(T a, T b, Ts... rest)
{
    return a > b ? max(a, rest...) : max(b, rest...);
}
}  // namespace Detail
namespace Common
{
template <std::size_t> Vc_INTRINSIC void *aligned_malloc(std::size_t);
Vc_ALWAYS_INLINE void free(void *);
}  // namespace Common

/**
 * \ingroup Utilities
 *
 * Helper class to ensure a given alignment.
 *
 * This class reimplements the \c new and \c delete operators to align objects allocated
 * on the heap suitably with the specified alignment \c Alignment.
 *
 * \see Vc::VectorAlignedBase
 * \see Vc::MemoryAlignedBase
 */
template <std::size_t Alignment> struct alignas(Alignment) AlignedBase
{
    Vc_FREE_STORE_OPERATORS_ALIGNED(Alignment);
};

/**
 * \ingroup Utilities
 *
 * Helper type to ensure suitable alignment for any Vc::Vector<T> type (using the default
 * VectorAbi).
 *
 * This class reimplements the \c new and \c delete operators to align objects allocated
 * on the heap suitably for objects of Vc::Vector<T> type. This is necessary since the
 * standard \c new operator does not adhere to the alignment requirements of the type.
 *
 * \see Vc::VectorAlignedBaseT
 * \see Vc::MemoryAlignedBase
 * \see Vc::AlignedBase
 */
using VectorAlignedBase = AlignedBase<
    Detail::max(alignof(Vector<float>), alignof(Vector<double>), alignof(Vector<ullong>),
                alignof(Vector<llong>), alignof(Vector<ulong>), alignof(Vector<long>),
                alignof(Vector<uint>), alignof(Vector<int>), alignof(Vector<ushort>),
                alignof(Vector<short>), alignof(Vector<uchar>), alignof(Vector<schar>))>;

/**
 * \ingroup Utilities
 * Variant of the above type ensuring suitable alignment only for the specified vector
 * type \p V.
 *
 * \see Vc::VectorAlignedBase
 * \see Vc::MemoryAlignedBaseT
 */
template <typename V> using VectorAlignedBaseT = AlignedBase<alignof(V)>;

/**
 * \ingroup Utilities
 *
 * Helper class to ensure suitable alignment for arrays of scalar objects for any
 * Vc::Vector<T> type (using the default VectorAbi).
 *
 * This class reimplements the \c new and \c delete operators to align objects allocated
 * on the heap suitably for arrays of type \p Vc::Vector<T>::EntryType. Subsequent load
 * and store operations are safe to use the aligned variant.
 *
 * \see Vc::MemoryAlignedBaseT
 * \see Vc::VectorAlignedBase
 * \see Vc::AlignedBase
 */
using MemoryAlignedBase = AlignedBase<
    Detail::max(Vector<float>::MemoryAlignment, Vector<double>::MemoryAlignment,
                Vector<ullong>::MemoryAlignment, Vector<llong>::MemoryAlignment,
                Vector<ulong>::MemoryAlignment, Vector<long>::MemoryAlignment,
                Vector<uint>::MemoryAlignment, Vector<int>::MemoryAlignment,
                Vector<ushort>::MemoryAlignment, Vector<short>::MemoryAlignment,
                Vector<uchar>::MemoryAlignment, Vector<schar>::MemoryAlignment)>;

/**
 * \ingroup Utilities
 * Variant of the above type ensuring suitable alignment only for the specified vector
 * type \p V.
 *
 * \see Vc::MemoryAlignedBase
 * \see Vc::VectorAlignedBaseT
 */
template <typename V> using MemoryAlignedBaseT = AlignedBase<V::MemoryAlignment>;
}

#endif  // VC_COMMON_ALIGNEDBASE_H_

// vim: foldmethod=marker
