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

#ifndef VC_COMMON_MASKENTRY_H_
#define VC_COMMON_MASKENTRY_H_

#include "macros.h"

namespace Vc_VERSIONED_NAMESPACE
{
namespace Common
{

namespace
{
    template<size_t Bytes> struct MaskBoolStorage;
    // the following for typedefs must use std::intN_t and NOT! Vc::intN_t. The latter
    // segfaults ICC 15.0.3.
    template<> struct MaskBoolStorage<1> { typedef std::int8_t  type; };
    template<> struct MaskBoolStorage<2> { typedef std::int16_t type; };
    template<> struct MaskBoolStorage<4> { typedef std::int32_t type; };
    template<> struct MaskBoolStorage<8> { typedef std::int64_t type; };
} // anonymous namespace

template<size_t Bytes> class MaskBool
{
    typedef typename MaskBoolStorage<Bytes>::type storage_type Vc_MAY_ALIAS;
    storage_type data;
public:
    constexpr MaskBool(bool x) noexcept : data(x ? -1 : 0) {}
    Vc_ALWAYS_INLINE MaskBool &operator=(bool x) noexcept { data = x ? -1 : 0; return *this; }
    template <typename T, typename = enable_if<(!std::is_same<T, bool>::value &&
                                                std::is_fundamental<T>::value)>>
    Vc_ALWAYS_INLINE MaskBool &operator=(T x) noexcept
    {
        data = reinterpret_cast<const storage_type &>(x);
        return *this;
    }

    Vc_ALWAYS_INLINE MaskBool(const MaskBool &) noexcept = default;
    Vc_ALWAYS_INLINE MaskBool &operator=(const MaskBool &) noexcept = default;

    template <typename T, typename = enable_if<(std::is_same<T, bool>::value ||
                                                (std::is_fundamental<T>::value &&
                                                 sizeof(storage_type) == sizeof(T)))>>
    constexpr operator T() const noexcept
    {
        return std::is_same<T, bool>::value ? T((data & 1) != 0) : aliasing_cast<T>(data);
    }
} Vc_MAY_ALIAS;

template <typename A,
          typename B,
          typename std::enable_if<
              std::is_convertible<A, bool>::value &&std::is_convertible<B, bool>::value,
              int>::type = 0>
constexpr bool operator==(A &&a, B &&b)
{
    return static_cast<bool>(a) == static_cast<bool>(b);
}
template <typename A,
          typename B,
          typename std::enable_if<
              std::is_convertible<A, bool>::value &&std::is_convertible<B, bool>::value,
              int>::type = 0>
constexpr bool operator!=(A &&a, B &&b)
{
    return static_cast<bool>(a) != static_cast<bool>(b);
}

}  // namespace Common
}  // namespace Vc

#endif // VC_COMMON_MASKENTRY_H_
