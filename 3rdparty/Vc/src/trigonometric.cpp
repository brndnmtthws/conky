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

// enable bit operators for easier portable bit manipulation on floats
#define Vc_ENABLE_FLOAT_BIT_OPERATORS 1

#include <Vc/vector.h>
#if defined(Vc_IMPL_SSE) || defined(Vc_IMPL_AVX)
#include <Vc/common/macros.h>
//#include <Vc/IO>

namespace Vc_VERSIONED_NAMESPACE
{
namespace Common
{
namespace
{
using Vc::Vector;
template <typename T, typename Abi>
using Const = typename std::conditional<std::is_same<Abi, VectorAbi::Avx>::value,
                                        AVX::Const<T>, SSE::Const<T>>::type;

template <typename V>
using best_int_v_for =
    typename std::conditional<(V::size() <= Vector<int, VectorAbi::Best<int>>::size()),
                              Vector<int, VectorAbi::Best<int>>,
                              SimdArray<int, V::size()>>::type;
template <typename Abi> using float_int_v = best_int_v_for<Vector<float, Abi>>;
template <typename Abi> using double_int_v = best_int_v_for<Vector<double, Abi>>;

template <typename Abi>
static Vc_ALWAYS_INLINE Vector<float, Abi> cosSeries(const Vector<float, Abi> &x)
{
    const Vector<float, Abi> x2 = x * x;
    Vector<float, Abi> y;
    y =          Vc::Detail::floatConstant< 1, 0x500000, -16>();  //  1/8!
    y = y * x2 + Vc::Detail::floatConstant<-1, 0x360800, -10>();  // -1/6!
    y = y * x2 + Vc::Detail::floatConstant< 1, 0x2AAAAB,  -5>();  //  1/4!
    return y * (x2 * x2) - .5f * x2 + 1.f;
    // alternative (appears neither faster nor more precise):
    // return (y * x2 - .5f) * x2 + 1.f;
}

template <typename Abi>
static Vc_ALWAYS_INLINE Vector<double, Abi> cosSeries(const Vector<double, Abi> &x)
{
    const Vector<double, Abi> x2 = x * x;
    Vector<double, Abi> y;
    y =          Vc::Detail::doubleConstant< 1, 0xAC00000000000, -45>();  //  1/16!
    y = y * x2 + Vc::Detail::doubleConstant<-1, 0x9394000000000, -37>();  // -1/14!
    y = y * x2 + Vc::Detail::doubleConstant< 1, 0x1EED8C0000000, -29>();  //  1/12!
    y = y * x2 + Vc::Detail::doubleConstant<-1, 0x27E4FB7400000, -22>();  // -1/10!
    y = y * x2 + Vc::Detail::doubleConstant< 1, 0xA01A01A018000, -16>();  //  1/8!
    y = y * x2 + Vc::Detail::doubleConstant<-1, 0x6C16C16C16C00, -10>();  // -1/6!
    y = y * x2 + Vc::Detail::doubleConstant< 1, 0x5555555555554,  -5>();  //  1/4!
    return (y * x2 - .5f) * x2 + 1.f;
}

template <typename Abi>
static Vc_ALWAYS_INLINE Vector<float, Abi> sinSeries(const Vector<float, Abi> &x)
{
    const Vector<float, Abi> x2 = x * x;
    Vector<float, Abi> y;
    y =          Vc::Detail::floatConstant<-1, 0x4E6000, -13>();  // -1/7!
    y = y * x2 + Vc::Detail::floatConstant< 1, 0x088880,  -7>();  //  1/5!
    y = y * x2 + Vc::Detail::floatConstant<-1, 0x2AAAAB,  -3>();  // -1/3!
    return y * (x2 * x) + x;
}

template <typename Abi>
static Vc_ALWAYS_INLINE Vector<double, Abi> sinSeries(const Vector<double, Abi> &x)
{
    // x  = [0, 0.7854 = pi/4]
    // x² = [0, 0.6169 = pi²/8]
    const Vector<double, Abi> x2 = x * x;
    Vector<double, Abi> y;
    y =          Vc::Detail::doubleConstant<-1, 0xACF0000000000, -41>();  // -1/15!
    y = y * x2 + Vc::Detail::doubleConstant< 1, 0x6124400000000, -33>();  //  1/13!
    y = y * x2 + Vc::Detail::doubleConstant<-1, 0xAE64567000000, -26>();  // -1/11!
    y = y * x2 + Vc::Detail::doubleConstant< 1, 0x71DE3A5540000, -19>();  //  1/9!
    y = y * x2 + Vc::Detail::doubleConstant<-1, 0xA01A01A01A000, -13>();  // -1/7!
    y = y * x2 + Vc::Detail::doubleConstant< 1, 0x1111111111110,  -7>();  //  1/5!
    y = y * x2 + Vc::Detail::doubleConstant<-1, 0x5555555555555,  -3>();  // -1/3!
    return y * (x2 * x) + x;
}

/**\internal
 * Fold \p x into [-¼π, ¼π] and remember the quadrant it came from:
 * quadrant 0: [-¼π,  ¼π]
 * quadrant 1: [ ¼π,  ¾π]
 * quadrant 2: [ ¾π, 1¼π]
 * quadrant 3: [1¼π, 1¾π]
 *
 * The algorithm determines `y` as the multiple `x - y * ¼π = [-¼π, ¼π]`. Using a bitmask,
 * `y` is reduced to `quadrant`. `y` can be calculated as
 * ```
 * y = trunc(x / ¼π);
 * y += fmod(y, 2);
 * ```
 * This can be simplified by moving the (implicit) division by 2 into the truncation
 * expression. The `+= fmod` effect can the be achieved by using rounding instead of
 * truncation:
 * `y = round(x / ½π) * 2`.
 * If precision allows, `2/π * x` is better (faster).
 */
template <class T, class Abi> struct folded {
    Vector<T, Abi> x, quadrant;
};

template <typename Abi>
static Vc_ALWAYS_INLINE folded<float, Abi> foldInput(const Vector<float, Abi> &x)
{
    using V = Vector<float, Abi>;
    using IV = best_int_v_for<V>;
    using C = Const<float, Abi>;

    folded<float, Abi> r;
    r.x = abs(x);
    if (Vc_IS_UNLIKELY(all_of(r.x < C::_pi_4()))) {
        r.quadrant = 0;
    } else if (Vc_IS_LIKELY(all_of(r.x < 33 * C::_pi_4()))) {
        const float _2_over_pi = Vc::Detail::floatConstant<1, 0x22F983, -1>();  // ½π
        V y = round(r.x * _2_over_pi);
        r.quadrant = simd_cast<V>(simd_cast<IV>(y) & 3);            // y mod 4
        r.x -= y * Vc::Detail::floatConstant<1, 0x490fe0, 0>();     // pi/2
        r.x -= y * Vc::Detail::floatConstant<-1, 0x2bbbd3, -21>();  // pi/2 remainder
    } else {
        typedef SimdArray<double, V::size()> VD;
        VD xd = simd_cast<VD>(abs(r.x));
        constexpr double _2_over_pi =
            Vc::Detail::doubleConstant<1, 0x45F306DC9C883, -1>();
        VD y = round(xd * _2_over_pi);
        r.quadrant = simd_cast<V>(simd_cast<IV>(y) & 3);  // = y mod 4
        constexpr double pi_over_2 = Vc::Detail::doubleConstant<1, 0x921FB54442D18, 0>();
        r.x = simd_cast<V>(xd - y * pi_over_2);
    }
    //std::cout << std::hexfloat << r.x << ' ' << r.quadrant << std::defaultfloat << '\n';
    return r;

    /*
    More alternatives to avoid double precision calculation follow. Their main problem
    is unfortunate rounding on intermediate values of x (after subtraction):

    if (all_of(y <= 0x1000)) {
        // 0 <= x <= 0x1.921fb6p12
        // π/4 - 12 bits precision (12 zero bits):
        //const auto _pi_4_1 = Vc::Detail::floatConstant< 1, 0x491000,  -1>();
        //const auto _pi_4_2 = Vc::Detail::floatConstant<-1, 0x157000, -19>();
        //const auto _pi_4_3 = Vc::Detail::floatConstant<-1, 0x6F4B9F, -32>();
        x -= y * C::_pi_4_hi();
        x -= y * C::_pi_4_rem1();
        x -= y * C::_pi_4_rem2();
    } else { //if (all_of(y <= 0x10000)) {
        // consider y = 0x1.fffep+16 / x = 0x1.921e24p+16
        // critical input: 0x1.921fb6p+12 <= x <= 0x1.921fb6p+16
        // π/4 - 8 bits precision (16 zero bits):
        const auto _pi_4_1 = Vc::Detail::floatConstant<+1, 0x490000,  -1>();
        const auto _pi_4_2 = Vc::Detail::floatConstant<+1, 0x7E0000, -13>();
        const auto _pi_4_3 = Vc::Detail::floatConstant<-1, 0x2C0000, -22>();
        const auto _pi_4_4 = Vc::Detail::floatConstant<+1, 0x085A31, -31>();
        x -= y * _pi_4_1; // (x -= 0x1.91fe6ep+16) ->  0x1.fb6000p+4
        x -= y * _pi_4_2; // (x -= 0x1.fbfe04p+4)  -> -0x1.3c0800p-6
        x -= y * _pi_4_3; // (x -= 0x1.57fea8p-5)  ->  0x1.bf5a80p-9
        x -= y * _pi_4_4; // (x -= 0x1.10b352p-14) ->  0x1.b6e4e6p-9
        x.setQnan(y > 0x10000);
    }
    */
}

template <typename Abi>
static Vc_ALWAYS_INLINE folded<double, Abi> foldInput(const Vector<double, Abi> &x)
{
    using V = Vector<double, Abi>;
    using IV = best_int_v_for<V>;
    using C = Const<double, Abi> ;

    folded<double, Abi> r;
    r.x = abs(x);
    constexpr double pi_over_4 = Vc::Detail::doubleConstant<1, 0x921FB54442D18, -1>();
    if (Vc_IS_UNLIKELY(all_of(r.x < pi_over_4))) {
        r.quadrant = 0;
        return r;
    }
    const V y = round(r.x / (2 * pi_over_4));
    r.quadrant = simd_cast<V>(simd_cast<IV>(y) & 3);

    if (Vc_IS_LIKELY(all_of(r.x < 1025 * pi_over_4))) {
        // x - y * pi/2, y uses no more than 11 mantissa bits
        r.x -= y * Vc::Detail::doubleConstant< 1, 0x921FB54443000,   0>();
        r.x -= y * Vc::Detail::doubleConstant<-1, 0x73DCB3B39A000, -43>();
        r.x -= y * Vc::Detail::doubleConstant< 1, 0x45C06E0E68948, -86>();
    } else if (Vc_IS_LIKELY(all_of(y <= Vc::Detail::doubleConstant<1, 0, 30>()))) {
        // x - y * pi/2, y uses no more than 29 mantissa bits
        r.x -= y * Vc::Detail::doubleConstant<1, 0x921FB40000000,   0>();
        r.x -= y * Vc::Detail::doubleConstant<1, 0x4442D00000000, -24>();
        r.x -= y * Vc::Detail::doubleConstant<1, 0x8469898CC5170, -48>();
    } else {
        // x - y * pi/2, y may require all mantissa bits
        const V y_hi = y & C::highMask(26);
        const V y_lo = y - y_hi;
        const auto _pi_2_1 = Vc::Detail::doubleConstant<1, 0x921FB50000000,   0>();
        const auto _pi_2_2 = Vc::Detail::doubleConstant<1, 0x110B460000000, -26>();
        const auto _pi_2_3 = Vc::Detail::doubleConstant<1, 0x1A62630000000, -54>();
        const auto _pi_2_4 = Vc::Detail::doubleConstant<1, 0x8A2E03707344A, -81>();
        /*
        std::cout << std::hexfloat;
        std::cout
            << y_hi << '\n'
            << y_lo << '\n'
            << _pi_2_1 << '\n'
            << _pi_2_2 << '\n'
            << _pi_2_3 << '\n'
            << y_hi * _pi_2_1 << '\n'
            << y_hi * _pi_2_2 << '\n'
            << y_hi * _pi_2_3 << '\n'
            << y * _pi_2_4 << '\n'
            << y_lo * _pi_2_1 << '\n'
            << y_lo * _pi_2_2 << '\n'
            << y_lo * _pi_2_3 << '\n';
        std::cout << std::defaultfloat;
        */
        r.x = r.x
            - y_hi * _pi_2_1
            - max(y_hi * _pi_2_2, y_lo * _pi_2_1)
            - min(y_hi * _pi_2_2, y_lo * _pi_2_1)
            - max(y_hi * _pi_2_3, y_lo * _pi_2_2)
            - min(y_hi * _pi_2_3, y_lo * _pi_2_2)
            - max(y    * _pi_2_4, y_lo * _pi_2_3)
            - min(y    * _pi_2_4, y_lo * _pi_2_3);
    }
    return r;
}

constexpr double signmask = -0.;
constexpr float signmaskf = -0.f;
} // anonymous namespace

/*
 * algorithm for sine and cosine:
 *
 * The result can be calculated with sine or cosine depending on the π/4 section the input is
 * in.
 * sine   ≈ x + x³
 * cosine ≈ 1 - x²
 *
 * sine:
 * Map -x to x and invert the output
 * Extend precision of x - n * π/4 by calculating
 * ((x - n * p1) - n * p2) - n * p3 (p1 + p2 + p3 = π/4)
 *
 * Calculate Taylor series with tuned coefficients.
 * Fix sign.
 */
template <>
template <>
Vc::double_v Trigonometric<Vc::Detail::TrigonometricImplementation<
    Vc::CurrentImplementation::current()>>::sin(const Vc::double_v &x)
{
    using V = Vc::double_v;
    const auto f = foldInput(x);
    // quadrant | effect
    //        0 | sinSeries
    //        1 | cosSeries
    //        2 | sinSeries, sign flip
    //        3 | cosSeries, sign flip
    const V sin_sign = (x ^ (1 - f.quadrant)) & signmask;

    const V sin_s = sinSeries(f.x);
    const V cos_s = cosSeries(f.x);
    return sin_sign ^ iif(f.quadrant == 0 || f.quadrant == 2, sin_s, cos_s);
}

template <>
template <>
Vc::float_v Trigonometric<Vc::Detail::TrigonometricImplementation<
    Vc::CurrentImplementation::current()>>::sin(const Vc::float_v &x)
{
    using V = Vc::float_v;
    if (Vc_IS_UNLIKELY(any_of(abs(x) >= 527449))) {
        return simd_cast<V>(sin(simd_cast<Vc::double_v, 0>(x)),
                            sin(simd_cast<Vc::double_v, 1>(x)));
    }

    const auto f = foldInput(x);
    const V sin_sign = (x ^ (1 - f.quadrant)) & signmaskf;

    const V sin_s = sinSeries(f.x);
    const V cos_s = cosSeries(f.x);
    return sin_sign ^ iif(f.quadrant == 0 || f.quadrant == 2, sin_s, cos_s);
}

template <>
template <>
Vc::double_v Trigonometric<Vc::Detail::TrigonometricImplementation<
    Vc::CurrentImplementation::current()>>::cos(const Vc::double_v &x)
{
    using V = Vc::double_v;
    const auto f = foldInput(x);
    // quadrant | effect
    //        0 | cosSeries, +
    //        1 | sinSeries, -
    //        2 | cosSeries, -
    //        3 | sinSeries, +
    const V cos_sign = ((0.5 - f.quadrant) & (f.quadrant - 2.5)) & signmask;

    const V sin_s = sinSeries(f.x);
    const V cos_s = cosSeries(f.x);
    return cos_sign ^ iif(f.quadrant == 0 || f.quadrant == 2, cos_s, sin_s);
}

template <>
template <>
Vc::float_v Trigonometric<Vc::Detail::TrigonometricImplementation<
    Vc::CurrentImplementation::current()>>::cos(const Vc::float_v &x)
{
    using V = Vc::float_v;
    if (Vc_IS_UNLIKELY(any_of(abs(x) >= 393382))) {
        return simd_cast<V>(cos(simd_cast<Vc::double_v, 0>(x)),
                            cos(simd_cast<Vc::double_v, 1>(x)));
    }
    const auto f = foldInput(x);
    const V cos_sign = ((0.5f - f.quadrant) & (f.quadrant - 2.5f)) & signmaskf;

    const V sin_s = sinSeries(f.x);
    const V cos_s = cosSeries(f.x);
    return cos_sign ^ iif(f.quadrant == 0 || f.quadrant == 2, cos_s, sin_s);
}

template <>
template <>
void Trigonometric<Vc::Detail::TrigonometricImplementation<
    Vc::CurrentImplementation::current()>>::sincos(const Vc::double_v &x, Vc::double_v *s,
                                                   Vc::double_v *c)
{
    using V = Vc::double_v;
    const auto f = foldInput(x);
    // quadrant | cosine       | sine
    //        0 | cosSeries, + | sinSeries
    //        1 | sinSeries, - | cosSeries
    //        2 | cosSeries, - | sinSeries, sign flip
    //        3 | sinSeries, + | cosSeries, sign flip
    const V sin_sign = (x ^ (1 - f.quadrant)) & signmask;
    const V cos_sign = ((0.5 - f.quadrant) & (f.quadrant - 2.5)) & signmask;

    const V sin_s = sinSeries(f.x);
    const V cos_s = cosSeries(f.x);
    *s = sin_sign ^ iif(f.quadrant == 0 || f.quadrant == 2, sin_s, cos_s);
    *c = cos_sign ^ iif(f.quadrant == 0 || f.quadrant == 2, cos_s, sin_s);
}

template <>
template <>
void Trigonometric<Vc::Detail::TrigonometricImplementation<
    Vc::CurrentImplementation::current()>>::sincos(const Vc::float_v &x, Vc::float_v *s,
                                                   Vc::float_v *c)
{
    using V = Vc::float_v;
    if (Vc_IS_UNLIKELY(any_of(abs(x) >= 393382))) {
        Vc::double_v s0, s1, c0, c1;
        sincos(simd_cast<Vc::double_v, 0>(x), &s0, &c0);
        sincos(simd_cast<Vc::double_v, 1>(x), &s1, &c1);
        *s = simd_cast<V>(s0, s1);
        *c = simd_cast<V>(c0, c1);
        return;
    }
    const auto f = foldInput(x);
    const V sin_sign = (x ^ (1 - f.quadrant)) & signmaskf;
    const V cos_sign = ((0.5f - f.quadrant) & (f.quadrant - 2.5f)) & signmaskf;

    const V sin_s = sinSeries(f.x);
    const V cos_s = cosSeries(f.x);
    *s = sin_sign ^ iif(f.quadrant == 0 || f.quadrant == 2, sin_s, cos_s);
    *c = cos_sign ^ iif(f.quadrant == 0 || f.quadrant == 2, cos_s, sin_s);
}

template <>
template <>
Vc::float_v Trigonometric<Vc::Detail::TrigonometricImplementation<
    Vc::CurrentImplementation::current()>>::asin(const Vc::float_v &_x)
{
    typedef Vc::float_v V;
    typedef Const<float, V::abi> C;
    typedef V::Mask M;

    const M &negative = _x < V::Zero();

    const V &a = abs(_x);
    const M outOfRange = a > V::One();
    const M &small = a < C::smallAsinInput();
    const M &gt_0_5 = a > C::_1_2();
    V x = a;
    V z = a * a;
    z(gt_0_5) = (V::One() - a) * C::_1_2();
    x(gt_0_5) = sqrt(z);
    z = ((((C::asinCoeff0(0)  * z
          + C::asinCoeff0(1)) * z
          + C::asinCoeff0(2)) * z
          + C::asinCoeff0(3)) * z
          + C::asinCoeff0(4)) * z * x
          + x;
    z(gt_0_5) = C::_pi_2() - (z + z);
    z(small) = a;
    z(negative) = -z;
    z.setQnan(outOfRange);

    return z;
}
template<> template<> Vc::double_v Trigonometric<Vc::Detail::TrigonometricImplementation<Vc::CurrentImplementation::current()>>::asin (const Vc::double_v &_x) {
    typedef Vc::double_v V;
    typedef Const<double, V::abi> C;
    typedef V::Mask M;

    const M negative = _x < V::Zero();

    const V a = abs(_x);
    const M outOfRange = a > V::One();
    const M small = a < C::smallAsinInput();
    const M large = a > C::largeAsinInput();

    V zz = V::One() - a;
    const V r = (((C::asinCoeff0(0) * zz + C::asinCoeff0(1)) * zz + C::asinCoeff0(2)) * zz +
            C::asinCoeff0(3)) * zz + C::asinCoeff0(4);
    const V s = (((zz + C::asinCoeff1(0)) * zz + C::asinCoeff1(1)) * zz +
            C::asinCoeff1(2)) * zz + C::asinCoeff1(3);
    V sqrtzz = sqrt(zz + zz);
    V z = C::_pi_4() - sqrtzz;
    z -= sqrtzz * (zz * r / s) - C::_pi_2_rem();
    z += C::_pi_4();

    V a2 = a * a;
    const V p = ((((C::asinCoeff2(0) * a2 + C::asinCoeff2(1)) * a2 + C::asinCoeff2(2)) * a2 +
                C::asinCoeff2(3)) * a2 + C::asinCoeff2(4)) * a2 + C::asinCoeff2(5);
    const V q = ((((a2 + C::asinCoeff3(0)) * a2 + C::asinCoeff3(1)) * a2 +
                C::asinCoeff3(2)) * a2 + C::asinCoeff3(3)) * a2 + C::asinCoeff3(4);
    z(!large) = a * (a2 * p / q) + a;

    z(negative) = -z;
    z(small) = _x;
    z.setQnan(outOfRange);

    return z;
}
template <>
template <>
Vc::float_v Trigonometric<Vc::Detail::TrigonometricImplementation<
    Vc::CurrentImplementation::current()>>::atan(const Vc::float_v &_x)
{
    using V = Vc::float_v;
    typedef Const<float, V::abi> C;
    typedef V::Mask M;
    V x = abs(_x);
    const M &gt_tan_3pi_8 = x > C::atanThrsHi();
    const M &gt_tan_pi_8  = x > C::atanThrsLo() && !gt_tan_3pi_8;
    V y = V::Zero();
    y(gt_tan_3pi_8) = C::_pi_2();
    y(gt_tan_pi_8)  = C::_pi_4();
    x(gt_tan_3pi_8) = -V::One() / x;
    x(gt_tan_pi_8)  = (x - V::One()) / (x + V::One());
    const V &x2 = x * x;
    y += (((C::atanP(0)  * x2
          - C::atanP(1)) * x2
          + C::atanP(2)) * x2
          - C::atanP(3)) * x2 * x
          + x;
    y(_x < V::Zero()) = -y;
    y.setQnan(isnan(_x));
    return y;
}
template<> template<> Vc::double_v Trigonometric<Vc::Detail::TrigonometricImplementation<Vc::CurrentImplementation::current()>>::atan (const Vc::double_v &_x) {
    typedef Vc::double_v V;
    typedef Const<double, V::abi> C;
    typedef V::Mask M;

    M sign = _x < V::Zero();
    V x = abs(_x);
    M finite = isfinite(_x);
    V ret = C::_pi_2();
    V y = V::Zero();
    const M large = x > C::atanThrsHi();
    const M gt_06 = x > C::atanThrsLo();
    V tmp = (x - V::One()) / (x + V::One());
    tmp(large) = -V::One() / x;
    x(gt_06) = tmp;
    y(gt_06) = C::_pi_4();
    y(large) = C::_pi_2();
    V z = x * x;
    const V p = (((C::atanP(0) * z + C::atanP(1)) * z + C::atanP(2)) * z + C::atanP(3)) * z + C::atanP(4);
    const V q = ((((z + C::atanQ(0)) * z + C::atanQ(1)) * z + C::atanQ(2)) * z + C::atanQ(3)) * z + C::atanQ(4);
    z = z * p / q;
    z = x * z + x;
    V morebits = C::_pi_2_rem();
    morebits(!large) *= C::_1_2();
    z(gt_06) += morebits;
    ret(finite) = y + z;
    ret(sign) = -ret;
    ret.setQnan(isnan(_x));
    return ret;
}
template <>
template <>
Vc::float_v Trigonometric<Vc::Detail::TrigonometricImplementation<
    Vc::CurrentImplementation::current()>>::atan2(const Vc::float_v &y,
                                                  const Vc::float_v &x)
{
    using V = Vc::float_v;
    typedef Const<float, V::abi> C;
    typedef V::Mask M;

    const M xZero = x == V::Zero();
    const M yZero = y == V::Zero();
    const M xMinusZero = xZero && isnegative(x);
    const M yNeg = y < V::Zero();
    const M xInf = !isfinite(x);
    const M yInf = !isfinite(y);

    V a = copysign(C::_pi(), y);
    a.setZero(x >= V::Zero());

    // setting x to any finite value will have atan(y/x) return sign(y/x)*pi/2, just in case x is inf
    V _x = x;
    _x(yInf) = copysign(V::One(), x);

    a += atan(y / _x);

    // if x is +0 and y is +/-0 the result is +0
    a.setZero(xZero && yZero);

    // for x = -0 we add/subtract pi to get the correct result
    a(xMinusZero) += copysign(C::_pi(), y);

    // atan2(-Y, +/-0) = -pi/2
    a(xZero && yNeg) = -C::_pi_2();

    // if both inputs are inf the output is +/- (3)pi/4
    a(xInf && yInf) += copysign(C::_pi_4(), x ^ ~y);

    // correct the sign of y if the result is 0
    a(a == V::Zero()) = copysign(a, y);

    // any NaN input will lead to NaN output
    a.setQnan(isnan(y) || isnan(x));

    return a;
}
template<> template<> Vc::double_v Trigonometric<Vc::Detail::TrigonometricImplementation<Vc::CurrentImplementation::current()>>::atan2 (const Vc::double_v &y, const Vc::double_v &x) {
    typedef Vc::double_v V;
    typedef Const<double, V::abi> C;
    typedef V::Mask M;

    const M xZero = x == V::Zero();
    const M yZero = y == V::Zero();
    const M xMinusZero = xZero && isnegative(x);
    const M yNeg = y < V::Zero();
    const M xInf = !isfinite(x);
    const M yInf = !isfinite(y);

    V a = copysign(V(C::_pi()), y);
    a.setZero(x >= V::Zero());

    // setting x to any finite value will have atan(y/x) return sign(y/x)*pi/2, just in case x is inf
    V _x = x;
    _x(yInf) = copysign(V::One(), x);

    a += atan(y / _x);

    // if x is +0 and y is +/-0 the result is +0
    a.setZero(xZero && yZero);

    // for x = -0 we add/subtract pi to get the correct result
    a(xMinusZero) += copysign(C::_pi(), y);

    // atan2(-Y, +/-0) = -pi/2
    a(xZero && yNeg) = -C::_pi_2();

    // if both inputs are inf the output is +/- (3)pi/4
    a(xInf && yInf) += copysign(C::_pi_4(), x ^ ~y);

    // correct the sign of y if the result is 0
    a(a == V::Zero()) = copysign(a, y);

    // any NaN input will lead to NaN output
    a.setQnan(isnan(y) || isnan(x));

    return a;
}

}
}

#endif
