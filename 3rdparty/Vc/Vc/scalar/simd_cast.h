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

#ifndef VC_SCALAR_SIMD_CAST_H_
#define VC_SCALAR_SIMD_CAST_H_

#include "../common/simd_cast.h"
#include "type_traits.h"
#include "macros.h"

namespace Vc_VERSIONED_NAMESPACE
{

// Scalar::Vector to Scalar::Vector
template <typename To, typename From>
Vc_INTRINSIC Vc_CONST To
    simd_cast(Scalar::Vector<From> x, enable_if<Scalar::is_vector<To>::value> = nullarg)
{
    return static_cast<To>(x.data());
}

// Scalar::Mask to Scalar::Mask
template <typename To, typename From>
Vc_INTRINSIC Vc_CONST To
    simd_cast(Scalar::Mask<From> x, enable_if<Scalar::is_mask<To>::value> = nullarg)
{
    return static_cast<To>(x.data());
}

// Any vector (Vector<T> or SimdArray) to multiple Scalar::Vector<T>
template <typename Return, int offset, typename T>
Vc_INTRINSIC Vc_CONST Return simd_cast(
    T &&x,
    enable_if<Traits::is_simd_vector<T>::value && Scalar::is_vector<Return>::value> = nullarg)
{
    return Return(x[offset]);
}

template <typename Return, int offset, typename T>
Vc_INTRINSIC Vc_CONST enable_if<offset == 0 && Traits::is_simd_vector<Return>::value &&
                                    !Scalar::is_vector<Return>::value,
                                Return>
    simd_cast(Scalar::Vector<T> x)
{
    Return r{};
    r[0] = static_cast<typename Return::EntryType>(x.data());
    return r;
}


// Any mask (Mask<T> or SimdMaskArray) to multiple Scalar::Mask<T>
template <typename Return, int offset, typename T>
Vc_INTRINSIC Vc_CONST Return simd_cast(
    T &&x,
    enable_if<Traits::is_simd_mask<T>::value && Scalar::is_mask<Return>::value> = nullarg)
{
    return Return(bool(x[offset]));
}

template <typename Return, int offset, typename T>
Vc_INTRINSIC Vc_CONST enable_if<
    offset == 0 && Traits::is_simd_mask<Return>::value && !Scalar::is_mask<Return>::value,
    Return>
    simd_cast(Scalar::Mask<T> x)
{
    Return r(false);
    r[0] = x[0];
    return r;
}

}  // namespace Vc

#endif  // VC_SCALAR_SIMD_CAST_H_

// vim: foldmethod=marker
