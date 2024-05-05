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
DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

}}}*/

#ifndef VC_COMMON_SIMDIZE_H_
#define VC_COMMON_SIMDIZE_H_

#include <tuple>
#include <array>

#include "../Allocator"
#include "interleavedmemory.h"

/*!
\addtogroup Simdize

Automatic type vectorization.

Struct Vectorization
======================

The `Vc::simdize<T>` expression transforms the type \c T to a vectorized type. This requires the type
\c T to be a class template instance or an arithmetic type.

Example:
First, we declare a class template for a three-dimensional point. The template parameter \c T
determines the type of the members and is \c float in the scalar (classical) case.
\code
template <typename T> struct PointTemplate
{
  T x, y, z;

  // Declares tuple_size and makes the members accessible via get<N>(point), allowing
  // the simdize implementation to convert between Point and PointV (see below).
  Vc_SIMDIZE_INTERFACE((x, y, z));

  PointTemplate(T xx, T yy, T zz) : x{xx}, y{yy}, z{zz} {};

  // The following function will automatically be vectorized in the PointV type.
  T distance_to_origin() const {
    using std::sqrt;
    return sqrt(x * x + y * y + z * z);
  }
};
\endcode

In the following we create a type alias for the scalar type, which simply means instantiating
\c PointTemplate with \c float. The resulting type can then be transformed with \ref simdize.
\code
using Point  = PointTemplate<float>;  // A simple struct with three floats and two functions.
using PointV = Vc::simdize<Point>;    // The vectorization of Point stores three float_v and thus
                                      // float_v::size() Points.
\endcode

The following shows a code example using the above \c Point and \c PointV types.
\code
PointV pv = Point{0.f, 1.f, 2.f};  // Constructs a PointV containing PointV::size()
                                   // copies of Point{0, 1, 2}.
for (int i = 1; i < int(pv.size()); ++i) {
  assign(pv, i, {i + 0.f, i + 1.f, i + 2.f});
}

const Vc::float_v l = pv.distance_to_origin();
std::cout << l << '\n';
// prints [2.23607, 3.74166, 5.38516, 7.07107, 8.77496, 10.4881, 12.2066, 13.9284] with
// float_v::size() == 8

const Point most_distant = extract(pv, (l.max() == l).firstOne());
std::cout << '(' << most_distant.x << ", " << most_distant.y << ", " << most_distant.z << ")\n";
// prints (7, 8, 9) with float_v::size() == 8
\endcode

Iterator Vectorization
======================

`Vc::simdize<Iterator>` can also be used to turn an iterator type into a new iterator type with `Vc::simdize<Iterator::value_type>` as its `value_type`.
Note that `Vc::simdize<double>` turns into `Vc::Vector<double>`, which makes it easy to iterate over a given container of builtin arithmetics using `Vc::Vector`.
\code
void classic(const std::vector<Point> &data) {
  using It = std::vector<Point>::const_iterator;
  const It end = data.end();
  for (It it = data.begin(); it != end; ++it) {
    Point x = *it;
    do_something(x);
  }
}

void vectorized(const std::vector<float> &data) {
  using It = Vc::simdize<std::vector<Point>::const_iterator>;
  const It end = data.end();
  for (It it = data.begin(); it != end; ++it) {
    Vc::simdize<Point> x = *it;  // i.e. PointV
    do_something(x);
  }
}
\endcode

 */
namespace Vc_VERSIONED_NAMESPACE
{
/**\internal
 * \ingroup Simdize
 * This namespace contains all the required code for implementing simdize<T>. None of this
 * code should be directly accessed by users, though the unit test for simdize<T>
 * certainly may look into some of the details if necessary.
 */
namespace SimdizeDetail  // {{{
{
/**
 * \addtogroup Simdize
 * @{
 */
using std::is_same;
using std::is_base_of;
using std::false_type;
using std::true_type;
using std::iterator_traits;
using std::conditional;
using std::size_t;

/**\internal
 * Typelist is a simple helper class for supporting multiple parameter packs in one class
 * template.
 */
template <typename... Ts> struct Typelist;

/**\internal
 * The Category identifies how the type argument to simdize<T> has to be transformed.
 */
enum class Category {
    ///\internal No transformation
    NoTransformation,
    ///\internal simple Vector<T> transformation
    ArithmeticVectorizable,
    ///\internal transform an input iterator to return vectorized entries
    InputIterator,
    ///\internal transform a forward iterator to return vectorized entries
    OutputIterator,
    ///\internal transform an output iterator to return vectorized entries
    ForwardIterator,
    ///\internal transform a bidirectional iterator to return vectorized entries
    BidirectionalIterator,
    ///\internal transform a random access iterator to return vectorized entries
    RandomAccessIterator,
    ///\internal transform a class template recursively
    ClassTemplate
};

/**\internal
 * iteratorCategories<T>(int()) returns whether iterator_traits<T>::iterator_category is a
 * valid type and whether it is derived from RandomAccessIterator or ForwardIterator.
 */
template <typename T, typename ItCat = typename T::iterator_category>
constexpr Category iteratorCategories(int, ItCat * = nullptr)
{
    return is_base_of<std::random_access_iterator_tag, ItCat>::value
               ? Category::RandomAccessIterator
               : is_base_of<std::bidirectional_iterator_tag, ItCat>::value
                     ? Category::BidirectionalIterator
                     : is_base_of<std::forward_iterator_tag, ItCat>::value
                           ? Category::ForwardIterator
                           : is_base_of<std::output_iterator_tag, ItCat>::value
                                 ? Category::OutputIterator
                                 : is_base_of<std::input_iterator_tag, ItCat>::value
                                       ? Category::InputIterator
                                       : Category::NoTransformation;
}
/**\internal
 * This overload is selected for pointer types => RandomAccessIterator.
 */
template <typename T>
constexpr enable_if<std::is_pointer<T>::value, Category> iteratorCategories(float)
{
    return Category::RandomAccessIterator;
}
/**\internal
 * This overload is selected if T does not work with iterator_traits.
 */
template <typename T> constexpr Category iteratorCategories(...)
{
    return Category::NoTransformation;
}

/**\internal
 * Simple trait to identify whether a type T is a class template or not.
 */
template <typename T> struct is_class_template : public false_type
{
};
template <template <typename...> class C, typename... Ts>
struct is_class_template<C<Ts...>> : public true_type
{
};

/**\internal
 * Returns the Category for the given type \p T.
 */
template <typename T> constexpr Category typeCategory()
{
    return (is_same<T, bool>::value || is_same<T, short>::value ||
            is_same<T, unsigned short>::value || is_same<T, int>::value ||
            is_same<T, unsigned int>::value || is_same<T, float>::value ||
            is_same<T, double>::value)
               ? Category::ArithmeticVectorizable
               : iteratorCategories<T>(int()) != Category::NoTransformation
                     ? iteratorCategories<T>(int())
                     : is_class_template<T>::value ? Category::ClassTemplate
                                                   : Category::NoTransformation;
}

/**\internal
 * Trait determining the number of data members that get<N>(x) can access.
 * The type \p T either has to provide a std::tuple_size specialization or contain a
 * constexpr tuple_size member.
 */
template <typename T, size_t TupleSize = std::tuple_size<T>::value>
constexpr size_t determine_tuple_size()
{
    return TupleSize;
}
template <typename T, size_t TupleSize = T::tuple_size>
constexpr size_t determine_tuple_size(size_t = T::tuple_size)
{
    return TupleSize;
}

// workaround for MSVC limitation: constexpr functions in template arguments
// confuse the compiler
template <typename T> struct determine_tuple_size_
: public std::integral_constant<size_t, determine_tuple_size<T>()>
{};

namespace
{
template <typename T> struct The_simdization_for_the_requested_type_is_not_implemented;
}  // unnamed namespace

/**\internal
 * The type behind the simdize expression whose member type \c type determines the
 * transformed type.
 *
 * \tparam T The type to be transformed.
 * \tparam N The width the resulting vectorized type should have. A value of 0 lets the
 *           implementation choose the width.
 * \tparam MT The base type to use for mask types. If set to \c void the implementation
 *            chooses the type itself.
 * \tparam Category The type category of \p T. This determines the implementation strategy
 *                  (via template specialization).
 */
template <typename T, size_t N, typename MT, Category = typeCategory<T>()>
struct ReplaceTypes : public The_simdization_for_the_requested_type_is_not_implemented<T>
{
};

/**\internal
 * Specialization of ReplaceTypes that is used for types that should not be transformed by
 * simdize.
 */
template <typename T, size_t N, typename MT> struct ReplaceTypes<T, N, MT, Category::NoTransformation>
{
    typedef T type;
};

/**\internal
 * The ReplaceTypes class template is nicer to use as an alias template. This is exported
 * to the outer Vc namespace.
 */
template <typename T, size_t N = 0, typename MT = void>
using simdize = typename SimdizeDetail::ReplaceTypes<T, N, MT>::type;

// Alias for Vector<T, Abi> with size() == N, or SimdArray<T, N> otherwise.
template <class T, size_t N,
          class Best = typename Common::select_best_vector_type<T, N>::type>
using deduce_vector_t =
    typename std::conditional<Best::size() == N, Best, SimdArray<T, N>>::type;

/**\internal
 * ReplaceTypes specialization for simdizable arithmetic types. This results in either
 * Vector<T> or SimdArray<T, N>.
 */
template <typename T, size_t N, typename MT>
struct ReplaceTypes<T, N, MT, Category::ArithmeticVectorizable>
    : public conditional<N == 0, Vector<T>, deduce_vector_t<T, N>> {
};

/**\internal
 * ReplaceTypes specialization for bool. This results either in Mask<MT> or
 * SimdMaskArray<MT, N>.
 */
template <size_t N, typename MT>
struct ReplaceTypes<bool, N, MT, Category::ArithmeticVectorizable>
    : public std::enable_if<true, typename ReplaceTypes<MT, N, MT>::type::mask_type> {
};
/**\internal
 * ReplaceTypes specialization for bool and MT = void. In that case MT is set to float.
 */
template <size_t N>
struct ReplaceTypes<bool, N, void, Category::ArithmeticVectorizable>
    : public ReplaceTypes<bool, N, float, Category::ArithmeticVectorizable>
{
};

/**\internal
 * This type substitutes the first type (\p T) in \p Remaining via simdize<T, N, MT> and
 * appends it to the Typelist in \p Replaced. If \p N = 0, the first simdize expression
 * that yields a vectorized type determines \p N for the subsequent SubstituteOneByOne
 * instances.
 */
template <size_t N, typename MT, typename Replaced, typename... Remaining>
struct SubstituteOneByOne;

/**\internal
 * Template specialization for the case that there is at least one type in \p Remaining.
 * The member type \p type recurses via SubstituteOneByOne.
 */
template <size_t N, typename MT, typename... Replaced, typename T,
          typename... Remaining>
struct SubstituteOneByOne<N, MT, Typelist<Replaced...>, T, Remaining...>
{
private:
    /**\internal
     * If \p U::size() yields a constant expression convertible to size_t then value will
     * be equal to U::size(), 0 otherwise.
     */
    template <typename U, size_t M = U::Size>
    static std::integral_constant<size_t, M> size_or_0(int);
    template <typename U> static std::integral_constant<size_t, 0> size_or_0(...);

    ///\internal The vectorized type for \p T.
    using V = simdize<T, N, MT>;

    /**\internal
     * Determine the new \p N to use for the SubstituteOneByOne expression below. If N is
     * non-zero that value is used. Otherwise size_or_0<V> determines the new value.
     */
    static constexpr auto NewN = N != 0 ? N : decltype(size_or_0<V>(int()))::value;

    /**\internal
     * Determine the new \p MT type to use for the SubstituteOneByOne expression below.
     * This is normally the old \p MT type. However, if N != NewN and MT = void, NewMT is
     * set to either \c float or \p T, depending on whether \p T is \c bool or not.
     */
    typedef conditional_t<(N != NewN && is_same<MT, void>::value),
                          conditional_t<is_same<T, bool>::value, float, T>, MT> NewMT;

public:
    /**\internal
     * An alias to the type member of the completed recursion over SubstituteOneByOne.
     */
    using type = typename SubstituteOneByOne<NewN, NewMT, Typelist<Replaced..., V>,
                                             Remaining...>::type;
};

///\internal Generates the SubstitutedWithValues member. This needs specialization for the
/// number of types in the template argument list.
template <size_t Size, typename... Replaced> struct SubstitutedBase;
///\internal Specialization for one type parameter.
template <typename Replaced> struct SubstitutedBase<1, Replaced> {
    template <typename ValueT, template <typename, ValueT...> class C, ValueT... Values>
    using SubstitutedWithValues = C<Replaced, Values...>;
};
///\internal Specialization for two type parameters.
template <typename R0, typename R1> struct SubstitutedBase<2, R0, R1>
{
    template <typename ValueT, template <typename, typename, ValueT...> class C,
              ValueT... Values>
    using SubstitutedWithValues = C<R0, R1, Values...>;
};
///\internal Specialization for three type parameters.
template <typename R0, typename R1, typename R2> struct SubstitutedBase<3, R0, R1, R2>
{
    template <typename ValueT, template <typename, typename, typename, ValueT...> class C,
              ValueT... Values>
    using SubstitutedWithValues = C<R0, R1, R2, Values...>;
};
#if defined Vc_ICC || defined Vc_MSVC
#define Vc_VALUE_PACK_EXPANSION_IS_BROKEN 1
#endif
///\internal Specialization for four type parameters.
template <typename... Replaced> struct SubstitutedBase<4, Replaced...> {
#ifndef Vc_VALUE_PACK_EXPANSION_IS_BROKEN
    template <typename ValueT,
              template <typename, typename, typename, typename, ValueT...> class C,
              ValueT... Values>
    using SubstitutedWithValues = C<Replaced..., Values...>;
#endif // Vc_VALUE_PACK_EXPANSION_IS_BROKEN
};
///\internal Specialization for five type parameters.
template <typename... Replaced> struct SubstitutedBase<5, Replaced...> {
#ifndef Vc_VALUE_PACK_EXPANSION_IS_BROKEN
    template <typename ValueT, template <typename, typename, typename, typename, typename,
                                         ValueT...> class C,
              ValueT... Values>
    using SubstitutedWithValues = C<Replaced..., Values...>;
#endif // Vc_VALUE_PACK_EXPANSION_IS_BROKEN
};
///\internal Specialization for six type parameters.
template <typename... Replaced> struct SubstitutedBase<6, Replaced...> {
#ifndef Vc_VALUE_PACK_EXPANSION_IS_BROKEN
    template <typename ValueT, template <typename, typename, typename, typename, typename,
                                         typename, ValueT...> class C,
              ValueT... Values>
    using SubstitutedWithValues = C<Replaced..., Values...>;
#endif // Vc_VALUE_PACK_EXPANSION_IS_BROKEN
};
///\internal Specialization for seven type parameters.
template <typename... Replaced> struct SubstitutedBase<7, Replaced...> {
#ifndef Vc_VALUE_PACK_EXPANSION_IS_BROKEN
    template <typename ValueT, template <typename, typename, typename, typename, typename,
                                         typename, typename, ValueT...> class C,
              ValueT... Values>
    using SubstitutedWithValues = C<Replaced..., Values...>;
#endif // Vc_VALUE_PACK_EXPANSION_IS_BROKEN
};
///\internal Specialization for eight type parameters.
template <typename... Replaced> struct SubstitutedBase<8, Replaced...> {
#ifndef Vc_VALUE_PACK_EXPANSION_IS_BROKEN
    template <typename ValueT, template <typename, typename, typename, typename, typename,
                                         typename, typename, typename, ValueT...> class C,
              ValueT... Values>
    using SubstitutedWithValues = C<Replaced..., Values...>;
#endif // Vc_VALUE_PACK_EXPANSION_IS_BROKEN
};

/**\internal
 * Template specialization that ends the recursion and determines the return type \p type.
 * The end of the recursion is identified by an empty typelist (i.e. no template
 * parameters) after the Typelist parameter.
 */
template <size_t N_, typename MT, typename Replaced0, typename... Replaced>
struct SubstituteOneByOne<N_, MT, Typelist<Replaced0, Replaced...>>
{
    /**\internal
     * Return type for returning the vector width and list of substituted types
     */
    struct type
        : public SubstitutedBase<sizeof...(Replaced) + 1, Replaced0, Replaced...> {
        static constexpr auto N = N_;
        /**\internal
         * Alias template to construct a class template instantiation with the replaced
         * types.
         */
        template <template <typename...> class C>
        using Substituted = C<Replaced0, Replaced...>;
    };
};

/**\internal
 * Vectorized class templates are not substituted directly by ReplaceTypes/simdize.
 * Instead the replaced type is used as a base class for an adapter type which enables
 * the addition of extra operations. Specifically the following features are added:
 * \li a constexpr \p size() function, which returns the width of the vectorization. Note
 *     that this may hide a \p size() member in the original class template (e.g. for STL
 *     container classes).
 * \li The member type \p base_type is an alias for the vectorized (i.e. substituted)
 *     class template
 * \li The member type \p scalar_type is an alias for the class template argument
 *     originally passed to the \ref simdize expression.
 *
 * \tparam Scalar
 * \tparam Base
 * \tparam N
 */
template <typename Scalar, typename Base, size_t N> class Adapter;

/**\internal
 * Specialization of ReplaceTypes for class templates (\p C) where each template argument
 * needs to be substituted via SubstituteOneByOne.
 */
template <template <typename...> class C, typename... Ts, size_t N, typename MT>
struct ReplaceTypes<C<Ts...>, N, MT, Category::ClassTemplate>
{
    ///\internal The \p type member of the SubstituteOneByOne instantiation
    using SubstitutionResult =
        typename SubstituteOneByOne<N, MT, Typelist<>, Ts...>::type;
    /**\internal
     * This expression instantiates the class template \p C with the substituted template
     * arguments in the \p Ts parameter pack. The alias \p Vectorized thus is the
     * vectorized equivalent to \p C<Ts...>.
     */
    using Vectorized = typename SubstitutionResult::template Substituted<C>;
    /**\internal
     * The result type of this ReplaceTypes instantiation is set to \p C<Ts...> if no
     * template parameter substitution was done in SubstituteOneByOne. Otherwise, the type
     * aliases an Adapter instantiation.
     */
    using type = conditional_t<is_same<C<Ts...>, Vectorized>::value, C<Ts...>,
                               Adapter<C<Ts...>, Vectorized, SubstitutionResult::N>>;
};

/**\internal
 * Specialization of the ReplaceTypes class template allowing transformation of class
 * templates with non-type parameters. This is impossible to express with variadic
 * templates and therefore requires a lot of code duplication.
 */
#ifdef Vc_VALUE_PACK_EXPANSION_IS_BROKEN
// ICC barfs on packs of values
#define Vc_DEFINE_NONTYPE_REPLACETYPES_(ValueType_)                                      \
    template <template <typename, ValueType_...> class C, typename T, ValueType_ Value0, \
              ValueType_... Values>                                                      \
    struct is_class_template<C<T, Value0, Values...>> : public true_type {               \
    };                                                                                   \
    template <template <typename, typename, ValueType_...> class C, typename T0,         \
              typename T1, ValueType_ Value0, ValueType_... Values>                      \
    struct is_class_template<C<T0, T1, Value0, Values...>> : public true_type {          \
    };                                                                                   \
    template <template <typename, typename, typename, ValueType_...> class C,            \
              typename T0, typename T1, typename T2, ValueType_ Value0,                  \
              ValueType_... Values>                                                      \
    struct is_class_template<C<T0, T1, T2, Value0, Values...>> : public true_type {      \
    };                                                                                   \
    template <template <typename, typename, typename, typename, ValueType_...> class C,  \
              typename T0, typename T1, typename T2, typename T3, ValueType_ Value0,     \
              ValueType_... Values>                                                      \
    struct is_class_template<C<T0, T1, T2, T3, Value0, Values...>> : public true_type {  \
    };                                                                                   \
    template <template <typename, typename, typename, typename, typename, ValueType_...> \
              class C,                                                                   \
              typename T0, typename T1, typename T2, typename T3, typename T4,           \
              ValueType_ Value0, ValueType_... Values>                                   \
    struct is_class_template<C<T0, T1, T2, T3, T4, Value0, Values...>>                   \
        : public true_type {                                                             \
    };                                                                                   \
    template <template <typename, typename, typename, typename, typename, typename,      \
                        ValueType_...> class C,                                          \
              typename T0, typename T1, typename T2, typename T3, typename T4,           \
              typename T5, ValueType_ Value0, ValueType_... Values>                      \
    struct is_class_template<C<T0, T1, T2, T3, T4, T5, Value0, Values...>>               \
        : public true_type {                                                             \
    };                                                                                   \
    template <template <typename, typename, typename, typename, typename, typename,      \
                        typename, ValueType_...> class C,                                \
              typename T0, typename T1, typename T2, typename T3, typename T4,           \
              typename T5, typename T6, ValueType_ Value0, ValueType_... Values>         \
    struct is_class_template<C<T0, T1, T2, T3, T4, T5, T6, Value0, Values...>>           \
        : public true_type {                                                             \
    };                                                                                   \
    template <template <typename, ValueType_> class C, typename T0, ValueType_ Value0,   \
              size_t N, typename MT>                                                     \
    struct ReplaceTypes<C<T0, Value0>, N, MT, Category::ClassTemplate> {                 \
        typedef typename SubstituteOneByOne<N, MT, Typelist<>, T0>::type tmp;            \
        typedef typename tmp::template SubstitutedWithValues<ValueType_, C, Value0>      \
            Substituted;                                                                 \
        static constexpr auto NN = tmp::N;                                               \
        typedef conditional_t<is_same<C<T0, Value0>, Substituted>::value, C<T0, Value0>, \
                              Adapter<C<T0, Value0>, Substituted, NN>> type;             \
    };                                                                                   \
    template <template <typename, typename, ValueType_> class C, typename T0,            \
              typename T1, ValueType_ Value0, size_t N, typename MT>                     \
    struct ReplaceTypes<C<T0, T1, Value0>, N, MT, Category::ClassTemplate> {             \
        typedef typename SubstituteOneByOne<N, MT, Typelist<>, T0, T1>::type tmp;        \
        typedef typename tmp::template SubstitutedWithValues<ValueType_, C, Value0>      \
            Substituted;                                                                 \
        static constexpr auto NN = tmp::N;                                               \
        typedef conditional_t<is_same<C<T0, T1, Value0>, Substituted>::value,            \
                              C<T0, T1, Value0>,                                         \
                              Adapter<C<T0, T1, Value0>, Substituted, NN>> type;         \
    };                                                                                   \
    template <template <typename, typename, typename, ValueType_> class C, typename T0,  \
              typename T1, typename T2, ValueType_ Value0, size_t N, typename MT>        \
    struct ReplaceTypes<C<T0, T1, T2, Value0>, N, MT, Category::ClassTemplate> {         \
        typedef typename SubstituteOneByOne<N, MT, Typelist<>, T0, T1, T2>::type tmp;    \
        typedef typename tmp::template SubstitutedWithValues<ValueType_, C, Value0>      \
            Substituted;                                                                 \
        static constexpr auto NN = tmp::N;                                               \
        typedef conditional_t<is_same<C<T0, T1, T2, Value0>, Substituted>::value,        \
                              C<T0, T1, T2, Value0>,                                     \
                              Adapter<C<T0, T1, T2, Value0>, Substituted, NN>> type;     \
    }
#else
#define Vc_DEFINE_NONTYPE_REPLACETYPES_(ValueType_)                                      \
    template <template <typename, ValueType_...> class C, typename T, ValueType_ Value0, \
              ValueType_... Values>                                                      \
    struct is_class_template<C<T, Value0, Values...>> : public true_type {               \
    };                                                                                   \
    template <template <typename, typename, ValueType_...> class C, typename T0,         \
              typename T1, ValueType_ Value0, ValueType_... Values>                      \
    struct is_class_template<C<T0, T1, Value0, Values...>> : public true_type {          \
    };                                                                                   \
    template <template <typename, typename, typename, ValueType_...> class C,            \
              typename T0, typename T1, typename T2, ValueType_ Value0,                  \
              ValueType_... Values>                                                      \
    struct is_class_template<C<T0, T1, T2, Value0, Values...>> : public true_type {      \
    };                                                                                   \
    template <template <typename, typename, typename, typename, ValueType_...> class C,  \
              typename T0, typename T1, typename T2, typename T3, ValueType_ Value0,     \
              ValueType_... Values>                                                      \
    struct is_class_template<C<T0, T1, T2, T3, Value0, Values...>> : public true_type {  \
    };                                                                                   \
    template <template <typename, typename, typename, typename, typename, ValueType_...> \
              class C,                                                                   \
              typename T0, typename T1, typename T2, typename T3, typename T4,           \
              ValueType_ Value0, ValueType_... Values>                                   \
    struct is_class_template<C<T0, T1, T2, T3, T4, Value0, Values...>>                   \
        : public true_type {                                                             \
    };                                                                                   \
    template <template <typename, typename, typename, typename, typename, typename,      \
                        ValueType_...> class C,                                          \
              typename T0, typename T1, typename T2, typename T3, typename T4,           \
              typename T5, ValueType_ Value0, ValueType_... Values>                      \
    struct is_class_template<C<T0, T1, T2, T3, T4, T5, Value0, Values...>>               \
        : public true_type {                                                             \
    };                                                                                   \
    template <template <typename, typename, typename, typename, typename, typename,      \
                        typename, ValueType_...> class C,                                \
              typename T0, typename T1, typename T2, typename T3, typename T4,           \
              typename T5, typename T6, ValueType_ Value0, ValueType_... Values>         \
    struct is_class_template<C<T0, T1, T2, T3, T4, T5, T6, Value0, Values...>>           \
        : public true_type {                                                             \
    };                                                                                   \
    template <template <typename, ValueType_...> class C, typename T0,                   \
              ValueType_ Value0, ValueType_... Values, size_t N, typename MT>            \
    struct ReplaceTypes<C<T0, Value0, Values...>, N, MT, Category::ClassTemplate> {      \
        typedef typename SubstituteOneByOne<N, MT, Typelist<>, T0>::type tmp;            \
        typedef typename tmp::template SubstitutedWithValues<ValueType_, C, Value0,      \
                                                             Values...> Substituted;     \
        static constexpr auto NN = tmp::N;                                               \
        typedef conditional_t<is_same<C<T0, Value0, Values...>, Substituted>::value,     \
                              C<T0, Value0, Values...>,                                  \
                              Adapter<C<T0, Value0, Values...>, Substituted, NN>> type;  \
    };                                                                                   \
    template <template <typename, typename, ValueType_...> class C, typename T0,         \
              typename T1, ValueType_ Value0, ValueType_... Values, size_t N,            \
              typename MT>                                                               \
    struct ReplaceTypes<C<T0, T1, Value0, Values...>, N, MT, Category::ClassTemplate> {  \
        typedef typename SubstituteOneByOne<N, MT, Typelist<>, T0, T1>::type tmp;        \
        typedef typename tmp::template SubstitutedWithValues<ValueType_, C, Value0,      \
                                                             Values...> Substituted;     \
        static constexpr auto NN = tmp::N;                                               \
        typedef conditional_t<is_same<C<T0, T1, Value0, Values...>, Substituted>::value, \
                              C<T0, T1, Value0, Values...>,                              \
                              Adapter<C<T0, T1, Value0, Values...>, Substituted, NN>>    \
            type;                                                                        \
    };                                                                                   \
    template <template <typename, typename, typename, ValueType_...> class C,            \
              typename T0, typename T1, typename T2, ValueType_ Value0,                  \
              ValueType_... Values, size_t N, typename MT>                               \
    struct ReplaceTypes<C<T0, T1, T2, Value0, Values...>, N, MT,                         \
                        Category::ClassTemplate> {                                       \
        typedef typename SubstituteOneByOne<N, MT, Typelist<>, T0, T1, T2>::type tmp;    \
        typedef typename tmp::template SubstitutedWithValues<ValueType_, C, Value0,      \
                                                             Values...> Substituted;     \
        static constexpr auto NN = tmp::N;                                               \
        typedef conditional_t<                                                           \
            is_same<C<T0, T1, T2, Value0, Values...>, Substituted>::value,               \
            C<T0, T1, T2, Value0, Values...>,                                            \
            Adapter<C<T0, T1, T2, Value0, Values...>, Substituted, NN>> type;            \
    }
#endif  // Vc_VALUE_PACK_EXPANSION_IS_BROKEN
Vc_DEFINE_NONTYPE_REPLACETYPES_(bool);
Vc_DEFINE_NONTYPE_REPLACETYPES_(wchar_t);
Vc_DEFINE_NONTYPE_REPLACETYPES_(char);
Vc_DEFINE_NONTYPE_REPLACETYPES_(  signed char);
Vc_DEFINE_NONTYPE_REPLACETYPES_(unsigned char);
Vc_DEFINE_NONTYPE_REPLACETYPES_(  signed short);
Vc_DEFINE_NONTYPE_REPLACETYPES_(unsigned short);
Vc_DEFINE_NONTYPE_REPLACETYPES_(  signed int);
Vc_DEFINE_NONTYPE_REPLACETYPES_(unsigned int);
Vc_DEFINE_NONTYPE_REPLACETYPES_(  signed long);
Vc_DEFINE_NONTYPE_REPLACETYPES_(unsigned long);
Vc_DEFINE_NONTYPE_REPLACETYPES_(  signed long long);
Vc_DEFINE_NONTYPE_REPLACETYPES_(unsigned long long);
#undef Vc_DEFINE_NONTYPE_REPLACETYPES_

// preferred_construction {{{
namespace preferred_construction_impl
{
template <typename T> T create();
// 0: paren init
template <class Type, class... Init, class = decltype(Type(create<Init>()...))>
constexpr std::integral_constant<int, 0> test(int);
// 1: 1-brace init
template <class Type, class... Init, class = decltype(Type{create<Init>()...})>
constexpr std::integral_constant<int, 1> test(float);
// 2: 2-brace init
template <class Type, class... Init, class T, class = decltype(Type{{create<Init>()...}})>
constexpr std::integral_constant<int, 2> test(T);
// 3: no init at all
template <class Type, class... Init> constexpr std::integral_constant<int, 3> test(...);
}  // namespace preferred_construction_impl

template <class Type, class... Init>
constexpr inline decltype(preferred_construction_impl::test<Type, Init...>(0))
preferred_construction()
{
    return {};
}

// }}}
// get_dispatcher {{{
/**\internal
 * Uses either the `vc_get_<I>` member function of \p x or `std::get<I>(x)` to retrieve
 * the `I`-th member of \p x.
 */
template <size_t I, typename T,
          typename R = decltype(std::declval<T &>().template vc_get_<I>())>
R get_dispatcher(T &x, void * = nullptr)
{
    return x.template vc_get_<I>();
}
template <size_t I, typename T,
          typename R = decltype(std::declval<const T &>().template vc_get_<I>())>
R get_dispatcher(const T &x, void * = nullptr)
{
    return x.template vc_get_<I>();
}
template <size_t I, typename T, typename R = decltype(std::get<I>(std::declval<T &>()))>
R get_dispatcher(T &x, int = 0)
{
    return std::get<I>(x);
}
template <size_t I, typename T,
          typename R = decltype(std::get<I>(std::declval<const T &>()))>
R get_dispatcher(const T &x, int = 0)
{
    return std::get<I>(x);
}

// }}}
// my_tuple_element {{{
template <size_t I, class T, class = void>
struct my_tuple_element : std::tuple_element<I, T> {
};

template <size_t I, class T>
struct my_tuple_element<
    I, T, typename std::conditional<
              true, void, decltype(std::declval<T>().template vc_get_<I>())>::type> {
    using type =
        typename std::decay<decltype(std::declval<T>().template vc_get_<I>())>::type;
};

// }}}
// homogeneous_sizeof {{{
/**\internal
 * This trait determines the `sizeof` of all fundamental types (i.e. recursively, when
 * needed) in the template parameter pack \p Ts. If all fundamental types have equal
 * `sizeof`, the value is "returned" in the `value` member. Otherwise `value` is 0.
 */
template <class... Ts> struct homogeneous_sizeof;
template <class T, class = void> struct homogeneous_sizeof_one;
template <class T>
struct homogeneous_sizeof_one<T,
                              typename std::enable_if<std::is_arithmetic<T>::value>::type>
    : std::integral_constant<size_t, sizeof(T)> {
};
template <class T0> struct homogeneous_sizeof<T0> : homogeneous_sizeof_one<T0> {
};

template <class T0, class... Ts>
struct homogeneous_sizeof<T0, Ts...>
    : std::integral_constant<size_t, homogeneous_sizeof<T0>::value ==
                                             homogeneous_sizeof<Ts...>::value
                                         ? homogeneous_sizeof<T0>::value
                                         : 0> {
};

template <class T, size_t... Is>
std::integral_constant<
    size_t, homogeneous_sizeof<typename my_tuple_element<Is, T>::type...>::value>
    homogeneous_sizeof_helper(index_sequence<Is...>);

template <class T>
struct homogeneous_sizeof_one<T, typename std::enable_if<std::is_class<T>::value>::type>
    : decltype(homogeneous_sizeof_helper<T>(
          make_index_sequence<determine_tuple_size_<T>::value>())) {
};

// }}}
// class Adapter {{{
template <typename Scalar, typename Base, size_t N> class Adapter : public Base
{
private:
    /// helper for the broadcast ctor below, error case
    template <std::size_t... Indexes, int X>
    Adapter(Vc::index_sequence<Indexes...>, const Scalar,
            std::integral_constant<int, X>)
    {
        static_assert(
            X < 3, "Failed to construct an object of type Base. Neither via "
                   "parenthesis-init, brace-init, nor double-brace init appear to work.");
    }

    /// helper for the broadcast ctor below using double braces for Base initialization
    template <std::size_t... Indexes>
    Adapter(Vc::index_sequence<Indexes...>, const Scalar &x_,
            std::integral_constant<int, 2>)
        : Base{{get_dispatcher<Indexes>(x_)...}}
    {
    }

    /// helper for the broadcast ctor below using single braces for Base initialization
    template <std::size_t... Indexes>
    Adapter(Vc::index_sequence<Indexes...>, const Scalar &x_,
            std::integral_constant<int, 1>)
        : Base{get_dispatcher<Indexes>(x_)...}
    {
    }

    /// helper for the broadcast ctor below using parenthesis for Base initialization
    template <std::size_t... Indexes>
    Adapter(Vc::index_sequence<Indexes...>, const Scalar &x_,
            std::integral_constant<int, 0>)
        : Base(get_dispatcher<Indexes>(x_)...)
    {
    }

    template <std::size_t... Indexes>
    Adapter(Vc::index_sequence<Indexes...> seq_, const Scalar &x_)
        : Adapter(seq_, x_,
                  preferred_construction<Base, decltype(get_dispatcher<Indexes>(
                                                   std::declval<const Scalar &>()))...>())
    {
    }

public:
    /// The SIMD vector width of the members.
    static constexpr size_t size() { return N; }
    static constexpr size_t Size = N;

    /// The vectorized base class template instantiation this Adapter class derives from.
    using base_type = Base;
    /// The original non-vectorized class template instantiation that was passed to the
    /// simdize expression.
    using scalar_type = Scalar;

    /// Allow default construction. This is automatically ill-formed if Base() is
    /// ill-formed.
    Adapter() = default;

    /// Defaulted copy and move construction and assignment
#if defined Vc_CLANG && Vc_CLANG < 0x30700
    Vc_INTRINSIC Adapter(const Adapter &x) : Base(x) {}
#else
    Adapter(const Adapter &) = default;
#endif
    /// Defaulted copy and move construction and assignment
    Adapter(Adapter &&) = default;
    /// Defaulted copy and move construction and assignment
    Adapter &operator=(const Adapter &) = default;
    /// Defaulted copy and move construction and assignment
    Adapter &operator=(Adapter &&) = default;

    /// Broadcast constructor
    template <typename U, size_t TupleSize = determine_tuple_size_<Scalar>::value,
              typename Seq = Vc::make_index_sequence<TupleSize>,
              typename = enable_if<std::is_convertible<U, Scalar>::value>>
    Adapter(U &&x_)
        : Adapter(Seq(), static_cast<const Scalar &>(x_))
    {
    }

    /// Generator constructor {{{
    template <class F,
              class = decltype(static_cast<Scalar>(std::declval<F>()(
                  size_t())))>  // F returns objects that are convertible to S
    Adapter(F &&fun);           // implementation below

    // }}}
    /// perfect forward all Base constructors
    template <typename A0, typename... Args,
              typename = typename std::enable_if<
                  !Traits::is_index_sequence<A0>::value &&
                  (sizeof...(Args) > 0 || !std::is_convertible<A0, Scalar>::value)>::type>
    Adapter(A0 &&arg0_, Args &&... arguments_)
        : Base(std::forward<A0>(arg0_), std::forward<Args>(arguments_)...)
    {
    }

    /// perfect forward Base constructors that accept an initializer_list
    template <typename T,
              typename = decltype(Base(std::declval<const std::initializer_list<T> &>()))>
    Adapter(const std::initializer_list<T> &l_)
        : Base(l_)
    {
    }

    /// Overload the new operator to adhere to the alignment requirements which C++11
    /// ignores by default.
    void *operator new(size_t size)
    {
        return Vc::Common::aligned_malloc<alignof(Adapter)>(size);
    }
    void *operator new(size_t, void *p_) { return p_; }
    void *operator new[](size_t size)
    {
        return Vc::Common::aligned_malloc<alignof(Adapter)>(size);
    }
    void *operator new[](size_t , void *p_) { return p_; }
    void operator delete(void *ptr_, size_t) { Vc::Common::free(ptr_); }
    void operator delete(void *, void *) {}
    void operator delete[](void *ptr_, size_t) { Vc::Common::free(ptr_); }
    void operator delete[](void *, void *) {}
};  // }}}
// delete compare operators for Adapter {{{
/**\internal
 * Delete compare operators for simdize<tuple<...>> types because the tuple compares
 * require the compares to be bool based.
 */
template <class... TTypes, class... TTypesV, class... UTypes, class... UTypesV, size_t N>
inline void operator==(
    const Adapter<std::tuple<TTypes...>, std::tuple<TTypesV...>, N> &t,
    const Adapter<std::tuple<UTypes...>, std::tuple<UTypesV...>, N> &u) = delete;
template <class... TTypes, class... TTypesV, class... UTypes, class... UTypesV, size_t N>
inline void operator!=(
    const Adapter<std::tuple<TTypes...>, std::tuple<TTypesV...>, N> &t,
    const Adapter<std::tuple<UTypes...>, std::tuple<UTypesV...>, N> &u) = delete;
template <class... TTypes, class... TTypesV, class... UTypes, class... UTypesV, size_t N>
inline void operator<=(
    const Adapter<std::tuple<TTypes...>, std::tuple<TTypesV...>, N> &t,
    const Adapter<std::tuple<UTypes...>, std::tuple<UTypesV...>, N> &u) = delete;
template <class... TTypes, class... TTypesV, class... UTypes, class... UTypesV, size_t N>
inline void operator>=(
    const Adapter<std::tuple<TTypes...>, std::tuple<TTypesV...>, N> &t,
    const Adapter<std::tuple<UTypes...>, std::tuple<UTypesV...>, N> &u) = delete;
template <class... TTypes, class... TTypesV, class... UTypes, class... UTypesV, size_t N>
inline void operator<(
    const Adapter<std::tuple<TTypes...>, std::tuple<TTypesV...>, N> &t,
    const Adapter<std::tuple<UTypes...>, std::tuple<UTypesV...>, N> &u) = delete;
template <class... TTypes, class... TTypesV, class... UTypes, class... UTypesV, size_t N>
inline void operator>(
    const Adapter<std::tuple<TTypes...>, std::tuple<TTypesV...>, N> &t,
    const Adapter<std::tuple<UTypes...>, std::tuple<UTypesV...>, N> &u) = delete;
// }}}
/** @}*/
}  // namespace SimdizeDetail }}}
}  // namespace Vc

namespace std  // {{{
{
/**\internal
 * A std::tuple_size specialization for the SimdizeDetail::Adapter class.
 */
template <typename Scalar, typename Base, size_t N>
class tuple_size<Vc::SimdizeDetail::Adapter<Scalar, Base, N>> : public tuple_size<Base>
{
};
/**\internal
 * A std::tuple_element specialization for the SimdizeDetail::Adapter class.
 */
template <size_t I, typename Scalar, typename Base, size_t N>
class tuple_element<I, Vc::SimdizeDetail::Adapter<Scalar, Base, N>>
    : public tuple_element<I, Base>
{
};
// std::get does not need additional work because Vc::Adapter derives from
// C<Ts...> and therefore if get<N>(C<Ts...>) works it works for Adapter as well.

/**\internal
 * A std::allocator specialization for SimdizeDetail::Adapter which uses the Vc::Allocator
 * class to make allocation correctly aligned per default.
 */
template <typename S, typename T, size_t N>
class allocator<Vc::SimdizeDetail::Adapter<S, T, N>>
    : public Vc::Allocator<Vc::SimdizeDetail::Adapter<S, T, N>>
{
public:
    template <typename U> struct rebind
    {
        typedef std::allocator<U> other;
    };
};
}  // namespace std }}}

namespace Vc_VERSIONED_NAMESPACE
{
namespace SimdizeDetail
{
/**\addtogroup Simdize
 * @{
 */
/**\internal
 * Since std::decay can ICE GCC (with types that are declared as may_alias), this is used
 * as an alternative approach. Using decltype the template type deduction implements the
 * std::decay behavior.
 */
template <typename T> static inline T decay_workaround(const T &x) { return x; }

// assign_impl {{{
/**\internal
 * Generic implementation of assign using the std::tuple get interface.
 */
template <typename S, typename T, size_t N, size_t... Indexes>
inline void assign_impl(Adapter<S, T, N> &a, size_t i, const S &x,
                        Vc::index_sequence<Indexes...>)
{
    const std::tuple<decltype(decay_workaround(get_dispatcher<Indexes>(x)))...> tmp(
        decay_workaround(get_dispatcher<Indexes>(x))...);
    auto &&unused = {(get_dispatcher<Indexes>(a)[i] = get_dispatcher<Indexes>(tmp), 0)...};
    if (&unused == &unused) {}
}  // }}}
// construct (parens, braces, double-braces) {{{
template <class S, class... Args>
S construct(std::integral_constant<int, 0>, Args &&... args)
{
    return S(std::forward<Args>(args)...);
}
template <class S, class... Args>
S construct(std::integral_constant<int, 1>, Args &&... args)
{
    return S{std::forward<Args>(args)...};
}
template <class S, class... Args>
S construct(std::integral_constant<int, 2>, Args &&... args)
{
    return S{{std::forward<Args>(args)...}};
}
// }}}
// extract_impl {{{
/**\internal
 * index_sequence based implementation for extract below.
 */
template <typename S, typename T, size_t N, size_t... Indexes>
inline S extract_impl(const Adapter<S, T, N> &a, size_t i, Vc::index_sequence<Indexes...>)
{
    const std::tuple<decltype(decay_workaround(get_dispatcher<Indexes>(a)[i]))...> tmp(
        decay_workaround(get_dispatcher<Indexes>(a)[i])...);
    return construct<S>(
        preferred_construction<S, decltype(decay_workaround(
                                      get_dispatcher<Indexes>(a)[i]))...>(),
        decay_workaround(get_dispatcher<Indexes>(a)[i])...);
    //return S(get_dispatcher<Indexes>(tmp)...);
}
// }}}
// shifted_impl {{{
template <typename S, typename T, std::size_t N, std::size_t... Indexes>
inline Adapter<S, T, N> shifted_impl(const Adapter<S, T, N> &a, int shift,
                                     Vc::index_sequence<Indexes...>)
{
    Adapter<S, T, N> r;
    auto &&unused = {(get_dispatcher<Indexes>(r) = get_dispatcher<Indexes>(a).shifted(shift), 0)...};
    if (&unused == &unused) {}
    return r;
}
// }}}
// shifted(Adapter) {{{
/**
 * Returns a new vectorized object where each entry is shifted by \p shift. This basically
 * calls Vector<T>::shifted on every entry.
 *
 * \param a The object to apply the shift on.
 * \param shift The number of entries to shift by.
 * \returns a copy of \p a shifted by \p shift.
 */
template <typename S, typename T, size_t N>
inline Adapter<S, T, N> shifted(const Adapter<S, T, N> &a, int shift)
{
    return shifted_impl(a, shift, Vc::make_index_sequence<determine_tuple_size<T>()>());
}
// }}}
// swap_impl {{{
/** \internal
 * Generic implementation of simdize_swap using the std::tuple get interface.
 */
template <typename S, typename T, std::size_t N, std::size_t... Indexes>
inline void swap_impl(Adapter<S, T, N> &a, std::size_t i, S &x,
                      Vc::index_sequence<Indexes...>)
{
    const auto &a_const = a;
    const std::tuple<decltype(decay_workaround(get_dispatcher<Indexes>(a_const)[0]))...>
        tmp{decay_workaround(get_dispatcher<Indexes>(a_const)[i])...};
    auto &&unused = {(get_dispatcher<Indexes>(a)[i] = get_dispatcher<Indexes>(x), 0)...};
    auto &&unused2 = {(get_dispatcher<Indexes>(x) = get_dispatcher<Indexes>(tmp), 0)...};
    if (&unused == &unused2) {}
}
template <typename S, typename T, std::size_t N, std::size_t... Indexes>
inline void swap_impl(Adapter<S, T, N> &a, std::size_t i, Adapter<S, T, N> &b,
                      std::size_t j, Vc::index_sequence<Indexes...>)
{
    const auto &a_const = a;
    const auto &b_const = b;
    const std::tuple<decltype(decay_workaround(get_dispatcher<Indexes>(a_const)[0]))...>
        tmp{decay_workaround(get_dispatcher<Indexes>(a_const)[i])...};
    auto &&unused = {(get_dispatcher<Indexes>(a)[i] = get_dispatcher<Indexes>(b_const)[j], 0)...};
    auto &&unused2 = {(get_dispatcher<Indexes>(b)[j] = get_dispatcher<Indexes>(tmp), 0)...};
    if (&unused == &unused2) {}
}
// }}}
// swap(Adapter) {{{
/**
 * Swaps one scalar object \p x with a SIMD slot at offset \p i in the simdized object \p
 * a.
 */
template <typename S, typename T, std::size_t N>
inline void swap(Adapter<S, T, N> &a, std::size_t i, S &x)
{
    swap_impl(a, i, x, Vc::make_index_sequence<determine_tuple_size<T>()>());
}
template <typename S, typename T, std::size_t N>
inline void swap(Adapter<S, T, N> &a, std::size_t i, Adapter<S, T, N> &b, std::size_t j)
{
    swap_impl(a, i, b, j, Vc::make_index_sequence<determine_tuple_size<T>()>());
}
// }}}
template <typename A> class Scalar  // {{{
{
    using reference = typename std::add_lvalue_reference<A>::type;
    using S = typename A::scalar_type;
    using IndexSeq = Vc::make_index_sequence<determine_tuple_size<S>()>;

public:
    Scalar(reference aa, size_t ii) : a(aa), i(ii) {}

    // delete copy and move to keep the type a pure proxy temporary object.
    Scalar(const Scalar &) = delete;
    Scalar(Scalar &&) = delete;
    Scalar &operator=(const Scalar &) = delete;
    Scalar &operator=(Scalar &&) = delete;

    void operator=(const S &x) { assign_impl(a, i, x, IndexSeq()); }
    operator S() const { return extract_impl(a, i, IndexSeq()); }

    template <typename AA>
    friend inline void swap(Scalar<AA> &&a, typename AA::scalar_type &b);
    template <typename AA>
    friend inline void swap(typename AA::scalar_type &b, Scalar<AA> &&a);
    template <typename AA> friend inline void swap(Scalar<AA> &&a, Scalar<AA> &&b);

private:
    reference a;
    size_t i;
};  // }}}
// swap(Scalar) {{{
/// std::swap interface to swapping one scalar object with a (virtual) reference to
/// another object inside a vectorized object
template <typename A> inline void swap(Scalar<A> &&a, typename A::scalar_type &b)
{
    swap_impl(a.a, a.i, b, typename Scalar<A>::IndexSeq());
}
/// std::swap interface to swapping one scalar object with a (virtual) reference to
/// another object inside a vectorized object
template <typename A> inline void swap(typename A::scalar_type &b, Scalar<A> &&a)
{
    swap_impl(a.a, a.i, b, typename Scalar<A>::IndexSeq());
}

template <typename A> inline void swap(Scalar<A> &&a, Scalar<A> &&b)
{
    swap_impl(a.a, a.i, b.a, b.i, typename Scalar<A>::IndexSeq());
}
// }}}
// load_interleaved_impl {{{
template <class S, class T, size_t N, size_t... I>
inline void load_interleaved_impl(Vc::index_sequence<I...>, Adapter<S, T, N> &a,
                                  const S *mem)
{
    const InterleavedMemoryWrapper<S, decltype(decay_workaround(get_dispatcher<0>(a)))>
    wrapper(const_cast<S *>(mem));
    Vc::tie(get_dispatcher<I>(a)...) = wrapper[0];
}
// }}}
// store_interleaved_impl {{{
template <class S, class T, size_t N, size_t... I>
inline void store_interleaved_impl(Vc::index_sequence<I...>, const Adapter<S, T, N> &a,
                                   S *mem)
{
    InterleavedMemoryWrapper<S, decltype(decay_workaround(get_dispatcher<0>(a)))> wrapper(
        mem);
    wrapper[0] = Vc::tie(get_dispatcher<I>(a)...);
}
// }}}
template <typename A> class Interface  // {{{
{
    using reference = typename std::add_lvalue_reference<A>::type;
    using IndexSeq =
        Vc::make_index_sequence<determine_tuple_size<typename A::scalar_type>()>;

public:
    Interface(reference aa) : a(aa) {}

    Scalar<A> operator[](size_t i)
    {
        return {a, i};
    }
    typename A::scalar_type operator[](size_t i) const
    {
        return extract_impl(a, i, IndexSeq());
    }

    A shifted(int amount) const
    {
        return shifted_impl(a, amount, IndexSeq());
    }

    void load(const typename A::scalar_type *mem) { load_interleaved(*this, mem); }
    void store(typename A::scalar_type *mem) { store_interleaved(*this, mem); }

private:
    reference a;
};  // }}}
}  // namespace SimdizeDetail
// assign {{{
/**
 * Assigns one scalar object \p x to a SIMD slot at offset \p i in the simdized object \p
 * a.
 */
template <typename S, typename T, size_t N>
inline void assign(SimdizeDetail::Adapter<S, T, N> &a, size_t i, const S &x)
{
    SimdizeDetail::assign_impl(
        a, i, x, Vc::make_index_sequence<SimdizeDetail::determine_tuple_size<T>()>());
}
/**\internal
 * Overload for standard Vector/SimdArray types.
 */
template <typename V, typename = enable_if<Traits::is_simd_vector<V>::value>>
Vc_INTRINSIC void assign(V &v, size_t i, typename V::EntryType x)
{
    v[i] = x;
}
// }}}
// extract {{{
/**
 * Extracts and returns one scalar object from a SIMD slot at offset \p i in the simdized
 * object \p a.
 */
template <typename S, typename T, size_t N>
inline S extract(const SimdizeDetail::Adapter<S, T, N> &a, size_t i)
{
    return SimdizeDetail::extract_impl(
        a, i, Vc::make_index_sequence<SimdizeDetail::determine_tuple_size<S>()>());
}
/**\internal
 * Overload for standard Vector/SimdArray types.
 */
template <typename V, typename = enable_if<Traits::is_simd_vector<V>::value>>
Vc_INTRINSIC typename V::EntryType extract(const V &v, size_t i)
{
    return v[i];
}
// }}}
// load_interleaved {{{
template <class S, class T, size_t N>
inline void load_interleaved(SimdizeDetail::Adapter<S, T, N> &a, const S *mem)
{
    if (SimdizeDetail::homogeneous_sizeof<S>::value == 0) {
        Common::unrolled_loop<std::size_t, 0, N>(
            [&](std::size_t i) { assign(a, i, mem[i]); });
    } else {
        constexpr size_t TupleSize = SimdizeDetail::determine_tuple_size_<S>::value;
        SimdizeDetail::load_interleaved_impl(Vc::make_index_sequence<TupleSize>(), a,
                                             mem);
    }
}
template <
    class V, class T,
    class = enable_if<Traits::is_simd_vector<V>::value && std::is_arithmetic<T>::value>>
Vc_INTRINSIC void load_interleaved(V &a, const T *mem)
{
    a.load(mem, Vc::Unaligned);
}
// }}}
// store_interleaved {{{
template <class S, class T, size_t N>
inline void store_interleaved(const SimdizeDetail::Adapter<S, T, N> &a, S *mem)
{
    if (SimdizeDetail::homogeneous_sizeof<S>::value == 0) {
        Common::unrolled_loop<std::size_t, 0, N>(
            [&](std::size_t i) { mem[i] = extract(a, i); });
    } else {
        constexpr size_t TupleSize = SimdizeDetail::determine_tuple_size_<S>::value;
        SimdizeDetail::store_interleaved_impl(Vc::make_index_sequence<TupleSize>(), a,
                                              mem);
    }
}
template <
    class V, class T,
    class = enable_if<Traits::is_simd_vector<V>::value && std::is_arithmetic<T>::value>>
Vc_INTRINSIC void store_interleaved(const V &a, T *mem)
{
    a.store(mem, Vc::Unaligned);
}
// }}}
// decorate(Adapter) {{{
template <typename S, typename T, size_t N>
SimdizeDetail::Interface<SimdizeDetail::Adapter<S, T, N>> decorate(
    SimdizeDetail::Adapter<S, T, N> &a)
{
    return {a};
}
template <typename S, typename T, size_t N>
const SimdizeDetail::Interface<const SimdizeDetail::Adapter<S, T, N>> decorate(
    const SimdizeDetail::Adapter<S, T, N> &a)
{
    return {a};
}
template <class V, class = typename std::enable_if<
                       Traits::is_simd_vector<typename std::decay<V>::type>::value>>
V &&decorate(V &&v)
{
    return std::forward<V>(v);
}
// }}}
namespace SimdizeDetail
{
// Adapter::Adapter(F) Generator {{{
template <typename Scalar, typename Base, size_t N>
template <class F, class>
Adapter<Scalar, Base, N>::Adapter(F &&fun)
{
    for (size_t i = 0; i < N; ++i) {
        Vc::assign(*this, i, fun(i));
    }
}
// }}}
namespace IteratorDetails  // {{{
{
enum class Mutable { Yes, No };

template <typename It, typename V, size_t I, size_t End>
Vc_INTRINSIC V fromIteratorImpl(enable_if<(I == End), It>)
{
    return {};
}
template <typename It, typename V, size_t I, size_t End>
Vc_INTRINSIC V fromIteratorImpl(enable_if<(I < End), It> it)
{
    V r = fromIteratorImpl<It, V, I + 1, End>(it);
    Traits::decay<decltype(get_dispatcher<I>(r))> tmp;
    for (size_t j = 0; j < V::size(); ++j, ++it) {
        tmp[j] = get_dispatcher<I>(*it);
    }
    get_dispatcher<I>(r) = tmp;
    return r;
}
template <typename It, typename V>
Vc_INTRINSIC V fromIterator(enable_if<!Traits::is_simd_vector<V>::value, const It &> it)
{
    return fromIteratorImpl<It, V, 0, determine_tuple_size<V>()>(it);
}

template <typename It, typename V>
Vc_INTRINSIC V fromIterator(
    enable_if<
        Traits::is_simd_vector<V>::value && Traits::has_contiguous_storage<It>::value, It>
        it)
{
#ifndef _MSC_VER
    // this check potentially moves it past the end of a container, which is UB. Some STL
    // implementations, like MS STL, trap this.
    Vc_ASSERT(&*it + 1 == &*(it + 1));
#endif
    return V(&*it, Vc::Unaligned);
}

template <typename It, typename V>
Vc_INTRINSIC V fromIterator(enable_if<Traits::is_simd_vector<V>::value &&
                                          !Traits::has_contiguous_storage<It>::value,
                                      It>
                                it)
{
    V r;
    for (size_t j = 0; j < V::size(); ++j, ++it) {
        r[j] = *it;
    }
    return r;
}

// Note: §13.5.6 says: “An expression x->m is interpreted as (x.operator->())->m for a
// class object x of type T if T::operator->() exists and if the operator is selected as
// the best match function by the overload resolution mechanism (13.3).”
template <typename T, typename value_vector, Mutable> class Pointer;

/**\internal
 * Proxy type for a pointer returned from operator->(). The mutable variant requires at
 * least a ForwardIterator. An InputIterator cannot work since no valid copies and
 * independent iteration can be guaranteed.
 *
 * The implementation creates the pointer-like behavior by creating an lvalue for the
 * proxied data. This
 */
template <typename T, typename value_vector> class Pointer<T, value_vector, Mutable::Yes>
{
    static constexpr auto Size = value_vector::size();

public:
    /// \returns a pointer to the (temporary) member object.
    value_vector *operator->() { return &data; }

    /**
     * A Pointer can only be constructed from a scalar iterator or move constructed (for
     * function returns).
     */
    Pointer() = delete;
    Pointer(const Pointer &) = delete;
    Pointer &operator=(const Pointer &) = delete;
    Pointer &operator=(Pointer &&) = delete;

    /// required for returning the Pointer
    Pointer(Pointer &&) = default;

    /**
     * Writes the vectorized object back to the scalar objects referenced by the
     * iterator. This store is done unconditionally for the mutable variant of the
     * Pointer. The immutable Pointer OTOH does not store back at all.
     */
    ~Pointer()
    {
        // store data back to where it came from
        for (size_t i = 0; i < Size; ++i, ++begin_iterator) {
            *begin_iterator = extract(data, i);
        }
    }

    /// Construct the Pointer object from the values returned by the scalar iterator \p it.
    Pointer(const T &it) : data(fromIterator<T, value_vector>(it)), begin_iterator(it) {}

private:
    /// The vectorized object needed for dereferencing the pointer.
    value_vector data;
    /// A copy of the scalar iterator, used for storing the results back.
    T begin_iterator;
};
/**\internal
 * The immutable variant of the Pointer class specialization above. It behaves the same as
 * the mutable Pointer except that it returns a const pointer from \c operator-> and
 * avoids the write back in the destructor.
 */
template <typename T, typename value_vector> class Pointer<T, value_vector, Mutable::No>
{
    static constexpr auto Size = value_vector::size();

public:
    const value_vector *operator->() const { return &data; }

    Pointer() = delete;
    Pointer(const Pointer &) = delete;
    Pointer &operator=(const Pointer &) = delete;
    Pointer &operator=(Pointer &&) = delete;

    Pointer(Pointer &&) = default;  // required for returning the Pointer

    Pointer(const T &it) : data(fromIterator<T, value_vector>(it)) {}

private:
    value_vector data;
};

/**\internal
 * The Reference class behaves as much as possible like a reference to an object of type
 * \p value_vector. The \p Mutable parameter determines whether the referenced object my
 * be modified or not (basically whether it's a ref or a const-ref, though the semantics
 * of mutable are actually stricter than that of const. Const only determines the logical
 * constness whereas mutability identifies the constness on the bit-level.)
 *
 * \tparam T The scalar iterator type.
 * \tparam value_vector The vector object the scalar iterator needs to fill.
 * \tparam M A flag that determines whether the reference acts as a mutable or immutable
 *           reference.
 */
template <typename T, typename value_vector, Mutable M> class Reference;

///\internal mutable specialization of the Reference proxy class
template <typename T, typename value_vector>
class Reference<T, value_vector, Mutable::Yes> : public value_vector
{
    static constexpr auto Size = value_vector::size();

    using reference = typename std::add_lvalue_reference<T>::type;
    reference scalar_it;

public:
    /// Construct the reference from the given iterator \p first_it and store a reference
    /// to the iterator for write back in the assignment operator.
    Reference(reference first_it)
        : value_vector(fromIterator<T, value_vector>(first_it)), scalar_it(first_it)
    {
    }

    /// disable all copy and move operations, except the one needed for function returns
    Reference(const Reference &) = delete;
    Reference(Reference &&) = default;
    Reference &operator=(const Reference &) = delete;
    Reference &operator=(Reference &&) = delete;

    /**
     * Assignment to the reference assigns to the storage pointed to by the scalar
     * iterator as well as the reference object itself. (The compiler should eliminate the
     * store to \c this if it's never used since it is clearly a dead store.)
     */
    void operator=(const value_vector &x)
    {
        static_cast<value_vector &>(*this) = x;
        auto it = scalar_it;
        for (size_t i = 0; i < Size; ++i, ++it) {
            *it = extract(x, i);
        }
    }
};
#define Vc_OP(op_)                                                                       \
    template <typename T0, typename V0, typename T1, typename V1>                        \
    decltype(std::declval<const V0 &>() op_ std::declval<const V1 &>()) operator op_(    \
        const Reference<T0, V0, Mutable::Yes> &x,                                        \
        const Reference<T1, V1, Mutable::Yes> &y)                                        \
    {                                                                                    \
        return static_cast<const V0 &>(x) op_ static_cast<const V1 &>(y);                \
    }
Vc_ALL_COMPARES(Vc_OP);
Vc_ALL_ARITHMETICS(Vc_OP);
Vc_ALL_BINARY(Vc_OP);
Vc_ALL_LOGICAL(Vc_OP);
Vc_ALL_SHIFTS(Vc_OP);
#undef Vc_OP

///\internal immutable specialization of the Reference proxy class
template <typename T, typename value_vector>
class Reference<T, value_vector, Mutable::No> : public value_vector
{
    static constexpr auto Size = value_vector::size();

public:
    Reference(const T &it) : value_vector(fromIterator<T, value_vector>(it)) {}

    Reference(const Reference &) = delete;
    Reference(Reference &&) = default;
    Reference &operator=(const Reference &) = delete;
    Reference &operator=(Reference &&) = delete;

    /// Explicitly disable assignment to an immutable reference.
    void operator=(const value_vector &x) = delete;
};

template <typename T, size_t N,
          IteratorDetails::Mutable M =
              (Traits::is_output_iterator<T>::value ? Mutable::Yes : Mutable::No),
          typename V = simdize<typename std::iterator_traits<T>::value_type, N>,
          size_t Size = V::Size,
          typename = typename std::iterator_traits<T>::iterator_category>
class Iterator;

template <typename T, size_t N, IteratorDetails::Mutable M, typename V, size_t Size_>
class Iterator<T, N, M, V, Size_, std::forward_iterator_tag>
{
public:
    using iterator_category = typename std::iterator_traits<T>::iterator_category;
    using difference_type = typename std::iterator_traits<T>::difference_type;
    using value_type = V;
    using pointer = IteratorDetails::Pointer<T, V, M>;
    using reference = IteratorDetails::Reference<T, V, M>;
    using const_pointer = IteratorDetails::Pointer<T, V, IteratorDetails::Mutable::No>;
    using const_reference =
        IteratorDetails::Reference<T, V, IteratorDetails::Mutable::No>;

    /// Returns the vector width the iterator covers with each step.
    static constexpr std::size_t size() { return Size_; }
    static constexpr std::size_t Size = Size_;

    Iterator() = default;

    /**
     * A vectorizing iterator is typically initialized from a scalar iterator. The
     * scalar iterator points to the first entry to place into the vectorized object.
     * Subsequent entries returned by the iterator are used to fill the rest of the
     * vectorized object.
     */
    Iterator(const T &x) : scalar_it(x) {}
    /**
     * Move optimization of the above constructor.
     */
    Iterator(T &&x) : scalar_it(std::move(x)) {}
    /**
     * Reset the vectorizing iterator to the given start point \p x.
     */
    Iterator &operator=(const T &x)
    {
        scalar_it = x;
        return *this;
    }
    /**
     * Move optimization of the above constructor.
     */
    Iterator &operator=(T &&x)
    {
        scalar_it = std::move(x);
        return *this;
    }

    /// Default copy constructor.
    Iterator(const Iterator &) = default;
    /// Default move constructor.
    Iterator(Iterator &&) = default;
    /// Default copy assignment.
    Iterator &operator=(const Iterator &) = default;
    /// Default move assignment.
    Iterator &operator=(Iterator &&) = default;

    /// Advances the iterator by one vector width, or respectively N scalar steps.
    Iterator &operator++()
    {
        std::advance(scalar_it, Size);
        return *this;
    }
    /// Postfix overload of the above.
    Iterator operator++(int)
    {
        Iterator copy(*this);
        operator++();
        return copy;
    }

    /**
     * Returns whether the two iterators point to the same scalar entry.
     *
     * \warning If the end iterator you compare against is not a multiple of the SIMD
     * width away from the incrementing iterator then the two iterators may pass each
     * other without ever comparing equal. In debug builds (when NDEBUG is not
     * defined) an assertion tries to locate such passing iterators.
     */
    bool operator==(const Iterator &rhs) const
    {
#ifndef NDEBUG
        if (scalar_it == rhs.scalar_it) {
            return true;
        } else {
            T it(scalar_it);
            for (size_t i = 1; i < Size; ++i) {
                Vc_ASSERT((++it != rhs.scalar_it));
            }
            return false;
        }
#else
        return scalar_it == rhs.scalar_it;
#endif
    }
    /**
     * Returns whether the two iterators point to different scalar entries.
     *
     * \warning If the end iterator you compare against is not a multiple of the SIMD
     * width away from the incrementing iterator then the two iterators may pass each
     * other without ever comparing equal. In debug builds (when NDEBUG is not
     * defined) an assertion tries to locate such passing iterators.
     */
    bool operator!=(const Iterator &rhs) const
    {
        return !operator==(rhs);
    }

    pointer operator->() { return scalar_it; }

    /**
     * Returns a copy of the objects behind the iterator in a vectorized type. You can use
     * the assignment operator to modify the values in the container referenced by the
     * iterator. Use of any other mutating operation is undefined behavior and will most
     * likely not be reflected in the container.
     */
    reference operator*() { return scalar_it; }

    const_pointer operator->() const { return scalar_it; }

    /**
     * Returns a copy of the objects behind the iterator in a vectorized type.
     *
     * \warning This does not behave like the standard iterator interface as it does not
     * return an lvalue reference. Thus, changes to the container the iterator references
     * will not be reflected in the reference object you receive.
     */
    const_reference operator*() const { return scalar_it; }

    /**
     * Returns a const lvalue reference to the underlying scalar iterator. This
     * effectively allows you to cast simdized iterator objects to their scalar ancestor
     * type.
     *
     * Example:
     * \code
        const auto mask = *it == value_v;
        if (any_of(mask)) {
          return static_cast<ScalarIt>(it) + mask.firstOne();
        }
     * \endcode
     */
    operator const T &() const { return scalar_it; }

protected:
    T scalar_it;
};

/**
 * This is the iterator type created when applying simdize to a bidirectional
 * iterator type.
 */
template <typename T, size_t N, IteratorDetails::Mutable M, typename V, size_t Size>
class Iterator<T, N, M, V, Size, std::bidirectional_iterator_tag>
    : public Iterator<T, N, M, V, Size, std::forward_iterator_tag>
{
    using Base = Iterator<T, N, M, V, Size, std::forward_iterator_tag>;

protected:
    using Base::scalar_it;

public:
    using pointer = typename Base::pointer;
    using reference = typename Base::reference;
    using const_pointer = typename Base::const_pointer;
    using const_reference = typename Base::const_reference;

    using Iterator<T, N, M, V, Size,
                   std::forward_iterator_tag>::Iterator;  // in short: "using
                                                          // Base::Iterator", but that
                                                          // confuses ICC
    /// Advances the iterator by one vector width, or respectively N scalar steps.
    Iterator &operator--()
    {
        std::advance(scalar_it, -Size);
        return *this;
    }
    /// Postfix overload of the above.
    Iterator operator--(int)
    {
        Iterator copy(*this);
        operator--();
        return copy;
    }
};

/**
 * This is the iterator type created when applying simdize to a random access iterator
 * type.
 */
template <typename T, size_t N, IteratorDetails::Mutable M, typename V, size_t Size>
class Iterator<T, N, M, V, Size, std::random_access_iterator_tag>
    : public Iterator<T, N, M, V, Size, std::bidirectional_iterator_tag>
{
    using Base = Iterator<T, N, M, V, Size, std::bidirectional_iterator_tag>;

protected:
    using Base::scalar_it;

public:
    using pointer = typename Base::pointer;
    using reference = typename Base::reference;
    using const_pointer = typename Base::const_pointer;
    using const_reference = typename Base::const_reference;
    using difference_type = typename std::iterator_traits<T>::difference_type;

    using Iterator<T, N, M, V, Size, std::bidirectional_iterator_tag>::
        Iterator;  // in short: "using Base::Iterator", but that confuses ICC

    Iterator &operator+=(difference_type n)
    {
        scalar_it += n * difference_type(Size);
        return *this;
    }
    Iterator operator+(difference_type n) const { return Iterator(*this) += n; }

    Iterator &operator-=(difference_type n)
    {
        scalar_it -= n * difference_type(Size);
        return *this;
    }
    Iterator operator-(difference_type n) const { return Iterator(*this) -= n; }

    difference_type operator-(const Iterator &rhs) const
    {
        constexpr difference_type n = Size;
        Vc_ASSERT((scalar_it - rhs.scalar_it) % n ==
                  0);  // if this fails the two iterators are not a multiple of the vector
                       // width apart. The distance would be fractional and that doesn't
                       // make too much sense for iteration. Therefore, it is a
                       // precondition for the distance of the two iterators to be a
                       // multiple of Size.
        return (scalar_it - rhs.scalar_it) / n;
    }

    /**
     * Returns whether all entries accessed via iterator dereferencing come before the
     * iterator \p rhs.
     */
    bool operator<(const Iterator &rhs) const
    {
        return rhs.scalar_it - scalar_it >= difference_type(Size);
    }

    bool operator>(const Iterator &rhs) const
    {
        return scalar_it - rhs.scalar_it >= difference_type(Size);
    }

    bool operator<=(const Iterator &rhs) const
    {
        return rhs.scalar_it - scalar_it >= difference_type(Size) - 1;
    }

    bool operator>=(const Iterator &rhs) const
    {
        return scalar_it - rhs.scalar_it >= difference_type(Size) - 1;
    }

    reference operator[](difference_type i) { return *(*this + i); }
    const_reference operator[](difference_type i) const { return *(*this + i); }
};

template <typename T, size_t N, IteratorDetails::Mutable M, typename V, size_t Size>
Iterator<T, N, M, V, Size, std::random_access_iterator_tag> operator+(
    typename Iterator<T, N, M, V, Size, std::random_access_iterator_tag>::difference_type
        n,
    const Iterator<T, N, M, V, Size, std::random_access_iterator_tag> &i)
{
    return i + n;
}

}  // namespace IteratorDetails }}}

/**\internal
 *
 * Creates a member type \p type that acts as a vectorizing bidirectional iterator.
 *
 * \tparam T The bidirectional iterator type to be transformed.
 * \tparam N The width the resulting vectorized type should have.
 * \tparam MT The base type to use for mask types. Ignored for this specialization.
 */
template <typename T, size_t N, typename MT>
struct ReplaceTypes<T, N, MT, Category::ForwardIterator>
{
    using type = IteratorDetails::Iterator<T, N>;
};
template <typename T, size_t N, typename MT>
struct ReplaceTypes<T, N, MT, Category::BidirectionalIterator>
{
    using type = IteratorDetails::Iterator<T, N>;
};
template <typename T, size_t N, typename MT>
struct ReplaceTypes<T, N, MT, Category::RandomAccessIterator>
{
    using type = IteratorDetails::Iterator<T, N>;
};

/**\internal
 * Implementation for conditional assignment of whole vectorized objects.
 */
template <Vc::Operator Op, typename S, typename T, std::size_t N, typename M, typename U,
          std::size_t Offset>
Vc_INTRINSIC Vc::enable_if<(Offset >= determine_tuple_size_<S>::value && M::Size == N), void>
    conditional_assign(Adapter<S, T, N> &, const M &, const U &)
{
}
template <Vc::Operator Op, typename S, typename T, std::size_t N, typename M, typename U,
          std::size_t Offset = 0>
Vc_INTRINSIC Vc::enable_if<(Offset < determine_tuple_size_<S>::value && M::Size == N), void>
    conditional_assign(Adapter<S, T, N> &lhs, const M &mask, const U &rhs)
{
    using V = typename std::decay<decltype(get_dispatcher<Offset>(lhs))>::type;
    using M2 = typename V::mask_type;
    conditional_assign<Op>(get_dispatcher<Offset>(lhs), simd_cast<M2>(mask), get_dispatcher<Offset>(rhs));
    conditional_assign<Op, S, T, N, M, U, Offset + 1>(lhs, mask, rhs);
}
template <Vc::Operator Op, typename S, typename T, std::size_t N, typename M,
          std::size_t Offset>
Vc_INTRINSIC Vc::enable_if<(Offset >= determine_tuple_size_<S>::value && M::Size == N), void>
    conditional_assign(Adapter<S, T, N> &, const M &)
{
}
template <Vc::Operator Op, typename S, typename T, std::size_t N, typename M,
          std::size_t Offset = 0>
Vc_INTRINSIC Vc::enable_if<(Offset < determine_tuple_size_<S>::value && M::Size == N), void>
    conditional_assign(Adapter<S, T, N> &lhs, const M &mask)
{
    using V = typename std::decay<decltype(get_dispatcher<Offset>(lhs))>::type;
    using M2 = typename V::mask_type;
    conditional_assign<Op>(get_dispatcher<Offset>(lhs), simd_cast<M2>(mask));
    conditional_assign<Op, S, T, N, M, Offset + 1>(lhs, mask);
}

/** @}*/
}  // namespace SimdizeDetail

// user API {{{
/*!\ingroup Simdize
 * Vectorize/Simdize the given type T.
 *
 * \tparam T This type must be a class template instance where the template arguments can
 * be recursively replaced with their vectorized variant. If the type implements a
 * specific interface for introspection and member modification, the resulting type can
 * easily be constructed from objects of type T and scalar objects of type T can be
 * extracted from it.
 *
 * \tparam N This value determines the width of the vectorization. Per default it is set
 * to 0 making the implementation choose the value considering the compilation target and
 * the given type T.
 *
 * \tparam MT This type determines the type to be used when replacing bool with Mask<MT>.
 * If it is set to void the implementation choosed the type as smart as possible.
 *
 * \see Vc_SIMDIZE_STRUCT, Vc_SIMDIZE_MEMBER
 */
template <typename T, size_t N = 0, typename MT = void>
using simdize = SimdizeDetail::simdize<T, N, MT>;

/*!\ingroup Simdize
 * Declares functions and constants for introspection by the simdize functions. This
 * allows e.g. conversion between scalar \c T and \c simdize<T>.
 *
 * \param MEMBERS_ The data members of this struct/class listed inside extra parenthesis.
 * The extra parenthesis are required because the macro would otherwise see a variable
 * number of arguments.
 *
 * Example:
 * \code
 * template <typename T, typename U> struct X {
 *   T a;
 *   U b;
 *   Vc_SIMDIZE_INTERFACE((a, b));
 * };
 * \endcode
 *
 * \note You must use this macros in the public section of a class.
 */
#define Vc_SIMDIZE_INTERFACE(MEMBERS_)                                                   \
    template <std::size_t N_>                                                            \
    inline auto vc_get_()->decltype(std::get<N_>(std::tie MEMBERS_))                     \
    {                                                                                    \
        return std::get<N_>(std::tie MEMBERS_);                                          \
    }                                                                                    \
    template <std::size_t N_>                                                            \
    inline auto vc_get_() const->decltype(std::get<N_>(std::tie MEMBERS_))               \
    {                                                                                    \
        return std::get<N_>(std::tie MEMBERS_);                                          \
    }                                                                                    \
    enum : std::size_t {                                                                 \
        tuple_size = std::tuple_size<decltype(std::make_tuple MEMBERS_)>::value          \
    }
// }}}
}  // namespace Vc

namespace std  // {{{
{
using Vc::SimdizeDetail::swap;
}  // namespace std }}}

#endif  // VC_COMMON_SIMDIZE_H_

// vim: foldmethod=marker
