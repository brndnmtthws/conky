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

#ifndef VC_COMMON_MATH_H_
#define VC_COMMON_MATH_H_

#define Vc_COMMON_MATH_H_INTERNAL 1

#include "trigonometric.h"

#include "const.h"
#include "macros.h"

namespace Vc_VERSIONED_NAMESPACE
{
// TODO, not vectorized:
template <class T, class Abi>
SimdArray<int, Vector<T, Abi>::size()> fpclassify(const Vector<T, Abi> &x)
{
    return SimdArray<int, Vector<T, Abi>::size()>(
        [&](std::size_t i) { return std::fpclassify(x[i]); });
}
template <class T, size_t N> SimdArray<int, N> fpclassify(const SimdArray<T, N> &x)
{
    return SimdArray<int, N>([&](std::size_t i) { return std::fpclassify(x[i]); });
}

#ifdef Vc_IMPL_SSE
// for SSE, AVX, and AVX2
#include "logarithm.h"
#include "exponential.h"
#ifdef Vc_IMPL_AVX
inline AVX::double_v exp(AVX::double_v _x)
{
    AVX::Vector<double> x = _x;
    typedef AVX::Vector<double> V;
        typedef V::Mask M;
    typedef AVX::Const<double> C;

        const M overflow  = x > Vc::Detail::doubleConstant< 1, 0x0006232bdd7abcd2ull, 9>(); // max log
        const M underflow = x < Vc::Detail::doubleConstant<-1, 0x0006232bdd7abcd2ull, 9>(); // min log

        V px = floor(C::log2_e() * x + 0.5);
        __m128i tmp = _mm256_cvttpd_epi32(px.data());
    const SimdArray<int, V::Size> n = SSE::int_v{tmp};
        x -= px * C::ln2_large(); //Vc::Detail::doubleConstant<1, 0x00062e4000000000ull, -1>();  // ln2
        x -= px * C::ln2_small(); //Vc::Detail::doubleConstant<1, 0x0007f7d1cf79abcaull, -20>(); // ln2

        const double P[] = {
            Vc::Detail::doubleConstant<1, 0x000089cdd5e44be8ull, -13>(),
            Vc::Detail::doubleConstant<1, 0x000f06d10cca2c7eull,  -6>(),
            Vc::Detail::doubleConstant<1, 0x0000000000000000ull,   0>()
        };
        const double Q[] = {
            Vc::Detail::doubleConstant<1, 0x00092eb6bc365fa0ull, -19>(),
            Vc::Detail::doubleConstant<1, 0x0004ae39b508b6c0ull,  -9>(),
            Vc::Detail::doubleConstant<1, 0x000d17099887e074ull,  -3>(),
            Vc::Detail::doubleConstant<1, 0x0000000000000000ull,   1>()
        };
        const V x2 = x * x;
        px = x * ((P[0] * x2 + P[1]) * x2 + P[2]);
        x =  px / ((((Q[0] * x2 + Q[1]) * x2 + Q[2]) * x2 + Q[3]) - px);
        x = V::One() + 2.0 * x;

        x = ldexp(x, n); // == x * 2ⁿ

        x(overflow) = std::numeric_limits<double>::infinity();
        x.setZero(underflow);

        return x;
    }
#endif  // Vc_IMPL_AVX

inline SSE::double_v exp(SSE::double_v::AsArg _x) {
    SSE::Vector<double> x = _x;
    typedef SSE::Vector<double> V;
        typedef V::Mask M;
    typedef SSE::Const<double> C;

        const M overflow  = x > Vc::Detail::doubleConstant< 1, 0x0006232bdd7abcd2ull, 9>(); // max log
        const M underflow = x < Vc::Detail::doubleConstant<-1, 0x0006232bdd7abcd2ull, 9>(); // min log

        V px = floor(C::log2_e() * x + 0.5);
    SimdArray<int, V::Size> n;
        _mm_storel_epi64(reinterpret_cast<__m128i *>(&n), _mm_cvttpd_epi32(px.data()));
        x -= px * C::ln2_large(); //Vc::Detail::doubleConstant<1, 0x00062e4000000000ull, -1>();  // ln2
        x -= px * C::ln2_small(); //Vc::Detail::doubleConstant<1, 0x0007f7d1cf79abcaull, -20>(); // ln2

        const double P[] = {
            Vc::Detail::doubleConstant<1, 0x000089cdd5e44be8ull, -13>(),
            Vc::Detail::doubleConstant<1, 0x000f06d10cca2c7eull,  -6>(),
            Vc::Detail::doubleConstant<1, 0x0000000000000000ull,   0>()
        };
        const double Q[] = {
            Vc::Detail::doubleConstant<1, 0x00092eb6bc365fa0ull, -19>(),
            Vc::Detail::doubleConstant<1, 0x0004ae39b508b6c0ull,  -9>(),
            Vc::Detail::doubleConstant<1, 0x000d17099887e074ull,  -3>(),
            Vc::Detail::doubleConstant<1, 0x0000000000000000ull,   1>()
        };
        const V x2 = x * x;
        px = x * ((P[0] * x2 + P[1]) * x2 + P[2]);
        x =  px / ((((Q[0] * x2 + Q[1]) * x2 + Q[2]) * x2 + Q[3]) - px);
        x = V::One() + 2.0 * x;

        x = ldexp(x, n); // == x * 2ⁿ

        x(overflow) = std::numeric_limits<double>::infinity();
        x.setZero(underflow);

        return x;
    }

#endif
}  // namespace Vc

#undef Vc_COMMON_MATH_H_INTERNAL

#endif // VC_COMMON_MATH_H_
