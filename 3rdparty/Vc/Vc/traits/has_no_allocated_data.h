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
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

}}}*/

#ifndef VC_TRAITS_HAS_NO_ALLOCATED_DATA_H_
#define VC_TRAITS_HAS_NO_ALLOCATED_DATA_H_

#include <array>

namespace Vc_VERSIONED_NAMESPACE
{
namespace Traits
{

/**
 * Implements the has_no_allocated_data trait.
 *
 * Specialize this type for your container class if you need to make it usable with SIMD
 * gathers/scatters. Example:
 * \code
 * namespace Vc
 * {
 * namespace Traits
 * {
 * template<typename T> struct has_no_allocated_data_impl<MyContainer<T>> : public std::true_type {};
 * }
 * }
 * \endcode
 *
 * \see has_no_allocated_data
 */
template<typename T> struct has_no_allocated_data_impl : public std::false_type {};

/**
 * Type trait that tells whether a container stores its data inside the object or inside allocated
 * memory outside of the object.
 *
 * Per default the trait assumes any type to store its data outside, on the heap. The only types
 * where it knows that the storage is inside the object are std::array, Vc::array, and T[] (builtin
 * arrays).
 *
 * The trait forwards the actual decision to has_no_allocated_data_impl, but removes const/volatile
 * and references from the type \p T to make the number of required specializations of
 * has_no_allocated_data_impl minimal.
 */
template <typename T>
struct has_no_allocated_data
    : public has_no_allocated_data_impl<
          typename std::remove_cv<typename std::remove_reference<T>::type>::type>
{
};

// spezializations:
template<typename T, std::size_t N> struct has_no_allocated_data_impl<std::array<T, N>> : public std::true_type {};
template<typename T, std::size_t N> struct has_no_allocated_data_impl<T[N]> : public std::true_type {};
template<typename T> struct has_no_allocated_data_impl<T[]> : public std::true_type {};
}  // namespace Traits
}  // namespace Vc

#endif // VC_TRAITS_HAS_NO_ALLOCATED_DATA_H_
