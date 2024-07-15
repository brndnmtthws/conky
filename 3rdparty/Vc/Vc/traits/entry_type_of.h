/*  This file is part of the Vc library. {{{
Copyright © 2014 Matthias Kretz <kretz@kde.org>

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

#ifndef VC_TRAITS_ENTRY_TYPE_OF_H_
#define VC_TRAITS_ENTRY_TYPE_OF_H_

namespace Vc_VERSIONED_NAMESPACE
{
namespace Traits
{
namespace entry_type_of_internal
{
template <typename T, bool = Traits::is_simd_vector<T>::value> struct entry_type;

template <typename T> struct entry_type<T, true>
{
    using type = typename decay<T>::EntryType;
};

template <typename T> struct entry_type<T, false>
{
    using type = typename std::remove_cv<typename std::remove_reference<T>::type>::type;
};
}  // namespace entry_type_of_internal

/**
 * Resolves to T::EntryType if \p T is a SIMD type, otherwise it resolves to \p T itself.
 */
template <typename T> using entry_type_of = typename entry_type_of_internal::entry_type<T>::type;

}  // namespace Traits
}  // namespace Vc_VERSIONED_NAMESPACE

#endif  // VC_TRAITS_ENTRY_TYPE_OF_H_

// vim: foldmethod=marker
