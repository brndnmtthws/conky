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

#ifndef VC_COMMON_SIMDARRAYHELPER_H_
#define VC_COMMON_SIMDARRAYHELPER_H_

#include "macros.h"

namespace Vc_VERSIONED_NAMESPACE
{
// private_init {{{
namespace
{
static constexpr struct private_init_t {} private_init = {};
}  // unnamed namespace
// }}}

namespace Common
{

/// \addtogroup SimdArray
/// @{

namespace Operations/*{{{*/
{
struct tag {};
#define Vc_DEFINE_OPERATION(name_)                                                       \
    struct name_ : public tag {                                                          \
        template <typename V, typename... Args>                                          \
        Vc_INTRINSIC void operator()(V &v, Args &&... args)                              \
        {                                                                                \
            v.name_(std::forward<Args>(args)...);                                        \
        }                                                                                \
    }
Vc_DEFINE_OPERATION(gather);
Vc_DEFINE_OPERATION(scatter);
Vc_DEFINE_OPERATION(load);
Vc_DEFINE_OPERATION(store);
Vc_DEFINE_OPERATION(setZero);
Vc_DEFINE_OPERATION(setZeroInverted);
Vc_DEFINE_OPERATION(assign);
#undef Vc_DEFINE_OPERATION
#define Vc_DEFINE_OPERATION(name_, code_)                                                \
    struct name_ : public tag {                                                          \
        template <typename V> Vc_INTRINSIC void operator()(V &v) { code_; }              \
    }
Vc_DEFINE_OPERATION(increment, ++(v));
Vc_DEFINE_OPERATION(decrement, --(v));
Vc_DEFINE_OPERATION(random, v = V::Random());
#undef Vc_DEFINE_OPERATION
#define Vc_DEFINE_OPERATION_FORWARD(name_)                                               \
    struct Forward_##name_ : public tag                                                  \
    {                                                                                    \
        template <typename... Args, typename = decltype(name_(std::declval<Args>()...))> \
        Vc_INTRINSIC void operator()(decltype(name_(std::declval<Args>()...)) &v,        \
                                     Args &&... args)                                    \
        {                                                                                \
            v = name_(std::forward<Args>(args)...);                                      \
        }                                                                                \
        template <typename... Args, typename = decltype(name_(std::declval<Args>()...))> \
        Vc_INTRINSIC void operator()(std::nullptr_t, Args && ... args)                   \
        {                                                                                \
            name_(std::forward<Args>(args)...);                                          \
        }                                                                                \
    }
Vc_DEFINE_OPERATION_FORWARD(abs);
Vc_DEFINE_OPERATION_FORWARD(asin);
Vc_DEFINE_OPERATION_FORWARD(atan);
Vc_DEFINE_OPERATION_FORWARD(atan2);
Vc_DEFINE_OPERATION_FORWARD(cos);
Vc_DEFINE_OPERATION_FORWARD(ceil);
Vc_DEFINE_OPERATION_FORWARD(copysign);
Vc_DEFINE_OPERATION_FORWARD(exp);
Vc_DEFINE_OPERATION_FORWARD(exponent);
Vc_DEFINE_OPERATION_FORWARD(fma);
Vc_DEFINE_OPERATION_FORWARD(floor);
Vc_DEFINE_OPERATION_FORWARD(frexp);
Vc_DEFINE_OPERATION_FORWARD(isfinite);
Vc_DEFINE_OPERATION_FORWARD(isinf);
Vc_DEFINE_OPERATION_FORWARD(isnan);
Vc_DEFINE_OPERATION_FORWARD(isnegative);
Vc_DEFINE_OPERATION_FORWARD(ldexp);
Vc_DEFINE_OPERATION_FORWARD(log);
Vc_DEFINE_OPERATION_FORWARD(log10);
Vc_DEFINE_OPERATION_FORWARD(log2);
Vc_DEFINE_OPERATION_FORWARD(reciprocal);
Vc_DEFINE_OPERATION_FORWARD(round);
Vc_DEFINE_OPERATION_FORWARD(rsqrt);
Vc_DEFINE_OPERATION_FORWARD(sin);
Vc_DEFINE_OPERATION_FORWARD(sincos);
Vc_DEFINE_OPERATION_FORWARD(sqrt);
Vc_DEFINE_OPERATION_FORWARD(trunc);
Vc_DEFINE_OPERATION_FORWARD(min);
Vc_DEFINE_OPERATION_FORWARD(max);
#undef Vc_DEFINE_OPERATION_FORWARD
template<typename T> using is_operation = std::is_base_of<tag, T>;
}  // namespace Operations }}}

/**
 * \internal
 * Helper type to statically communicate segmentation of one vector register into 2^n parts
 * (Pieces).
 *
 * Forward declaration in common/types.h.
 */
template <typename T_, std::size_t Pieces_, std::size_t Index_> struct Segment/*{{{*/
{
    static_assert(Index_ < Pieces_, "You found a bug in Vc. Please report.");

    using type = T_;
    using type_decayed = typename std::decay<type>::type;
    static constexpr std::size_t Pieces = Pieces_;
    static constexpr std::size_t Index = Index_;
    using fixed_size_type =
        fixed_size_simd<conditional_t<Traits::is_simd_vector<type_decayed>::value,
                                      typename type_decayed::EntryType, float>,
                        type_decayed::Size / Pieces>;

    type data;

    static constexpr std::size_t EntryOffset = Index * type_decayed::Size / Pieces;

    // no non-const operator[] needed
    decltype(std::declval<const type &>()[0]) operator[](size_t i) const { return data[i + EntryOffset]; }

    fixed_size_type to_fixed_size() const
    {
        return simd_cast<fixed_size_type, Index>(data);
    }
};/*}}}*/

//Segment<T *, ...> specialization {{{
template <typename T_, std::size_t Pieces_, std::size_t Index_>
struct Segment<T_ *, Pieces_, Index_> {
    static_assert(Index_ < Pieces_, "You found a bug in Vc. Please report.");

    using type = T_ *;
    using type_decayed = typename std::decay<T_>::type;
    static constexpr size_t Pieces = Pieces_;
    static constexpr size_t Index = Index_;
    using fixed_size_type = fixed_size_simd<
        typename std::conditional<Traits::is_simd_vector<type_decayed>::value,
                                  typename type_decayed::VectorEntryType, float>::type,
        type_decayed::Size / Pieces> *;

    type data;

    static constexpr std::size_t EntryOffset = Index * type_decayed::size() / Pieces;

    fixed_size_type to_fixed_size() const
    {
        return reinterpret_cast<
#ifdef Vc_GCC
                   // GCC might ICE if this type is declared with may_alias. If it doesn't
                   // ICE it warns about ignoring the attribute.
                   typename std::remove_pointer<fixed_size_type>::type
#else
                   MayAlias<typename std::remove_pointer<fixed_size_type>::type>
#endif
                       *>(data) +
               Index;
    }

    //decltype(std::declval<type>()[0]) operator[](size_t i) { return data[i + EntryOffset]; }
    //decltype(std::declval<type>()[0]) operator[](size_t i) const { return data[i + EntryOffset]; }
};/*}}}*/

/** \internal
  Template class that is used to attach an offset value to an existing type. It is used
  for IndexesFromZero construction in SimdArray. The \c data1 constructor needs to know
  that the IndexesFromZero constructor requires an offset so that the whole data is
  constructed as a correct sequence from `0` to `Size - 1`.

  \tparam T The original type that needs the offset attached.
  \tparam Offset An integral value that determines the offset in the complete SimdArray.
 */
template <typename T, std::size_t Offset> struct AddOffset
{
    constexpr AddOffset() = default;
};

// class Split {{{1
/** \internal
  Helper type with static functions to generically adjust arguments for the \c data0 and
  \c data1 members of SimdArray and SimdMaskArray.

  \tparam secondOffset The offset in number of elements that \c data1 has in the SimdArray
                       / SimdMaskArray. This is essentially equal to the number of
                       elements in \c data0.
 */
template <std::size_t secondOffset> class Split
{
    // split composite SimdArray
    template <typename U, std::size_t N, typename V, std::size_t M,
              typename = enable_if<N != M>>
    static Vc_INTRINSIC auto loImpl(const SimdArray<U, N, V, M> &x)
        -> decltype(internal_data0(x))
    {
        return internal_data0(x);
    }
    template <typename U, std::size_t N, typename V, std::size_t M,
              typename = enable_if<N != M>>
    static Vc_INTRINSIC auto hiImpl(const SimdArray<U, N, V, M> &x)
        -> decltype(internal_data1(x))
    {
        return internal_data1(x);
    }
    template <typename U, std::size_t N, typename V, std::size_t M,
              typename = enable_if<N != M>>
    static Vc_INTRINSIC auto loImpl(SimdArray<U, N, V, M> *x)
        -> decltype(&internal_data0(*x))
    {
        return &internal_data0(*x);
    }
    template <typename U, std::size_t N, typename V, std::size_t M,
              typename = enable_if<N != M>>
    static Vc_INTRINSIC auto hiImpl(SimdArray<U, N, V, M> *x)
        -> decltype(&internal_data1(*x))
    {
        return &internal_data1(*x);
    }

    // split atomic SimdArray
    template <typename U, std::size_t N, typename V>
    static Vc_INTRINSIC Segment<V, 2, 0> loImpl(const SimdArray<U, N, V, N> &x)
    {
        return {internal_data(x)};
    }
    template <typename U, std::size_t N, typename V>
    static Vc_INTRINSIC Segment<V, 2, 1> hiImpl(const SimdArray<U, N, V, N> &x)
    {
        return {internal_data(x)};
    }
    template <typename U, std::size_t N, typename V>
    static Vc_INTRINSIC Segment<V *, 2, 0> loImpl(SimdArray<U, N, V, N> *x)
    {
        return {&internal_data(*x)};
    }
    template <typename U, std::size_t N, typename V>
    static Vc_INTRINSIC Segment<V *, 2, 1> hiImpl(SimdArray<U, N, V, N> *x)
    {
        return {&internal_data(*x)};
    }

    // split composite SimdMaskArray
    template <typename U, std::size_t N, typename V, std::size_t M>
    static Vc_INTRINSIC auto loImpl(const SimdMaskArray<U, N, V, M> &x) -> decltype(internal_data0(x))
    {
        return internal_data0(x);
    }
    template <typename U, std::size_t N, typename V, std::size_t M>
    static Vc_INTRINSIC auto hiImpl(const SimdMaskArray<U, N, V, M> &x) -> decltype(internal_data1(x))
    {
        return internal_data1(x);
    }

    template <typename U, std::size_t N, typename V>
    static Vc_INTRINSIC Segment<typename SimdMaskArray<U, N, V, N>::mask_type, 2, 0> loImpl(
        const SimdMaskArray<U, N, V, N> &x)
    {
        return {internal_data(x)};
    }
    template <typename U, std::size_t N, typename V>
    static Vc_INTRINSIC Segment<typename SimdMaskArray<U, N, V, N>::mask_type, 2, 1> hiImpl(
        const SimdMaskArray<U, N, V, N> &x)
    {
        return {internal_data(x)};
    }

    // split Vector<T> and Mask<T>
#ifdef Vc_IMPL_AVX
    template <class T>
    static Vc_INTRINSIC SSE::Vector<T> loImpl(Vector<T, VectorAbi::Avx> &&x)
    {
        return simd_cast<SSE::Vector<T>, 0>(x);
    }
    template <class T>
    static Vc_INTRINSIC SSE::Vector<T> hiImpl(Vector<T, VectorAbi::Avx> &&x)
    {
        return simd_cast<SSE::Vector<T>, 1>(x);
    }
    template <class T>
    static Vc_INTRINSIC SSE::Mask<T> loImpl(Mask<T, VectorAbi::Avx> &&x)
    {
        return simd_cast<SSE::Mask<T>, 0>(x);
    }
    template <class T>
    static Vc_INTRINSIC SSE::Mask<T> hiImpl(Mask<T, VectorAbi::Avx> &&x)
    {
        return simd_cast<SSE::Mask<T>, 1>(x);
    }
#endif  // Vc_IMPL_AVX
    template <typename T>
    static constexpr bool is_vector_or_mask(){
        return (Traits::is_simd_vector<T>::value && !Traits::isSimdArray<T>::value) ||
               (Traits::is_simd_mask<T>::value && !Traits::isSimdMaskArray<T>::value);
    }
    template <typename V>
    static Vc_INTRINSIC Segment<V, 2, 0> loImpl(V &&x, enable_if<is_vector_or_mask<V>()> = nullarg)
    {
        return {std::forward<V>(x)};
    }
    template <typename V>
    static Vc_INTRINSIC Segment<V, 2, 1> hiImpl(V &&x, enable_if<is_vector_or_mask<V>()> = nullarg)
    {
        return {std::forward<V>(x)};
    }

    // split std::vector<T>
    template <class T, class A>
    static Vc_INTRINSIC const T *loImpl(const std::vector<T, A> &x)
    {
        return x.data();
    }
    template <class T, class A>
    static Vc_INTRINSIC const T *hiImpl(const std::vector<T, A> &x)
    {
        return x.data() + secondOffset;
    }

    // generically split Segments
    template <typename V, std::size_t Pieces, std::size_t Index>
    static Vc_INTRINSIC Segment<V, 2 * Pieces, 2 * Index> loImpl(
        const Segment<V, Pieces, Index> &x)
    {
        return {x.data};
    }
    template <typename V, std::size_t Pieces, std::size_t Index>
    static Vc_INTRINSIC Segment<V, 2 * Pieces, 2 * Index + 1> hiImpl(
        const Segment<V, Pieces, Index> &x)
    {
        return {x.data};
    }

    /** \internal
     * \name Checks for existence of \c loImpl / \c hiImpl
     */
    //@{
    template <typename T, typename = decltype(loImpl(std::declval<T>()))>
    static std::true_type have_lo_impl(int);
    template <typename T> static std::false_type have_lo_impl(float);
    template <typename T> static constexpr bool have_lo_impl()
    {
        return decltype(have_lo_impl<T>(1))::value;
    }

    template <typename T, typename = decltype(hiImpl(std::declval<T>()))>
    static std::true_type have_hi_impl(int);
    template <typename T> static std::false_type have_hi_impl(float);
    template <typename T> static constexpr bool have_hi_impl()
    {
        return decltype(have_hi_impl<T>(1))::value;
    }
    //@}

public:
    /** \internal
     * \name with Operations tag
     *
     * These functions don't overload on the data parameter. The first parameter (the tag) clearly
     * identifies the intended function.
     */
    //@{
    template <typename U>
    static Vc_INTRINSIC const U *lo(Operations::gather, const U *ptr)
    {
        return ptr;
    }
    template <typename U>
    static Vc_INTRINSIC const U *hi(Operations::gather, const U *ptr)
    {
        return ptr + secondOffset;
    }
    template <typename U, typename = enable_if<!std::is_pointer<U>::value>>
    static Vc_ALWAYS_INLINE decltype(loImpl(std::declval<U>()))
        lo(Operations::gather, U &&x)
    {
        return loImpl(std::forward<U>(x));
    }
    template <typename U, typename = enable_if<!std::is_pointer<U>::value>>
    static Vc_ALWAYS_INLINE decltype(hiImpl(std::declval<U>()))
        hi(Operations::gather, U &&x)
    {
        return hiImpl(std::forward<U>(x));
    }
    template <typename U>
    static Vc_INTRINSIC const U *lo(Operations::scatter, const U *ptr)
    {
        return ptr;
    }
    template <typename U>
    static Vc_INTRINSIC const U *hi(Operations::scatter, const U *ptr)
    {
        return ptr + secondOffset;
    }
    //@}

    /** \internal
      \name without Operations tag

      These functions are not clearly tagged as to where they are used and therefore
      behave differently depending on the type of the parameter. Different behavior is
      implemented via overloads of \c loImpl and \c hiImpl. They are not overloads of \c
      lo and \c hi directly because it's hard to compete against a universal reference
      (i.e. an overload for `int` requires overloads for `int &`, `const int &`, and `int
      &&`. If one of them were missing `U &&` would win in overload resolution).
     */
    //@{
    template <typename U>
    static Vc_ALWAYS_INLINE decltype(loImpl(std::declval<U>())) lo(U &&x)
    {
        return loImpl(std::forward<U>(x));
    }
    template <typename U>
    static Vc_ALWAYS_INLINE decltype(hiImpl(std::declval<U>())) hi(U &&x)
    {
        return hiImpl(std::forward<U>(x));
    }

    template <typename U>
    static Vc_ALWAYS_INLINE enable_if<!have_lo_impl<U>(), U> lo(U &&x)
    {
        return std::forward<U>(x);
    }
    template <typename U>
    static Vc_ALWAYS_INLINE enable_if<!have_hi_impl<U>(), U> hi(U &&x)
    {
        return std::forward<U>(x);
    }
    //@}
};

// actual_value {{{1
template <typename Op, typename U, std::size_t M, typename V>
static Vc_INTRINSIC const V &actual_value(Op, const SimdArray<U, M, V, M> &x)
{
  return internal_data(x);
}
template <typename Op, typename U, std::size_t M, typename V>
static Vc_INTRINSIC V *actual_value(Op, SimdArray<U, M, V, M> *x)
{
  return &internal_data(*x);
}
template <typename Op, typename T, size_t Pieces, size_t Index>
static Vc_INTRINSIC typename Segment<T, Pieces, Index>::fixed_size_type actual_value(
    Op, Segment<T, Pieces, Index> &&seg)
{
    return seg.to_fixed_size();
}

template <typename Op, typename U, std::size_t M, typename V>
static Vc_INTRINSIC const typename V::Mask &actual_value(Op, const SimdMaskArray<U, M, V, M> &x)
{
  return internal_data(x);
}
template <typename Op, typename U, std::size_t M, typename V>
static Vc_INTRINSIC typename V::Mask *actual_value(Op, SimdMaskArray<U, M, V, M> *x)
{
  return &internal_data(*x);
}

// unpackArgumentsAuto {{{1
/**\internal
 * \name unpackArgumentsAuto
 *
 * Search for the right amount of SimdArray "unpacking" (via actual_value) to match the
 * interface of the function to be called.
 *
 * The compiler can figure this out for us thanks to SFINAE. The approach is to have a
 * number \c I that determines the indexes of the arguments to be transformed via
 * actual_value.  Each bit of \c I identifies an argument. unpackArgumentsAuto starts the
 * recursion with `I = 0`, i.e. no actual_value transformations. If the overload calling
 * \c op is unavailable due to a substitution failure \c I is incremented and the function
 * recurses. Otherwise there are two unpackArgumentsAutoImpl functions in the overload
 * set. The first argument (\c int / \c float) leads to a preference of the function
 * calling \c op, thus ending the recursion.
 */
///@{

///\internal transforms \p arg via actual_value
template <typename Op, typename Arg>
Vc_INTRINSIC decltype(actual_value(std::declval<Op &>(), std::declval<Arg>()))
conditionalUnpack(std::true_type, Op op, Arg &&arg)
{
    return actual_value(op, std::forward<Arg>(arg));
}
///\internal forwards \p arg to its return value
template <typename Op, typename Arg>
Vc_INTRINSIC Arg conditionalUnpack(std::false_type, Op, Arg &&arg)
{
    return std::forward<Arg>(arg);
}

///\internal true-/false_type that selects whether the argument with index B should be unpacked
template <size_t A, size_t B>
struct selectorType : public std::integral_constant<bool, !((A & (size_t(1) << B)) != 0)> {
};

///\internal ends the recursion, transforms arguments, and calls \p op
template <size_t I, typename Op, typename R, typename... Args, size_t... Indexes>
Vc_INTRINSIC decltype(std::declval<Op &>()(std::declval<R &>(),
                                           conditionalUnpack(selectorType<I, Indexes>(),
                                                             std::declval<Op &>(),
                                                             std::declval<Args>())...))
unpackArgumentsAutoImpl(int, index_sequence<Indexes...>, Op op, R &&r, Args &&... args)
{
    op(std::forward<R>(r),
       conditionalUnpack(selectorType<I, Indexes>(), op, std::forward<Args>(args))...);
}

///\internal the current actual_value calls don't work: recurse to I + 1
template <size_t I, typename Op, typename R, typename... Args, size_t... Indexes>
Vc_INTRINSIC enable_if<(I <= (size_t(1) << sizeof...(Args))), void> unpackArgumentsAutoImpl(
    float, index_sequence<Indexes...> is, Op op, R &&r, Args &&... args)
{
    // if R is nullptr_t then the return type cannot enforce that actually any unwrapping
    // of the SimdArray types happens. Thus, you could get an endless loop of the
    // SimdArray function overload calling itself, if the index goes up to (1 <<
    // sizeof...(Args)) - 1 (which means no argument transformations via actual_value).
    static_assert(
        I < (1 << sizeof...(Args)) - (std::is_same<R, std::nullptr_t>::value ? 1 : 0),
        "Vc or compiler bug. Please report. Failed to find a combination of "
        "actual_value(arg) transformations that allows calling Op.");
    unpackArgumentsAutoImpl<I + 1, Op, R, Args...>(int(), is, op, std::forward<R>(r),
                                                   std::forward<Args>(args)...);
}

#ifdef Vc_ICC
template <size_t, typename... Ts> struct IccWorkaround {
    using type = void;
};
template <typename... Ts> struct IccWorkaround<2, Ts...> {
    using type = typename std::remove_pointer<typename std::decay<
        typename std::tuple_element<1, std::tuple<Ts...>>::type>::type>::type;
};
#endif

///\internal The interface to start the machinery.
template <typename Op, typename R, typename... Args>
Vc_INTRINSIC void unpackArgumentsAuto(Op op, R &&r, Args &&... args)
{
#ifdef Vc_ICC
    // ugly hacky workaround for ICC:
    // The compiler fails to do SFINAE right on recursion. We have to hit the right
    // recursionStart number from the start.
    const int recursionStart =
        Traits::isSimdArray<
            typename IccWorkaround<sizeof...(Args), Args...>::type>::value &&
                (std::is_same<Op, Common::Operations::Forward_frexp>::value ||
                 std::is_same<Op, Common::Operations::Forward_ldexp>::value)
            ? 2
            : 0;
#else
    const int recursionStart = 0;
#endif
    unpackArgumentsAutoImpl<recursionStart>(
        int(), make_index_sequence<sizeof...(Args)>(), op, std::forward<R>(r),
        std::forward<Args>(args)...);
}
///@}

//}}}1
///@}
}  // namespace Common
}  // namespace Vc

#endif  // VC_COMMON_SIMDARRAYHELPER_H_

// vim: foldmethod=marker
