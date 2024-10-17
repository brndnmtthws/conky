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

#ifndef VC_COMMON_TYPES_H_
#define VC_COMMON_TYPES_H_

#ifdef Vc_CHECK_ALIGNMENT
#include <cstdlib>
#include <cstdio>
#endif

#include <ratio>
#include "../global.h"
#include "../traits/type_traits.h"
#include "permutation.h"

namespace Vc_VERSIONED_NAMESPACE
{
///\addtogroup Utilities
///@{

/// \internal Allow writing \c size_t without the `std::` prefix.
using std::size_t;

/// long long shorthand
using llong = long long;
/// unsigned long long shorthand
using ullong = unsigned long long;
/// unsigned long shorthand
using ulong = unsigned long;
/// unsigned int shorthand
using uint = unsigned int;
/// unsigned short shorthand
using ushort = unsigned short;
/// unsigned char shorthand
using uchar = unsigned char;
/// signed char shorthand
using schar = signed char;

/**\internal
 * Tag type for explicit zero-initialization
 */
struct VectorSpecialInitializerZero {};
/**\internal
 * Tag type for explicit one-initialization
 */
struct VectorSpecialInitializerOne {};
/**\internal
 * Tag type for explicit "iota-initialization"
 */
struct VectorSpecialInitializerIndexesFromZero {};

/**
 * The special object \p Vc::Zero can be used to construct Vector and Mask objects
 * initialized to zero/\c false.
 */
constexpr VectorSpecialInitializerZero Zero = {};
/**
 * The special object \p Vc::One can be used to construct Vector and Mask objects
 * initialized to one/\c true.
 */
constexpr VectorSpecialInitializerOne One = {};
/**
 * The special object \p Vc::IndexesFromZero can be used to construct Vector objects
 * initialized to values 0, 1, 2, 3, 4, ...
 */
constexpr VectorSpecialInitializerIndexesFromZero IndexesFromZero = {};
///@}

namespace Detail
{
template<typename T> struct MayAliasImpl {
#ifdef Vc_ICC
#pragma warning(disable:2621)
#endif
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wattributes"
#endif
    typedef T type Vc_MAY_ALIAS;
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif
#ifdef Vc_ICC
#pragma warning(enable:2621)
#endif
};
//template<size_t Bytes> struct MayAlias<MaskBool<Bytes>> { typedef MaskBool<Bytes> type; };
}  // namespace Detail
/**\internal
 * Helper MayAlias<T> that turns T into the type to be used for an aliasing pointer. This
 * adds the may_alias attribute to T (with compilers that support it). But for MaskBool this
 * attribute is already part of the type and applying it a second times leads to warnings/errors,
 * therefore MaskBool is simply forwarded as is.
 */
template <typename T> using MayAlias = typename Detail::MayAliasImpl<T>::type;

template <class To, class From> MayAlias<To> &aliasing_cast(From &x)
{
    return *reinterpret_cast<MayAlias<To> *>(&x);
}
template <class To, class From> const MayAlias<To> &aliasing_cast(const From &x)
{
    return *reinterpret_cast<const MayAlias<To> *>(&x);
}

template <class To, class From> MayAlias<To> *aliasing_cast(From *x)
{
    return reinterpret_cast<MayAlias<To> *>(x);
}
template <class To, class From> const MayAlias<To> *aliasing_cast(const From *x)
{
    return reinterpret_cast<const MayAlias<To> *>(x);
}

/**\internal
 * This enumeration lists all possible operators in C++.
 *
 * The assignment and compound assignment enumerators are used with the conditional_assign
 * implementation.
 */
enum class Operator : char {
    Assign,
    Multiply,
    MultiplyAssign,
    Divide,
    DivideAssign,
    Remainder,
    RemainderAssign,
    Plus,
    PlusAssign,
    Minus,
    MinusAssign,
    RightShift,
    RightShiftAssign,
    LeftShift,
    LeftShiftAssign,
    And,
    AndAssign,
    Xor,
    XorAssign,
    Or,
    OrAssign,
    PreIncrement,
    PostIncrement,
    PreDecrement,
    PostDecrement,
    LogicalAnd,
    LogicalOr,
    Comma,
    UnaryPlus,
    UnaryMinus,
    UnaryNot,
    UnaryOnesComplement,
    CompareEqual,
    CompareNotEqual,
    CompareLess,
    CompareGreater,
    CompareLessEqual,
    CompareGreaterEqual
};

// forward declaration for Vc::array in <Vc/array>
template <typename T, std::size_t N> struct array;
// forward declaration for Vc::span in <Vc/span>
namespace Common {
template <typename T, std::ptrdiff_t N> class span;
}

/* TODO: add type for half-float, something along these lines:
class half_float
{
    uint16_t data;
public:
    constexpr half_float() : data(0) {}
    constexpr half_float(const half_float &) = default;
    constexpr half_float(half_float &&) = default;
    constexpr half_float &operator=(const half_float &) = default;

    constexpr explicit half_float(float);
    constexpr explicit half_float(double);
    constexpr explicit half_float(int);
    constexpr explicit half_float(unsigned int);

    explicit operator float       () const;
    explicit operator double      () const;
    explicit operator int         () const;
    explicit operator unsigned int() const;

    bool operator==(half_float rhs) const;
    bool operator!=(half_float rhs) const;
    bool operator>=(half_float rhs) const;
    bool operator<=(half_float rhs) const;
    bool operator> (half_float rhs) const;
    bool operator< (half_float rhs) const;

    half_float operator+(half_float rhs) const;
    half_float operator-(half_float rhs) const;
    half_float operator*(half_float rhs) const;
    half_float operator/(half_float rhs) const;
};
*/

// TODO: the following doesn't really belong into the toplevel Vc namespace.
#ifndef Vc_CHECK_ALIGNMENT
template<typename _T> static Vc_ALWAYS_INLINE void assertCorrectAlignment(const _T *){}
#else
template<typename _T> static Vc_ALWAYS_INLINE void assertCorrectAlignment(const _T *ptr)
{
    const size_t s = alignof(_T);
    if((reinterpret_cast<size_t>(ptr) & ((s ^ (s & (s - 1))) - 1)) != 0) {
        fprintf(stderr, "A vector with incorrect alignment has just been created. Look at the stacktrace to find the guilty object.\n");
        abort();
    }
}
#endif

namespace Common
{
// defined in common/simdarrayhelper.h
template <typename T, std::size_t Pieces, std::size_t Index> struct Segment;

/**
 * \internal
 *
 * Helper interface to make m_indexes in InterleavedMemoryAccessBase behave like an integer vector.
 * Only that the entries are successive entries from the given start index.
 */
template<size_t StructSize> class SuccessiveEntries
{
#ifdef Vc_MSVC
    // scatterinterleavedmemory fails with garbage values in m_first if size_type is a
    // 64-bit integer type. Using a 32-bit type seems to work around the miscompilation.
    using size_type = unsigned;
#else
    using size_type = size_t;
#endif
    const size_type m_first;

public:
    typedef SuccessiveEntries AsArg;
    Vc_INTRINSIC SuccessiveEntries(size_type first) : m_first(first) {}
    Vc_INTRINSIC Vc_PURE size_type operator[](size_type offset) const
    {
        return m_first + offset * StructSize;
    }
    Vc_INTRINSIC Vc_PURE size_type data() const { return m_first; }
    Vc_INTRINSIC Vc_PURE SuccessiveEntries operator+(const SuccessiveEntries &rhs) const
    {
        return SuccessiveEntries(m_first + rhs.m_first);
    }
    Vc_INTRINSIC Vc_PURE SuccessiveEntries operator*(const SuccessiveEntries &rhs) const
    {
        return SuccessiveEntries(m_first * rhs.m_first);
    }
    Vc_INTRINSIC Vc_PURE SuccessiveEntries operator<<(size_type x) const
    {
        return {m_first << x};
    }

    friend Vc_INTRINSIC SuccessiveEntries &internal_data(SuccessiveEntries &x)
    {
        return x;
    }
    friend Vc_INTRINSIC const SuccessiveEntries &internal_data(const SuccessiveEntries &x)
    {
        return x;
    }
};

// declaration for functions in common/malloc.h
template <std::size_t alignment>
Vc_INTRINSIC_L void *aligned_malloc(std::size_t n) Vc_INTRINSIC_R;
Vc_ALWAYS_INLINE_L void free(void *p) Vc_ALWAYS_INLINE_R;

/**\internal
 * Central definition of the type combinations that convert implicitly.
 */
template <typename Mask, typename T, typename U>
using enable_if_mask_converts_implicitly =
    enable_if<(!std::is_same<Mask, Traits::decay<U>>::value &&  // that'd be the copy ctor
               Traits::is_simd_mask<U>::value && !Traits::isSimdMaskArray<U>::value &&
               Traits::is_implicit_cast_allowed_mask<
                   Traits::entry_type_of<typename Traits::decay<U>::Vector>, T>::value)>;
/**\internal
 * Central definition of the type combinations that only convert explicitly.
 */
template <typename T, typename U>
using enable_if_mask_converts_explicitly = enable_if<(
    Traits::isSimdMaskArray<U>::value ||
    (Traits::is_simd_mask<U>::value &&
     !Traits::is_implicit_cast_allowed_mask<
         Traits::entry_type_of<typename Traits::decay<U>::Vector>, T>::value))>;

/**\internal
 * Tag type for overloading on the width (\VSize{T}) of a vector.
 */
template <typename T> using WidthT = std::integral_constant<std::size_t, sizeof(T)>;

// forward declaration of MaskBool in common/maskbool.h
template <std::size_t Bytes> class MaskBool;

// forward declaration of SubscriptOperation in common/subscript.h
template <typename T, typename IndexVector, typename Scale, bool>
class SubscriptOperation;

/**
 * \internal
 * Helper type to pass along the two arguments for a gather operation.
 *
 * \tparam IndexVector  Normally an integer SIMD vector, but an array or std::vector also
 *                      works (though often not as efficient).
 */
template <class T, class IndexVector, int Scale = 1>
struct GatherArguments {
    static_assert(std::is_same<T, remove_cvref_t<T>>::value && !std::is_pointer<T>::value,
                  "GatherArguments expects an cv unqualified non-ref/ptr type");
    const IndexVector indexes;
    const T *const address;
};
template <int Scale, class T, class I>
GatherArguments<T, I, Scale> make_gather(const T *m, const I &i)
{
    return {i, m};
}

/**
 * \internal
 * Helper type to pass along the two arguments for a scatter operation.
 *
 * \tparam IndexVector  Normally an integer SIMD vector, but an array or std::vector also
 *                      works (though often not as efficient).
 */
template <typename T, typename IndexVector> struct ScatterArguments
{
    const IndexVector indexes;
    T *const address;
};

/**\internal
 * Break the recursion of the function below.
 */
template <typename I, I Begin, I End, typename F>
Vc_INTRINSIC enable_if<(Begin >= End), void> unrolled_loop(F &&)
{
}

/**\internal
 * Force the code in the lambda \p f to be called with indexes starting from \p Begin up
 * to (excluding) \p End to be called without compare and jump instructions (i.e. an
 * unrolled loop).
 */
template <typename I, I Begin, I End, typename F>
Vc_INTRINSIC Vc_FLATTEN enable_if<(Begin < End), void> unrolled_loop(F &&f)
{
    f(Begin);
    unrolled_loop<I, Begin + 1, End>(f);
}

/**\internal
 * Small simplification of the unrolled_loop call for ranges from 0 to \p Size using
 * std::size_t as the index type.
 */
template <std::size_t Size, typename F> Vc_INTRINSIC void for_all_vector_entries(F &&f)
{
    unrolled_loop<std::size_t, 0u, Size>(std::forward<F>(f));
}

}  // namespace Common
}  // namespace Vc

#include "vector.h"
#include "mask.h"
#include "memoryfwd.h"

#endif // VC_COMMON_TYPES_H_

// vim: foldmethod=marker
