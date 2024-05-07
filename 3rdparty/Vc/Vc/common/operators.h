/*  This file is part of the Vc library. {{{
Copyright © 2012-2016 Matthias Kretz <kretz@kde.org>

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

#ifndef COMMON_OPERATORS_H_
#define COMMON_OPERATORS_H_
#include "simdarray.h"
#include "macros.h"

namespace Vc_VERSIONED_NAMESPACE
{
namespace Detail
{
template <typename T, typename Abi, typename U>
enable_if<!std::is_same<T, U>::value, U> is_convertible_to_any_vector(Vector<U, Abi>);
template <typename T, typename Abi> T is_convertible_to_any_vector(Vector<T, Abi>);

template <typename T, typename U, bool = std::is_integral<T>::value,
          bool = std::is_integral<U>::value>
struct FundamentalReturnType;
template <class T, class U>
using fundamental_return_t = typename FundamentalReturnType<T, U>::type;

template <typename T, typename U> struct FundamentalReturnType<T, U, false, false> {
    using type = typename std::conditional<
        std::is_arithmetic<U>::value,
        typename std::conditional<(sizeof(T) < sizeof(U)), U, T>::type,
        // U is not arithmetic, e.g. an enum or a type with e.g. operator int()
        T>::type;
};
template <typename T, typename U> struct FundamentalReturnType<T, U, true, false> {
    using type = typename std::conditional<
        std::is_arithmetic<U>::value, U,
        // U is not arithmetic, e.g. an enum or a type with e.g. operator int()
        T>::type;
};
template <typename T, typename U> struct FundamentalReturnType<T, U, false, true> {
    using type = T;
};

template <typename T> struct my_make_signed : public std::make_signed<T> {
};
template <> struct my_make_signed<bool> {
    using type = bool;
};

template <typename TT, typename UU>
struct higher_conversion_rank {
    template <typename A>
    using fix_sign =
        typename std::conditional<(std::is_unsigned<TT>::value ||
                                   std::is_unsigned<UU>::value),
                                  typename std::make_unsigned<A>::type, A>::type;
    using T = typename my_make_signed<TT>::type;
    using U = typename my_make_signed<UU>::type;
    template <typename Test, typename Otherwise>
    using c = typename std::conditional<std::is_same<T, Test>::value ||
                                            std::is_same<U, Test>::value,
                                        Test, Otherwise>::type;

    using type = fix_sign<c<long long, c<long, c<int, c<short, c<signed char, void>>>>>>;
};

template <typename T, typename U> struct FundamentalReturnType<T, U, true, true> {
    template <bool B, class Then, class E>
    using c = typename std::conditional<B, Then, E>::type;
    using type =
        c<(sizeof(T) > sizeof(U)), T,
          c<(sizeof(T) < sizeof(U)), U, typename higher_conversion_rank<T, U>::type>>;
};

template <class V, class T, class Tq, class = void> struct ReturnTypeImpl {
    // no type => SFINAE
};
// 1. Vector × Vector
template <class T, class U, class Abi, class Uq>
struct ReturnTypeImpl<Vector<T, Abi>, Vector<U, Abi>, Uq, void> {
    using type = Vc::Vector<fundamental_return_t<T, U>, Abi>;
};
// 2. Vector × int
template <class T, class Abi, class Uq>
struct ReturnTypeImpl<Vector<T, Abi>, int, Uq, void> {
    // conversion from int is always allowed (because its the default when you hardcode a
    // number)
    using type = Vc::Vector<T, Abi>;
};
// 3. Vector × unsigned
template <class T, class Abi, class Uq>
struct ReturnTypeImpl<Vector<T, Abi>, uint, Uq, void> {
    // conversion from unsigned int is allowed for all integral Vector<T>, but ensures
    // unsigned result
    using type = Vc::Vector<
        typename std::conditional<std::is_integral<T>::value, std::make_unsigned<T>,
                                  std::enable_if<true, T>>::type::type,
        Abi>;
};
// 4. Vector × {enum, arithmetic}
template <class T, class U, class Abi, class Uq>
struct ReturnTypeImpl<
    Vector<T, Abi>, U, Uq,
    enable_if<!std::is_class<U>::value && !std::is_same<U, int>::value &&
                  !std::is_same<U, uint>::value &&
                  Traits::is_valid_vector_argument<fundamental_return_t<T, U>>::value,
              void>> {
    using type = Vc::Vector<fundamental_return_t<T, U>, Abi>;
};
// 5. Vector × UDT
template <class T, class U, class Abi, class Uq>
struct ReturnTypeImpl<
    Vector<T, Abi>, U, Uq,
    enable_if<std::is_class<U>::value && !Traits::is_simd_vector<U>::value &&
                  Traits::is_valid_vector_argument<decltype(
                      is_convertible_to_any_vector<T, Abi>(std::declval<Uq>()))>::value,
              void>> {
    using type =
        Vc::Vector<fundamental_return_t<T, decltype(is_convertible_to_any_vector<T, Abi>(
                                               std::declval<Uq>()))>,
                   Abi>;
};
template <class V, class Tq, class T = remove_cvref_t<Tq>>
using ReturnType = typename ReturnTypeImpl<V, T, Tq>::type;

template <class T> struct is_a_type : public std::true_type {
};

#ifdef Vc_ENABLE_FLOAT_BIT_OPERATORS
#define Vc_TEST_FOR_BUILTIN_OPERATOR(op_) true
#else
#define Vc_TEST_FOR_BUILTIN_OPERATOR(op_)                                                \
    Detail::is_a_type<decltype(std::declval<typename R::value_type>()                    \
                                   op_ std::declval<typename R::value_type>())>::value
#endif
}  // namespace Detail

#define Vc_GENERIC_OPERATOR(op_)                                                         \
    template <class T, class Abi, class U,                                               \
              class R = Detail::ReturnType<Vector<T, Abi>, U>>                           \
    Vc_ALWAYS_INLINE enable_if<Vc_TEST_FOR_BUILTIN_OPERATOR(op_) &&                      \
                                   std::is_convertible<Vector<T, Abi>, R>::value &&      \
                                   std::is_convertible<U, R>::value,                     \
                               R>                                                        \
    operator op_(Vector<T, Abi> x, U &&y)                                                \
    {                                                                                    \
        return Detail::operator op_(R(x), R(std::forward<U>(y)));                        \
    }                                                                                    \
    template <class T, class Abi, class U,                                               \
              class R = Detail::ReturnType<Vector<T, Abi>, U>>                           \
    Vc_ALWAYS_INLINE enable_if<Vc_TEST_FOR_BUILTIN_OPERATOR(op_) &&                      \
                                   !Traits::is_simd_vector<U>::value &&                  \
                                   std::is_convertible<Vector<T, Abi>, R>::value &&      \
                                   std::is_convertible<U, R>::value,                     \
                               R>                                                        \
    operator op_(U &&x, Vector<T, Abi> y)                                                \
    {                                                                                    \
        return Detail::operator op_(R(std::forward<U>(x)), R(y));                        \
    }                                                                                    \
    template <class T, class Abi, class U,                                               \
              class R = Detail::ReturnType<Vector<T, Abi>, U>>                           \
    Vc_ALWAYS_INLINE enable_if<Vc_TEST_FOR_BUILTIN_OPERATOR(op_) &&                      \
                                   std::is_convertible<Vector<T, Abi>, R>::value &&      \
                                   std::is_convertible<U, R>::value,                     \
                               Vector<T, Abi> &>                                         \
    operator op_##=(Vector<T, Abi> &x, U &&y)                                            \
    {                                                                                    \
        x = Detail::operator op_(R(x), R(std::forward<U>(y)));                           \
        return x;                                                                        \
    }

#define Vc_LOGICAL_OPERATOR(op_)                                                         \
    template <class T, class Abi>                                                        \
    Vc_ALWAYS_INLINE typename Vector<T, Abi>::Mask operator op_(Vector<T, Abi> x,        \
                                                                Vector<T, Abi> y)        \
    {                                                                                    \
        return !!x op_ !!y;                                                              \
    }                                                                                    \
    template <class T, class Abi, class U>                                               \
    Vc_ALWAYS_INLINE                                                                     \
        enable_if<std::is_convertible<Vector<T, Abi>, Vector<U, Abi>>::value &&          \
                      std::is_convertible<Vector<U, Abi>, Vector<T, Abi>>::value,        \
                  typename Detail::ReturnType<Vector<T, Abi>, Vector<U, Abi>>::Mask>     \
        operator op_(Vector<T, Abi> x, Vector<U, Abi> y)                                 \
    {                                                                                    \
        return !!x op_ !!y;                                                              \
    }                                                                                    \
    template <class T, class Abi, class U>                                               \
    Vc_ALWAYS_INLINE enable_if<std::is_same<bool, decltype(!std::declval<U>())>::value,  \
                               typename Vector<T, Abi>::Mask>                            \
    operator op_(Vector<T, Abi> x, U &&y)                                                \
    {                                                                                    \
        using M = typename Vector<T, Abi>::Mask;                                         \
        return !!x op_ M(!!std::forward<U>(y));                                          \
    }                                                                                    \
    template <class T, class Abi, class U>                                               \
    Vc_ALWAYS_INLINE enable_if<std::is_same<bool, decltype(!std::declval<U>())>::value,  \
                               typename Vector<T, Abi>::Mask>                            \
    operator op_(U &&x, Vector<T, Abi> y)                                                \
    {                                                                                    \
        using M = typename Vector<T, Abi>::Mask;                                         \
        return M(!!std::forward<U>(x)) op_ !!y;                                          \
    }

#define Vc_COMPARE_OPERATOR(op_)                                                         \
    template <class T, class Abi, class U,                                               \
              class R = Detail::ReturnType<Vector<T, Abi>, U>>                           \
    Vc_ALWAYS_INLINE enable_if<std::is_convertible<Vector<T, Abi>, R>::value &&          \
                                   std::is_convertible<U, R>::value,                     \
                               typename R::Mask>                                         \
    operator op_(Vector<T, Abi> x, U &&y)                                                \
    {                                                                                    \
        return Detail::operator op_(R(x), R(std::forward<U>(y)));                        \
    }                                                                                    \
    template <class T, class Abi, class U,                                               \
              class R = Detail::ReturnType<Vector<T, Abi>, U>>                           \
    Vc_ALWAYS_INLINE                                                                     \
        enable_if<!Traits::is_simd_vector_internal<remove_cvref_t<U>>::value &&          \
                      std::is_convertible<Vector<T, Abi>, R>::value &&                   \
                      std::is_convertible<U, R>::value,                                  \
                  typename R::Mask>                                                      \
        operator op_(U &&x, Vector<T, Abi> y)                                            \
    {                                                                                    \
        return Detail::operator op_(R(std::forward<U>(x)), R(y));                        \
    }

Vc_ALL_LOGICAL    (Vc_LOGICAL_OPERATOR);
Vc_ALL_BINARY     (Vc_GENERIC_OPERATOR);
Vc_ALL_ARITHMETICS(Vc_GENERIC_OPERATOR);
Vc_ALL_COMPARES   (Vc_COMPARE_OPERATOR);

#undef Vc_LOGICAL_OPERATOR
#undef Vc_GENERIC_OPERATOR
#undef Vc_COMPARE_OPERATOR
#undef Vc_INVALID_OPERATOR

}  // namespace Vc
#endif  // COMMON_OPERATORS_H_
