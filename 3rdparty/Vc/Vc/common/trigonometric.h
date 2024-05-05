/*  This file is part of the Vc library. {{{
Copyright © 2009-2015 Matthias Kretz <kretz@kde.org>

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

#ifndef VC_COMMON_TRIGONOMETRIC_H_
#define VC_COMMON_TRIGONOMETRIC_H_

#include "macros.h"

#ifdef Vc_HAVE_LIBMVEC
extern "C" {
__m128 _ZGVbN4v_sinf(__m128);
__m128d _ZGVbN2v_sin(__m128d);
__m128 _ZGVbN4v_cosf(__m128);
__m128d _ZGVbN2v_cos(__m128d);
__m256 _ZGVdN8v_sinf(__m256);
__m256d _ZGVdN4v_sin(__m256d);
__m256 _ZGVdN8v_cosf(__m256);
__m256d _ZGVdN4v_cos(__m256d);
}
#endif

namespace Vc_VERSIONED_NAMESPACE
{
namespace Detail
{
template<Vc::Implementation Impl> struct MapImpl { enum Dummy { Value = Impl }; };
template<> struct MapImpl<Vc::SSE42Impl> { enum Dummy { Value = MapImpl<Vc::SSE41Impl>::Value }; };

template<Vc::Implementation Impl> using TrigonometricImplementation =
    ImplementationT<MapImpl<Impl>::Value
#if defined(Vc_IMPL_XOP) && defined(Vc_IMPL_FMA4)
    + Vc::XopInstructions
    + Vc::Fma4Instructions
#endif
    >;
}  // namespace Detail

namespace Common
{
template<typename Impl> struct Trigonometric
{
    template<typename T> static T Vc_VDECL sin(const T &_x);
    template<typename T> static T Vc_VDECL cos(const T &_x);
    template<typename T> static void Vc_VDECL sincos(const T &_x, T *_sin, T *_cos);
    template<typename T> static T Vc_VDECL asin (const T &_x);
    template<typename T> static T Vc_VDECL atan (const T &_x);
    template<typename T> static T Vc_VDECL atan2(const T &y, const T &x);
};
}  // namespace Common

#if defined Vc_IMPL_SSE || defined DOXYGEN
// this is either SSE, AVX, or AVX2
namespace Detail
{
template <typename T, typename Abi>
using Trig = Common::Trigonometric<Detail::TrigonometricImplementation<
    (std::is_same<Abi, VectorAbi::Sse>::value
         ? SSE42Impl
         : std::is_same<Abi, VectorAbi::Avx>::value ? AVXImpl : ScalarImpl)>>;
}  // namespace Detail

#ifdef Vc_HAVE_LIBMVEC
Vc_INTRINSIC __m128  sin_dispatch(__m128  x) { return ::_ZGVbN4v_sinf(x); }
Vc_INTRINSIC __m128d sin_dispatch(__m128d x) { return ::_ZGVbN2v_sin (x); }
Vc_INTRINSIC __m128  cos_dispatch(__m128  x) { return ::_ZGVbN4v_cosf(x); }
Vc_INTRINSIC __m128d cos_dispatch(__m128d x) { return ::_ZGVbN2v_cos (x); }
#ifdef Vc_IMPL_AVX
Vc_INTRINSIC __m256  sin_dispatch(__m256  x) { return ::_ZGVdN8v_sinf(x); }
Vc_INTRINSIC __m256d sin_dispatch(__m256d x) { return ::_ZGVdN4v_sin (x); }
Vc_INTRINSIC __m256  cos_dispatch(__m256  x) { return ::_ZGVdN8v_cosf(x); }
Vc_INTRINSIC __m256d cos_dispatch(__m256d x) { return ::_ZGVdN4v_cos (x); }
#endif

template <typename T, typename Abi>
Vc_INTRINSIC Vector<T, detail::not_fixed_size_abi<Abi>> sin(const Vector<T, Abi> &x)
{
    return sin_dispatch(x.data());
}
template <typename T, typename Abi>
Vc_INTRINSIC Vector<T, detail::not_fixed_size_abi<Abi>> cos(const Vector<T, Abi> &x)
{
    return cos_dispatch(x.data());
}
#else
/**
 * \ingroup Math
 * Returns the sine of all input values in \p x.
 *
 * \param x The values to apply the sine function on.
 *
 * \returns the sine of \p x.
 *
 * \note The single-precision implementation has a precision of max. 2 ulp (mean 0.17 ulp)
 * in the range [-8192, 8192].
 * (testSin< float_v> with a maximal distance of 2 to the reference (mean: 0.310741))
 *
 * \note The double-precision implementation has a precision of max. 3 ulp (mean 1040 ulp)
 * in the range [-8192, 8192].
 * (testSin<double_v> with a maximal distance of 1 to the reference (mean: 0.170621))
 *
 * \note The precision and execution latency depends on:
 *       - `Abi` (e.g. Scalar uses the `<cmath>` implementation
 *       - whether `Vc_HAVE_LIBMVEC` is defined
 *       - for the `<cmath>` fallback, the implementations differ (e.g. MacOS vs. Linux
 * vs. Windows; fpmath=sse vs. fpmath=387)
 *
 * \note Vc versions before 1.4 had different precision.
 */
template <typename T, typename Abi>
Vc_INTRINSIC Vector<T, detail::not_fixed_size_abi<Abi>> sin(const Vector<T, Abi> &x)
{
    return Detail::Trig<T, Abi>::sin(x);
}

/**
 * \ingroup Math
 * Returns the cosine of all input values in \p x.
 *
 * \param x The values to apply the cosine function on.
 * \returns the cosine of \p x.
 *
 * \note The single-precision implementation has a precision of max. 2 ulp (mean 0.18 ulp) in the range [-8192, 8192].
 * \note The double-precision implementation has a precision of max. 3 ulp (mean 1160 ulp) in the range [-8192, 8192].
 * \note Vc versions before 1.4 had different precision.
 */
template <typename T, typename Abi>
Vc_INTRINSIC Vector<T, detail::not_fixed_size_abi<Abi>> cos(const Vector<T, Abi> &x)
{
    return Detail::Trig<T, Abi>::cos(x);
}
#endif

/**
 * \ingroup Math
 * Returns the arcsine of all input values in \p x.
 *
 * \param x The values to apply the arcsine function on.
 * \returns the arcsine of \p x.
 *
 * \note The single-precision implementation has an error of max. 2 ulp (mean 0.3 ulp).
 * \note The double-precision implementation has an error of max. 36 ulp (mean 0.4 ulp).
 */
template <typename T, typename Abi>
Vc_INTRINSIC Vector<T, detail::not_fixed_size_abi<Abi>> asin(const Vector<T, Abi> &x)
{
    return Detail::Trig<T, Abi>::asin(x);
}

/**
 * \ingroup Math
 * Returns the arctangent of all input values in \p x.
 *
 * \param x The values to apply the arctangent function on.
 * \returns the arctangent of \p x.
 * \note The single-precision implementation has an error of max. 3 ulp (mean 0.4 ulp) in the range [-8192, 8192].
 * \note The double-precision implementation has an error of max. 2 ulp (mean 0.1 ulp) in the range [-8192, 8192].
 */
template <typename T, typename Abi>
Vc_INTRINSIC Vector<T, detail::not_fixed_size_abi<Abi>> atan(const Vector<T, Abi> &x)
{
    return Detail::Trig<T, Abi>::atan(x);
}

/**
 * \ingroup Math
 * Returns the arctangent of all input values in \p x and \p y.
 *
 * Calculates the angle given the lengths of the opposite and adjacent legs in a right
 * triangle.
 * \param y The opposite leg.
 * \param x The adjacent leg.
 * \returns the arctangent of \p y / \p x.
 */
template <typename T, typename Abi>
Vc_INTRINSIC Vector<T, detail::not_fixed_size_abi<Abi>> atan2(const Vector<T, Abi> &y,
                                                              const Vector<T, Abi> &x)
{
    return Detail::Trig<T, Abi>::atan2(y, x);
}

/**
 * \ingroup Math
 *
 * \param x Input value to both sine and cosine.
 * \param sin A non-null pointer to a potentially uninitialized object of type Vector.
 *            When \c sincos returns, `*sin` contains the result of `sin(x)`.
 * \param cos A non-null pointer to a potentially uninitialized object of type Vector.
 *            When \c sincos returns, `*cos` contains the result of `cos(x)`.
 *
 * \see sin, cos
 */
template <typename T, typename Abi>
Vc_INTRINSIC void sincos(const Vector<T, Abi> &x,
                         Vector<T, detail::not_fixed_size_abi<Abi>> *sin,
                         Vector<T, Abi> *cos)
{
    Detail::Trig<T, Abi>::sincos(x, sin, cos);
}
#endif
}  // namespace Vc_VERSIONED_NAMESPACE

#endif  // VC_COMMON_TRIGONOMETRIC_H_
