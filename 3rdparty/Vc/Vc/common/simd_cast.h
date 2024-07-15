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

#ifndef VC_COMMON_SIMD_CAST_H_
#define VC_COMMON_SIMD_CAST_H_

#include <type_traits>
#include "macros.h"

// declare a bogus simd_cast function template in the global namespace to enable ADL for
// simd_cast<T>
template <class> void simd_cast();

namespace Vc_VERSIONED_NAMESPACE
{
/**
 * Casts the argument \p x from type \p From to type \p To.
 *
 * This function implements the trivial case where \p To and \p From are the same type.
 *
 * \param x The object of type \p From to be converted to type \p To.
 * \returns An object of type \p To with all vector components converted according to
 *          standard conversion behavior as mandated by the C++ standard for the
 *          underlying arithmetic types.
 */
template <typename To, typename From>
Vc_INTRINSIC Vc_CONST To
simd_cast(From &&x, enable_if<std::is_same<To, Traits::decay<From>>::value> = nullarg)
{
    return std::forward<From>(x);
}

/**
 * A cast from nothing results in default-initialization of \p To.
 *
 * This function can be useful in generic code where a parameter pack expands to nothing.
 *
 * \returns A zero-initialized object of type \p To.
 */
template <typename To> Vc_INTRINSIC Vc_CONST To simd_cast() { return To(); }

}  // namespace Vc

#endif // VC_COMMON_SIMD_CAST_H_
