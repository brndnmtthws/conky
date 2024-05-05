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

#ifndef VC_SCALAR_MATH_H_
#define VC_SCALAR_MATH_H_

#include <cstdlib>
#include "macros.h"

namespace Vc_VERSIONED_NAMESPACE
{
// copysign {{{1
Vc_INTRINSIC Scalar::float_v copysign(Scalar::float_v mag, Scalar::float_v sign)
{
    union {
        float f;
        unsigned int i;
    } value, s;
    value.f = mag.data();
    s.f = sign.data();
    value.i = (s.i & 0x80000000u) | (value.i & 0x7fffffffu);
    return Scalar::float_v{value.f};
}
Vc_INTRINSIC Vc_CONST Scalar::double_v copysign(Scalar::double_v mag,
                                                Scalar::double_v sign)
{
    union {
        double f;
        unsigned long long i;
    } value, s;
    value.f = mag.data();
    s.f = sign.data();
    value.i = (s.i & 0x8000000000000000ull) | (value.i & 0x7fffffffffffffffull);
    return Scalar::double_v{value.f};
}

// }}}1

#define Vc_MINMAX(V)                                                                     \
    static Vc_ALWAYS_INLINE Scalar::V min(const Scalar::V &x, const Scalar::V &y)        \
    {                                                                                    \
        return Scalar::V(std::min(x.data(), y.data()));                                  \
    }                                                                                    \
    static Vc_ALWAYS_INLINE Scalar::V max(const Scalar::V &x, const Scalar::V &y)        \
    {                                                                                    \
        return Scalar::V(std::max(x.data(), y.data()));                                  \
    }
Vc_ALL_VECTOR_TYPES(Vc_MINMAX);
#undef Vc_MINMAX

template<typename T> static Vc_ALWAYS_INLINE Scalar::Vector<T> sqrt (const Scalar::Vector<T> &x)
{
    return Scalar::Vector<T>(std::sqrt(x.data()));
}

template<typename T> static Vc_ALWAYS_INLINE Scalar::Vector<T> rsqrt(const Scalar::Vector<T> &x)
{
    const typename Vector<T, VectorAbi::Scalar>::EntryType one = 1; return Scalar::Vector<T>(one / std::sqrt(x.data()));
}

template <typename T,
          typename = enable_if<std::is_same<T, double>::value || std::is_same<T, float>::value ||
                               std::is_same<T, int>::value>>
Vc_ALWAYS_INLINE Vc_PURE Scalar::Vector<T> abs(Scalar::Vector<T> x)
{
    return std::abs(x.data());
}

Vc_ALWAYS_INLINE Vc_PURE Scalar::Vector<short> abs(Scalar::Vector<short> x)
{
    return std::abs(static_cast<int>(x.data()));
}

template<typename T> static Vc_ALWAYS_INLINE void sincos(const Scalar::Vector<T> &x, Scalar::Vector<T> *sin, Scalar::Vector<T> *cos)
{
#if defined(_WIN32) || defined(__APPLE__)
    sin->data() = std::sin(x.data());
    cos->data() = std::cos(x.data());
#elif Vc_HAS_BUILTIN(__builtin_sincosf) || defined Vc_GCC
    __builtin_sincosf(x.data(), &sin->data(), &cos->data());
#else
    sincosf(x.data(), &sin->data(), &cos->data());
#endif
}

template<> Vc_ALWAYS_INLINE void sincos(const Scalar::Vector<double> &x, Scalar::Vector<double> *sin, Scalar::Vector<double> *cos)
{
#if defined(_WIN32) || defined(__APPLE__)
    sin->data() = std::sin(x.data());
    cos->data() = std::cos(x.data());
#elif Vc_HAS_BUILTIN(__builtin_sincos) || defined Vc_GCC
    __builtin_sincos(x.data(), &sin->data(), &cos->data());
#else
    ::sincos(x.data(), &sin->data(), &cos->data());
#endif
}

template<typename T> static Vc_ALWAYS_INLINE Scalar::Vector<T> sin  (const Scalar::Vector<T> &x)
{
    return Scalar::Vector<T>(std::sin(x.data()));
}

template<typename T> static Vc_ALWAYS_INLINE Scalar::Vector<T> asin (const Scalar::Vector<T> &x)
{
    return Scalar::Vector<T>(std::asin(x.data()));
}

template<typename T> static Vc_ALWAYS_INLINE Scalar::Vector<T> cos  (const Scalar::Vector<T> &x)
{
    return Scalar::Vector<T>(std::cos(x.data()));
}

template<typename T> static Vc_ALWAYS_INLINE Scalar::Vector<T> log  (const Scalar::Vector<T> &x)
{
    return Scalar::Vector<T>(std::log(x.data()));
}

template<typename T> static Vc_ALWAYS_INLINE Scalar::Vector<T> log10(const Scalar::Vector<T> &x)
{
    return Scalar::Vector<T>(std::log10(x.data()));
}

template<typename T> static Vc_ALWAYS_INLINE Scalar::Vector<T> log2(const Scalar::Vector<T> &x)
{
    return Scalar::Vector<T>(std::log2(x.data()));
}

template<typename T> static Vc_ALWAYS_INLINE Scalar::Vector<T> exp (const Scalar::Vector<T> &x)
{
    return Scalar::Vector<T>(std::exp(x.data()));
}

template<typename T> static Vc_ALWAYS_INLINE Scalar::Vector<T> atan (const Scalar::Vector<T> &x)
{
    return Scalar::Vector<T>(std::atan( x.data() ));
}

template<typename T> static Vc_ALWAYS_INLINE Scalar::Vector<T> atan2(const Scalar::Vector<T> &x, const Scalar::Vector<T> &y)
{
    return Scalar::Vector<T>(std::atan2( x.data(), y.data() ));
}

template<typename T> static Vc_ALWAYS_INLINE Scalar::Vector<T> trunc(const Scalar::Vector<T> &x)
{
    return std::trunc(x.data());
}

template<typename T> static Vc_ALWAYS_INLINE Scalar::Vector<T> floor(const Scalar::Vector<T> &x)
{
    return Scalar::Vector<T>(std::floor(x.data()));
}

template<typename T> static Vc_ALWAYS_INLINE Scalar::Vector<T> ceil(const Scalar::Vector<T> &x)
{
    return Scalar::Vector<T>(std::ceil(x.data()));
}

template<typename T> static Vc_ALWAYS_INLINE Scalar::Vector<T> round(const Scalar::Vector<T> &x)
{
    return x;
}

namespace
{
    template<typename T> bool _realIsEvenHalf(T x) {
        const T two = 2;
        const T half = 0.5;
        const T f = std::floor(x * half) * two;
        return (x - f) == half;
    }
} // namespace
template<> Vc_ALWAYS_INLINE Scalar::Vector<float>  round(const Scalar::Vector<float>  &x)
{
    return Scalar::float_v(std::floor(x.data() + 0.5f) - (_realIsEvenHalf(x.data()) ? 1.f : 0.f));
}

template<> Vc_ALWAYS_INLINE Scalar::Vector<double> round(const Scalar::Vector<double> &x)
{
    return Scalar::double_v(std::floor(x.data() + 0.5 ) - (_realIsEvenHalf(x.data()) ? 1.  : 0. ));
}

template<typename T> static Vc_ALWAYS_INLINE Scalar::Vector<T> reciprocal(const Scalar::Vector<T> &x)
{
    const typename Vector<T, VectorAbi::Scalar>::EntryType one = 1; return Scalar::Vector<T>(one / x.data());
}

#ifdef isfinite
#undef isfinite
#endif
#ifdef isnan
#undef isnan
#endif
template<typename T> static Vc_ALWAYS_INLINE typename Vector<T, VectorAbi::Scalar>::Mask isfinite(const Scalar::Vector<T> &x)
{
    return typename Vector<T, VectorAbi::Scalar>::Mask(
#ifdef _MSC_VER
            !!_finite(x.data())
#elif defined(__INTEL_COMPILER) && __INTEL_COMPILER < 1500
            ::isfinite(x.data())
#else
            std::isfinite(x.data())
#endif
            );
}

template<typename T> Vc_ALWAYS_INLINE typename Vector<T, VectorAbi::Scalar>::Mask isinf(const Scalar::Vector<T> &x)
{
    return typename Vector<T, VectorAbi::Scalar>::Mask(std::isinf(x.data()));
}

template<typename T> static Vc_ALWAYS_INLINE typename Vector<T, VectorAbi::Scalar>::Mask isnan(const Scalar::Vector<T> &x)
{
    return typename Vector<T, VectorAbi::Scalar>::Mask(
#ifdef _MSC_VER
            !!_isnan(x.data())
#elif defined(__INTEL_COMPILER) && __INTEL_COMPILER < 1500
            ::isnan(x.data())
#else
            std::isnan(x.data())
#endif
            );
}

Vc_ALWAYS_INLINE Scalar::Vector<float> frexp(Scalar::Vector<float> x, SimdArray<int, 1, Scalar::Vector<int>, 1> *e) {
    return Scalar::float_v(std::frexp(x.data(), &internal_data(*e).data()));
}
Vc_ALWAYS_INLINE Scalar::Vector<double> frexp(Scalar::Vector<double> x, SimdArray<int, 1, Scalar::Vector<int>, 1> *e) {
    return Scalar::double_v(std::frexp(x.data(), &internal_data(*e).data()));
}

Vc_ALWAYS_INLINE Scalar::Vector<float> ldexp(Scalar::Vector<float> x, const SimdArray<int, 1, Scalar::Vector<int>, 1> &e) {
    return Scalar::float_v(std::ldexp(x.data(), internal_data(e).data()));
}
Vc_ALWAYS_INLINE Scalar::Vector<double> ldexp(Scalar::Vector<double> x, const SimdArray<int, 1, Scalar::Vector<int>, 1> &e) {
    return Scalar::double_v(std::ldexp(x.data(), internal_data(e).data()));
}

// fma {{{1
template <typename T>
Vc_ALWAYS_INLINE Vector<T, VectorAbi::Scalar> fma(Vector<T, VectorAbi::Scalar> a,
                                                  Vector<T, VectorAbi::Scalar> b,
                                                  Vector<T, VectorAbi::Scalar> c)
{
    if (std::is_integral<T>::value) {
        return a * b + c;
    } else {
        return std::fma(a.data(), b.data(), c.data());
    }
}

// }}}1
}  // namespace Vc

#endif // VC_SCALAR_MATH_H_

// vim: foldmethod=marker
