/*  This file is part of the Vc library. {{{
Copyright © 2018 Matthias Kretz <kretz@kde.org>

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

#ifndef VC_COMMON_DETAIL_H_
#define VC_COMMON_DETAIL_H_

#include <vector>

#include "macros.h"

namespace Vc_VERSIONED_NAMESPACE
{
namespace Common
{
// convertIndexVector {{{
// if the argument is a Vector<T> already we definitely want to keep it that way
template <typename IV>
Vc_INTRINSIC enable_if<(Traits::is_simd_vector<IV>::value &&
                        sizeof(typename IV::EntryType) >= sizeof(int)),
                       const IV &>
convertIndexVector(const IV &indexVector)
{
    return indexVector;
}

// but if the scalar (integral) type is smaller than int we convert it up to int. Otherwise it's
// very likely that the calculations we have to perform will overflow.
template <typename IV>
Vc_INTRINSIC enable_if<(Traits::is_simd_vector<IV>::value &&
                        sizeof(typename IV::EntryType) < sizeof(int)),
                       fixed_size_simd<int, IV::Size>>
convertIndexVector(const IV &indexVector)
{
    return static_cast<fixed_size_simd<int, IV::Size>>(indexVector);
}

// helper for promoting int types to int or higher
template <class T> using promoted_type = decltype(std::declval<T>() + 1);

// std::array, Vc::array, and C-array are fixed size and can therefore be converted to a
// fixed_size_simd of the same size
template <typename T, std::size_t N>
Vc_INTRINSIC enable_if<std::is_integral<T>::value, fixed_size_simd<promoted_type<T>, N>>
convertIndexVector(const std::array<T, N> &indexVector)
{
    return fixed_size_simd<promoted_type<T>, N>{std::addressof(indexVector[0]),
                                                Vc::Unaligned};
}
template <typename T, std::size_t N>
Vc_INTRINSIC enable_if<std::is_integral<T>::value, fixed_size_simd<promoted_type<T>, N>>
convertIndexVector(const Vc::array<T, N> &indexVector)
{
    return fixed_size_simd<promoted_type<T>, N>{std::addressof(indexVector[0]),
                                                Vc::Unaligned};
}
template <typename T, std::size_t N>
Vc_INTRINSIC enable_if<std::is_integral<T>::value, fixed_size_simd<promoted_type<T>, N>>
convertIndexVector(const T (&indexVector)[N])
{
    return fixed_size_simd<promoted_type<T>, N>{std::addressof(indexVector[0]),
                                                Vc::Unaligned};
}

// a plain pointer won't work. Because we need some information on the number of values in
// the index argument
#ifndef Vc_MSVC
// MSVC treats the function as usable in SFINAE context if it is deleted. If it's not declared we
// seem to get what we wanted (except for bad diagnostics)
template <class T>
enable_if<std::is_pointer<T>::value, void> convertIndexVector(T indexVector) = delete;
#endif

// an initializer_list works, but is runtime-sized (before C++14, at least) so we have to
// fall back to std::vector
template <typename T>
Vc_INTRINSIC std::vector<promoted_type<T>> convertIndexVector(
    const std::initializer_list<T> &indexVector)
{
    return {begin(indexVector), end(indexVector)};
}

// a std::vector cannot be converted to anything better
template <typename T>
Vc_INTRINSIC
    enable_if<(std::is_integral<T>::value && sizeof(T) >= sizeof(int)), std::vector<T>>
    convertIndexVector(const std::vector<T> &indexVector)
{
    return indexVector;
}
template <typename T>
Vc_INTRINSIC enable_if<(std::is_integral<T>::value && sizeof(T) < sizeof(int)),
                       std::vector<promoted_type<T>>>
convertIndexVector(const std::vector<T> &indexVector)
{
    return {std::begin(indexVector), std::end(indexVector)};
}

template <class T,
          class = enable_if<
              (!std::is_pointer<T>::value && !Traits::is_simd_vector<T>::value &&
               !std::is_lvalue_reference<decltype(std::declval<const T &>()[0])>::value)>>
Vc_INTRINSIC const T &convertIndexVector(const T &i)
{
    return i;
}

// }}}
}  // namespace Common
}  // namespace Vc_VERSIONED_NAMESPACE

#endif  // VC_COMMON_DETAIL_H_

// vim: foldmethod=marker
