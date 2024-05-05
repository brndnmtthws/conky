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

#ifndef VC_FWDDECL_H_
#define VC_FWDDECL_H_

#include <cstddef>  // for std::size_t

#define Vc_VERSIONED_NAMESPACE Vc_1

namespace Vc_VERSIONED_NAMESPACE
{
namespace VectorAbi
{
struct Scalar {};
struct Sse {};
struct Avx {};
struct Mic {};
template <class T> struct DeduceCompatible;
template <class T> struct DeduceBest;
}  // namespace VectorAbi

namespace Common
{
template <class T, std::size_t N> struct select_best_vector_type;
}  // namespace Common

template <class T, class Abi> class Mask;
template <class T, class Abi> class Vector;

// === having SimdArray<T, N> in the Vc namespace leads to a ABI bug ===
//
// SimdArray<double, 4> can be { double[4] }, { __m128d[2] }, or { __m256d } even though the type
// is the same.
// The question is, what should SimdArray focus on?
// a) A type that makes interfacing between different implementations possible?
// b) Or a type that makes fixed size SIMD easier and efficient?
//
// a) can be achieved by using a union with T[N] as one member. But this may have more serious
// performance implications than only less efficient parameter passing (because compilers have a
// much harder time wrt. aliasing issues). Also alignment would need to be set to the sizeof in
// order to be compatible with targets with larger alignment requirements.
// But, the in-memory representation of masks is not portable. Thus, at the latest with AVX-512,
// there would be a problem with requiring SimdMaskArray<T, N> to be an ABI compatible type.
// AVX-512 uses one bit per boolean, whereas SSE/AVX use sizeof(T) Bytes per boolean. Conversion
// between the two representations is not a trivial operation. Therefore choosing one or the other
// representation will have a considerable impact for the targets that do not use this
// representation. Since the future probably belongs to one bit per boolean representation, I would
// go with that choice.
//
// b) requires that SimdArray<T, N> != SimdArray<T, N> if
// SimdArray<T, N>::vector_type != SimdArray<T, N>::vector_type
//
// Therefore use SimdArray<T, N, V>, where V follows from the above.
template <class T, std::size_t N,
          class V = typename Common::select_best_vector_type<T, N>::type,
          std::size_t Wt = V::Size>
class SimdArray;
template <class T, std::size_t N,
          class V = typename Common::select_best_vector_type<T, N>::type,
          std::size_t Wt = V::Size>
class SimdMaskArray;

namespace simd_abi
{
using scalar = VectorAbi::Scalar;
template <int N> struct fixed_size;
template <class T> using compatible = typename VectorAbi::DeduceCompatible<T>::type;
template <class T> using native = typename VectorAbi::DeduceBest<T>::type;
using __sse = VectorAbi::Sse;
using __avx = VectorAbi::Avx;
struct __avx512;
struct __neon;
}  // namespace simd_abi

template <class T, class Abi = simd_abi::compatible<T>> using simd = Vector<T, Abi>;
template <class T, class Abi = simd_abi::compatible<T>> using simd_mask = Mask<T, Abi>;
template <class T> using native_simd = simd<T, simd_abi::native<T>>;
template <class T> using native_simd_mask = simd_mask<T, simd_abi::native<T>>;
template <class T, int N> using fixed_size_simd = simd<T, simd_abi::fixed_size<N>>;
template <class T, int N>
using fixed_size_simd_mask = simd_mask<T, simd_abi::fixed_size<N>>;

}  // namespace Vc_VERSIONED_NAMESPACE

#ifndef DOXYGEN
// doxygen has Vc_VERSIONED_NAMESPACE predefined to Vc
namespace Vc = Vc_VERSIONED_NAMESPACE;
#endif  // DOXYGEN

#endif  // VC_FWDDECL_H_

// vim: foldmethod=marker
