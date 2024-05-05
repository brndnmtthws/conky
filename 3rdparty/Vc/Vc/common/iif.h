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

#ifndef VC_COMMON_IIF_H_
#define VC_COMMON_IIF_H_

#include "../type_traits"
#include "macros.h"

namespace Vc_VERSIONED_NAMESPACE
{
/**
 * \ingroup Utilities
 *
 * Function to mimic the ternary operator '?:' (inline-if).
 *
 * \param condition  Determines which values are returned. This is analog to the first argument to
 *                   the ternary operator.
 * \param trueValue  The values to return where \p condition is \c true.
 * \param falseValue The values to return where \p condition is \c false.
 * \return A combination of entries from \p trueValue and \p falseValue, according to \p condition.
 *
 * So instead of the scalar variant
 * \code
 * float x = a > 1.f ? b : b + c;
 * \endcode
 * you'd write
 * \code
 * float_v x = Vc::iif (a > 1.f, b, b + c);
 * \endcode
 *
 * Assuming \c a has the values [0, 3, 5, 1], \c b is [1, 1, 1, 1], and \c c is [1, 2, 3, 4], then x
 * will be [2, 2, 3, 5].
 */
template <typename Mask, typename T>
Vc_ALWAYS_INLINE enable_if<is_simd_mask<Mask>::value && is_simd_vector<T>::value, T> iif(
    const Mask &condition, const T &trueValue, const T &falseValue)
{
    T result(falseValue);
    Vc::where(condition) | result = trueValue;
    return result;
}

/**\internal
 * The following declaration makes it explicit that `iif (Mask, non-vector, non-vector)`
 * is not supposed to work. Doing the same thing with \c static_assert would break SFINAE.
 */
template <typename Mask, typename T>
enable_if<is_simd_mask<Mask>::value && !is_simd_vector<T>::value, T> iif(
    const Mask &, const T &, const T &) = delete;

/**
 * \ingroup Utilities
 *
 * Overload of the above for boolean conditions.
 *
 * This typically results in direct use of the ternary operator. This function makes it easier to
 * switch from a Vc type to a builtin type.
 *
 * \param condition  Determines which value is returned. This is analog to the first argument to
 *                   the ternary operator.
 * \param trueValue  The value to return if \p condition is \c true.
 * \param falseValue The value to return if \p condition is \c false.
 * \return Either \p trueValue or \p falseValue, depending on \p condition.
 */
template<typename T> constexpr T iif (bool condition, const T &trueValue, const T &falseValue)
{
    return condition ? trueValue : falseValue;
}

}  // namespace Vc

#endif // VC_COMMON_IIF_H_
