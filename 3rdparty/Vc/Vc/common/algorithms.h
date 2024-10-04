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

#ifndef VC_COMMON_ALGORITHMS_H_
#define VC_COMMON_ALGORITHMS_H_

#include "simdize.h"

namespace Vc_VERSIONED_NAMESPACE
{
#ifdef DOXYGEN
/**
 * \ingroup Utilities
 * \headerfile algorithms.h <Vc/Vc>
 *
 * Vc variant of the `std::for_each` algorithm.
 *
 * This algorithm calls \p f with one argument of type
 * `Vc::Vector<` *iterator value type* `, ` *unspecified* `>` as often as is needed to
 * iterate over the complete range from \p first to \p last.
 * It will try to use the best vector size (VectorAbi) to work on the largest chunks
 * possible.
 * To support aligned loads (and stores) and to support arbitrary range distances, the
 * algorithm may require the use of `Vc::VectorAbi` types that work on fewer elements in
 * parallel.
 *
 * The following example requires C++14 for generic lambdas. If you don't have generic
 * lambdas available you can use a "classic" functor type with a templated call operator
 * instead.
 *
 * \code
 * void scale(std::vector<double> &data, double factor) {
 *   Vc::simd_for_each(data.begin(), data.end(), [&](auto v) {
 *      v *= factor;
 *   });
 * }
 * \endcode
 */
template <class InputIt, class UnaryFunction>
UnaryFunction simd_for_each(InputIt first, InputIt last, UnaryFunction f);
#else
template <class InputIt, class UnaryFunction,
          class ValueType = typename std::iterator_traits<InputIt>::value_type>
inline enable_if<
    Traits::is_functor_argument_immutable<UnaryFunction, simdize<ValueType>>::value,
    UnaryFunction>
simd_for_each(InputIt first, InputIt last, UnaryFunction f)
{
    typedef simdize<ValueType> V;
    typedef simdize<ValueType, 1> V1;
    const auto lastV = last - V::Size + 1;
    for (; first < lastV; first += V::Size) {
        V tmp;
        load_interleaved(tmp, std::addressof(*first));
        f(tmp);
    }
    for (; first != last; ++first) {
        V1 tmp;
        load_interleaved(tmp, std::addressof(*first));
        f(tmp);
    }
    return f;
}

template <typename InputIt, typename UnaryFunction,
          class ValueType = typename std::iterator_traits<InputIt>::value_type>
inline enable_if<
        !Traits::is_functor_argument_immutable<UnaryFunction, simdize<ValueType>>::value,
    UnaryFunction>
simd_for_each(InputIt first, InputIt last, UnaryFunction f)
{
    typedef simdize<ValueType> V;
    typedef simdize<ValueType, 1> V1;
    const auto lastV = last - V::size() + 1;
    for (; first < lastV; first += V::size()) {
        V tmp;
        load_interleaved(tmp, std::addressof(*first));
        f(tmp);
        store_interleaved(tmp, std::addressof(*first));
    }
    for (; first != last; ++first) {
        V1 tmp;
        load_interleaved(tmp, std::addressof(*first));
        f(tmp);
        store_interleaved(tmp, std::addressof(*first));
    }
    return f;
}
#endif

///////////////////////////////////////////////////////////////////////////////
template <typename InputIt, typename UnaryFunction,
          class ValueType = typename std::iterator_traits<InputIt>::value_type>
inline enable_if<
    Traits::is_functor_argument_immutable<UnaryFunction, simdize<ValueType>>::value,
    UnaryFunction>
simd_for_each_n(InputIt first, std::size_t count, UnaryFunction f)
{
    typename std::make_signed<size_t>::type len = count;
    typedef simdize<ValueType> V;
    typedef simdize<ValueType, 1> V1;
    for (; len >= int(V::size()); len -= V::Size, first += V::Size) {
        V tmp;
        load_interleaved(tmp, std::addressof(*first));
        f(tmp);
    }
    for (; len != 0; --len, ++first) {
        V1 tmp;
        load_interleaved(tmp, std::addressof(*first));
        f(tmp);
    }
    return f;
}

template <typename InputIt, typename UnaryFunction,
          class ValueType = typename std::iterator_traits<InputIt>::value_type>
inline enable_if<
    !Traits::is_functor_argument_immutable<UnaryFunction, simdize<ValueType>>::value,
    UnaryFunction>
simd_for_each_n(InputIt first, std::size_t count, UnaryFunction f)
{
    typename std::make_signed<size_t>::type len = count;
    typedef simdize<ValueType> V;
    typedef simdize<ValueType, 1> V1;
    for (; len >= int(V::size()); len -= V::Size, first += V::Size) {
        V tmp;
        load_interleaved(tmp, std::addressof(*first));
        f(tmp);
        store_interleaved(tmp, std::addressof(*first));
    }
    for (; len != 0; --len, ++first) {
        V1 tmp;
        load_interleaved(tmp, std::addressof(*first));
        f(tmp);
        store_interleaved(tmp, std::addressof(*first));
    }
    return f;
}

}  // namespace Vc

#endif // VC_COMMON_ALGORITHMS_H_
