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

#ifndef VC_TRAITS_TYPE_TRAITS_H_
#define VC_TRAITS_TYPE_TRAITS_H_

#include <type_traits>
#include "decay.h"
#include "has_no_allocated_data.h"
#include "has_contiguous_storage.h"
#include "is_functor_argument_immutable.h"
#include "is_output_iterator.h"
#include "is_index_sequence.h"
#include "is_implicit_cast_allowed.h"

namespace Vc_VERSIONED_NAMESPACE
{
// meta-programming helpers
struct enable_if_default_type
{
    constexpr enable_if_default_type() {}
};
static constexpr enable_if_default_type nullarg;
template <bool Test, typename T = enable_if_default_type> using enable_if = typename std::enable_if<Test, T>::type;

template <bool B, class T, class F>
using conditional_t = typename std::conditional<B, T, F>::type;

template <class T>
using remove_cvref_t =
    typename std::remove_cv<typename std::remove_reference<T>::type>::type;

namespace Traits
{
#include "has_subscript_operator.h"
#include "has_multiply_operator.h"
#include "has_addition_operator.h"
#include "has_equality_operator.h"

template<typename T> struct is_valid_vector_argument : public std::false_type {};

template <> struct is_valid_vector_argument<double> : public std::true_type {};
template <> struct is_valid_vector_argument<float>  : public std::true_type {};
template <> struct is_valid_vector_argument<int>    : public std::true_type {};
template <> struct is_valid_vector_argument<unsigned int>   : public std::true_type {};
template <> struct is_valid_vector_argument<short>  : public std::true_type {};
template <> struct is_valid_vector_argument<unsigned short> : public std::true_type {};

template<typename T> struct is_simd_mask_internal : public std::false_type {};
template<typename T> struct is_simd_vector_internal : public std::false_type {};
template<typename T> struct is_simdarray_internal : public std::false_type {};
template<typename T> struct is_simd_mask_array_internal : public std::false_type {};
template<typename T> struct is_loadstoreflag_internal : public std::false_type {};

template <typename T, bool = is_simd_vector_internal<T>::value> struct is_integral_internal;
template <typename T, bool = is_simd_vector_internal<T>::value> struct is_floating_point_internal;
template <typename T, bool = is_simd_vector_internal<T>::value> struct is_signed_internal;
template <typename T, bool = is_simd_vector_internal<T>::value> struct is_unsigned_internal;

template <typename T> struct is_integral_internal      <T, false> : public std::is_integral      <T> {};
template <typename T> struct is_floating_point_internal<T, false> : public std::is_floating_point<T> {};
template <typename T> struct is_signed_internal        <T, false> : public std::is_signed        <T> {};
template <typename T> struct is_unsigned_internal      <T, false> : public std::is_unsigned      <T> {};

template <typename V> struct is_integral_internal      <V, true> : public std::is_integral      <typename V::EntryType> {};
template <typename V> struct is_floating_point_internal<V, true> : public std::is_floating_point<typename V::EntryType> {};
template <typename V> struct is_signed_internal        <V, true> : public std::is_signed        <typename V::EntryType> {};
template <typename V> struct is_unsigned_internal      <V, true> : public std::is_unsigned      <typename V::EntryType> {};

template <typename T>
struct is_arithmetic_internal
    : public std::integral_constant<
          bool,
          (is_floating_point_internal<T>::value || is_integral_internal<T>::value)>
{
};

template <class T, class = void>
struct vector_size_internal : std::integral_constant<std::size_t, 0> {
};
template <class T>
struct vector_size_internal<T, decltype((void)(T::size() > 0))>
    : std::integral_constant<std::size_t, T::size()> {
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Identifies any SIMD mask type (independent of implementation or whether it's
 * SimdMaskArray<T, N>).
 */
template <typename T>
struct is_simd_mask : public std::integral_constant<bool,
                                                    (is_simd_mask_internal<decay<T>>::value ||
                                                     is_simd_mask_array_internal<decay<T>>::value)>
{
};

/**
 * Identifies any SIMD vector type (independent of implementation or whether it's
 * SimdArray<T, N>).
 */
template <typename T>
struct is_simd_vector
    : public std::integral_constant<bool,
                                    (is_simd_vector_internal<decay<T>>::value ||
                                     is_simdarray_internal<decay<T>>::value)>
{
};

/// Identifies any possible SimdArray<T, N> type (independent of const/volatile or reference)
template <typename T>
struct isSimdArray : public is_simdarray_internal<decay<T>>
{
};

/// Identifies any possible SimdMaskArray<T, N> type (independent of const/volatile or reference)
template <typename T>
struct isSimdMaskArray : public is_simd_mask_array_internal<decay<T>>
{
};

/// \internal Identifies LoadStoreFlag types
template <typename T> struct is_load_store_flag : public is_loadstoreflag_internal<decay<T>> {};

/// \internal Identifies a SimdArray type with a single Vector member
template <typename T> struct is_atomic_simdarray_internal : public std::false_type {};
template <typename T> using isAtomicSimdArray = is_atomic_simdarray_internal<decay<T>>;

/// \internal Identifies a SimdMaskArray type with a single Mask member
template <typename T> struct is_atomic_simd_mask_array_internal : public std::false_type {};
template <typename T> using isAtomicSimdMaskArray = is_atomic_simd_mask_array_internal<decay<T>>;

/**
 * The \p value member will either be the number of SIMD vector entries or 0 if \p T is not a SIMD
 * type.
 */
template <typename T> struct simd_vector_size : public vector_size_internal<decay<T>> {};

template <typename T> struct is_integral : public is_integral_internal<decay<T>> {};
template <typename T> struct is_floating_point : public is_floating_point_internal<decay<T>> {};
template <typename T> struct is_arithmetic : public is_arithmetic_internal<decay<T>> {};
template <typename T> struct is_signed : public is_signed_internal<decay<T>> {};
template <typename T> struct is_unsigned : public is_unsigned_internal<decay<T>> {};

template <typename T, bool IsSimdVector> struct scalar_type_internal { using type = T; };
template <typename T> struct scalar_type_internal<T, true> { using type = typename T::EntryType; };
template <typename T> using scalar_type = typename scalar_type_internal<decay<T>, is_simd_vector<T>::value>::type;

}  // namespace Traits
}  // namespace Vc

#include "entry_type_of.h"

#endif // VC_TRAITS_TYPE_TRAITS_H_
