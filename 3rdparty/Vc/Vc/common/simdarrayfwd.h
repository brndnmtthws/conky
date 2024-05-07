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

#ifndef VC_COMMON_SIMDARRAYFWD_H_
#define VC_COMMON_SIMDARRAYFWD_H_

#include "../scalar/types.h"
#include "../sse/types.h"
#include "../avx/types.h"

#include "utility.h"
#include "macros.h"

namespace Vc_VERSIONED_NAMESPACE
{
// specialization of Vector for fixed_size<N> {{{
template <class T, int N>
class Vector<T, simd_abi::fixed_size<N>> : public SimdArray<T, N>
{
    using SimdArray<T, N>::SimdArray;

public:
    // overload copy to force argument passing via the stack. This makes the type more
    // usable on ABI boundaries
    Vc_INTRINSIC Vector(const Vector &x) : SimdArray<T, N>(x) {}
    Vc_INTRINSIC Vector &operator=(const Vector &x)
    {
        SimdArray<T, N>::operator=(x);
        return *this;
    }
    Vector() = default;

    using abi_type = simd_abi::fixed_size<N>;
    using abi = abi_type;

    Vc_DEPRECATED("use Vector([](int n) { return n; }) instead of "
                  "Vector::IndexesFromZero()") static Vector IndexesFromZero()
    {
        return Vector([](size_t i) -> T { return i; });
    }
    Vc_DEPRECATED("use 0 instead of Vector::Zero()") static Vector Zero() { return 0; }
    Vc_DEPRECATED("use 1 instead of Vector::One()") static Vector One() { return 1; }
};

template <class T, int N>
class Mask<T, simd_abi::fixed_size<N>> : public SimdMaskArray<T, N>
{
    using SimdMaskArray<T, N>::SimdMaskArray;

public:
    // overload copy to force argument passing via the stack. This makes the type more
    // usable on ABI boundaries
    Vc_INTRINSIC Mask(const Mask &x) : SimdMaskArray<T, N>(x) {}
    Vc_INTRINSIC Mask &operator=(const Mask &x)
    {
        SimdMaskArray<T, N>::operator=(x);
        return *this;
    }
    Mask() = default;

    using abi_type = simd_abi::fixed_size<N>;
    using abi = abi_type;
};
// }}}

/** \internal
 * Simple traits for SimdArray to easily access internal types of non-atomic SimdArray
 * types.
 */
template <typename T, std::size_t N> struct SimdArrayTraits {
    static constexpr std::size_t N0 = Common::left_size<N>();
    static constexpr std::size_t N1 = Common::right_size<N>();

    using storage_type0 = fixed_size_simd<T, N0>;
    using storage_type1 = fixed_size_simd<T, N1>;
};

template <typename T, std::size_t N, typename VectorType, std::size_t VectorSize>
Vc_INTRINSIC_L typename SimdArrayTraits<T, N>::storage_type0 &internal_data0(
    SimdArray<T, N, VectorType, VectorSize> &x) Vc_INTRINSIC_R;
template <typename T, std::size_t N, typename VectorType, std::size_t VectorSize>
Vc_INTRINSIC_L typename SimdArrayTraits<T, N>::storage_type1 &internal_data1(
    SimdArray<T, N, VectorType, VectorSize> &x) Vc_INTRINSIC_R;
template <typename T, std::size_t N, typename VectorType, std::size_t VectorSize>
Vc_INTRINSIC_L const typename SimdArrayTraits<T, N>::storage_type0 &internal_data0(
    const SimdArray<T, N, VectorType, VectorSize> &x) Vc_INTRINSIC_R;
template <typename T, std::size_t N, typename VectorType, std::size_t VectorSize>
Vc_INTRINSIC_L const typename SimdArrayTraits<T, N>::storage_type1 &internal_data1(
    const SimdArray<T, N, VectorType, VectorSize> &x) Vc_INTRINSIC_R;

template <typename T, std::size_t N, typename V>
Vc_INTRINSIC_L V &internal_data(SimdArray<T, N, V, N> &x) Vc_INTRINSIC_R;
template <typename T, std::size_t N, typename V>
Vc_INTRINSIC_L const V &internal_data(const SimdArray<T, N, V, N> &x) Vc_INTRINSIC_R;

namespace Traits
{
// is_fixed_size_simd {{{1
template <class T> struct is_fixed_size_simd : std::false_type {
};
template <class T, int N>
struct is_fixed_size_simd<fixed_size_simd<T, N>> : std::true_type {
};
template <class T, int N>
struct is_fixed_size_simd<fixed_size_simd_mask<T, N>> : std::true_type {
};

// is_simd_vector_internal {{{1
template <class T, int N>
struct is_simd_vector_internal<fixed_size_simd<T, N>> : is_valid_vector_argument<T> {};

// is_simd_mask_internal {{{1
template <class T, int N>
struct is_simd_mask_internal<fixed_size_simd_mask<T, N>> : is_valid_vector_argument<T> {};

// is_atomic_simdarray_internal {{{1
template <typename T, std::size_t N, typename V>
struct is_atomic_simdarray_internal<SimdArray<T, N, V, N>> : is_valid_vector_argument<T> {};
template <typename T, int N>
struct is_atomic_simdarray_internal<fixed_size_simd<T, N>>
    : is_atomic_simdarray_internal<SimdArray<T, N>> {
};

// is_atomic_simd_mask_array_internal {{{1
template <typename T, std::size_t N, typename V>
struct is_atomic_simd_mask_array_internal<SimdMaskArray<T, N, V, N>>
    : is_valid_vector_argument<T> {
};
template <typename T, int N>
struct is_atomic_simd_mask_array_internal<fixed_size_simd_mask<T, N>>
    : is_atomic_simd_mask_array_internal<SimdMaskArray<T, N>> {
};

// is_simdarray_internal {{{1
template <typename T, std::size_t N, typename VectorType, std::size_t M>
struct is_simdarray_internal<SimdArray<T, N, VectorType, M>>
    : is_valid_vector_argument<T> {
};
template <typename T, int N>
struct is_simdarray_internal<fixed_size_simd<T, N>> : is_valid_vector_argument<T> {
};

// is_simd_mask_array_internal {{{1
template <typename T, std::size_t N, typename VectorType, std::size_t M>
struct is_simd_mask_array_internal<SimdMaskArray<T, N, VectorType, M>>
    : is_valid_vector_argument<T> {
};
template <typename T, int N>
struct is_simd_mask_array_internal<fixed_size_simd_mask<T, N>>
    : is_valid_vector_argument<T> {
};

// is_integral_internal {{{1
template <typename T, std::size_t N, typename V, std::size_t M>
struct is_integral_internal<SimdArray<T, N, V, M>, false> : std::is_integral<T> {
};

// is_floating_point_internal {{{1
template <typename T, std::size_t N, typename V, std::size_t M>
struct is_floating_point_internal<SimdArray<T, N, V, M>, false>
    : std::is_floating_point<T> {
};

// is_signed_internal {{{1
template <typename T, std::size_t N, typename V, std::size_t M>
struct is_signed_internal<SimdArray<T, N, V, M>, false> : std::is_signed<T> {
};

// is_unsigned_internal {{{1
template <typename T, std::size_t N, typename V, std::size_t M>
struct is_unsigned_internal<SimdArray<T, N, V, M>, false> : std::is_unsigned<T> {
};

// has_no_allocated_data_impl {{{1
template <typename T, std::size_t N>
struct has_no_allocated_data_impl<Vc::SimdArray<T, N>> : std::true_type {
};

// }}}1
}  // namespace Traits

}  // namespace Vc

#endif  // VC_COMMON_SIMDARRAYFWD_H_

// vim: foldmethod=marker
