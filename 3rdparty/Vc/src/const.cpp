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

#include <Vc/avx/const_data.h>
#include <Vc/sse/const_data.h>
#include <Vc/version.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <Vc/common/const.h>
#include <Vc/common/macros.h>

namespace Vc_VERSIONED_NAMESPACE
{
namespace AVX
{
    using Detail::doubleConstant;
    using Detail::floatConstant;

    // cacheline 1
    alignas(64) extern const unsigned int   _IndexesFromZero32[ 8] = { 0, 1, 2, 3, 4, 5, 6, 7 };
    alignas(16) extern const unsigned short _IndexesFromZero16[16] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };
    alignas(16) extern const unsigned char  _IndexesFromZero8 [32] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31 };

    template <>
    alignas(64) const double c_trig<double>::data[] = {
    // cacheline 4
        doubleConstant<1, 0x921fb54442d18ull, -1>(), // π/4
        doubleConstant<1, 0x921fb40000000ull, -1>(), // π/4 - 30bits precision
        doubleConstant<1, 0x4442d00000000ull, -25>(), // π/4 remainder1 - 32bits precision
        doubleConstant<1, 0x8469898cc5170ull, -49>(), // π/4 remainder2
        0.0625,
        16.,
        0., // padding
        0., // padding
        0., // padding (for alignment with float)
        doubleConstant<1, 0x8BE60DB939105ull,  0>(), // 4/π
        doubleConstant<1, 0x921fb54442d18ull,  0>(), // π/2
        doubleConstant<1, 0x921fb54442d18ull,  1>(), // π
    // cacheline 10
        doubleConstant<-1, 0xc007fa1f72594ull, -1>(), // atan P coefficients
        doubleConstant<-1, 0x028545b6b807aull,  4>(), // atan P coefficients
        doubleConstant<-1, 0x2c08c36880273ull,  6>(), // atan P coefficients
        doubleConstant<-1, 0xeb8bf2d05ba25ull,  6>(), // atan P coefficients
        doubleConstant<-1, 0x03669fd28ec8eull,  6>(), // atan P coefficients
        doubleConstant< 1, 0x8dbc45b14603cull,  4>(), // atan Q coefficients
        doubleConstant< 1, 0x4a0dd43b8fa25ull,  7>(), // atan Q coefficients
        doubleConstant< 1, 0xb0e18d2e2be3bull,  8>(), // atan Q coefficients
    // cacheline 12
        doubleConstant< 1, 0xe563f13b049eaull,  8>(), // atan Q coefficients
        doubleConstant< 1, 0x8519efbbd62ecull,  7>(), // atan Q coefficients
        doubleConstant< 1, 0x3504f333f9de6ull,  1>(), // tan( 3/8 π )
        0.66,                                    // lower threshold for special casing in atan
        doubleConstant<1, 0x1A62633145C07ull, -54>(), // remainder of pi/2
        1.e-8, // small asin input threshold
        0.625, // large asin input threshold
        0., // padding
    // cacheline 14
        doubleConstant< 1, 0x84fc3988e9f08ull, -9>(), // asinCoeff0
        doubleConstant<-1, 0x2079259f9290full, -1>(), // asinCoeff0
        doubleConstant< 1, 0xbdff5baf33e6aull,  2>(), // asinCoeff0
        doubleConstant<-1, 0x991aaac01ab68ull,  4>(), // asinCoeff0
        doubleConstant< 1, 0xc896240f3081dull,  4>(), // asinCoeff0
        doubleConstant<-1, 0x5f2a2b6bf5d8cull,  4>(), // asinCoeff1
        doubleConstant< 1, 0x26219af6a7f42ull,  7>(), // asinCoeff1
        doubleConstant<-1, 0x7fe08959063eeull,  8>(), // asinCoeff1
    // cacheline 16
        doubleConstant< 1, 0x56709b0b644beull,  8>(), // asinCoeff1
        doubleConstant< 1, 0x16b9b0bd48ad3ull, -8>(), // asinCoeff2
        doubleConstant<-1, 0x34341333e5c16ull, -1>(), // asinCoeff2
        doubleConstant< 1, 0x5c74b178a2dd9ull,  2>(), // asinCoeff2
        doubleConstant<-1, 0x04331de27907bull,  4>(), // asinCoeff2
        doubleConstant< 1, 0x39007da779259ull,  4>(), // asinCoeff2
        doubleConstant<-1, 0x0656c06ceafd5ull,  3>(), // asinCoeff2
        doubleConstant<-1, 0xd7b590b5e0eabull,  3>(), // asinCoeff3
    // cacheline 18
        doubleConstant< 1, 0x19fc025fe9054ull,  6>(), // asinCoeff3
        doubleConstant<-1, 0x265bb6d3576d7ull,  7>(), // asinCoeff3
        doubleConstant< 1, 0x1705684ffbf9dull,  7>(), // asinCoeff3
        doubleConstant<-1, 0x898220a3607acull,  5>(), // asinCoeff3
    };
#define Vc_4(x) x
    template <>
    alignas(64) const float c_trig<float>::data[] = {
    // cacheline
        Vc_4((floatConstant< 1, 0x490FDB,  -1>())), // π/4
        Vc_4((floatConstant< 1, 0x491000,  -1>())), // π/4 - 12 bits precision
        Vc_4((floatConstant<-1, 0x157000, -19>())), // π/4 remainder1 - 12 bits precision
        Vc_4((floatConstant<-1, 0x6F4B9F, -32>())), // π/4 remainder2
        Vc_4(0.0625f),
        Vc_4(16.f),
        Vc_4(0.f), // padding
        Vc_4(0.f), // padding
    // cacheline
        Vc_4(8192.f), // loss threshold
        Vc_4((floatConstant<1, 0x22F983, 0>())), // 1.27323949337005615234375 = 4/π
        Vc_4((floatConstant<1, 0x490FDB, 0>())), // π/2
        Vc_4((floatConstant<1, 0x490FDB, 1>())), // π
        Vc_4(8.05374449538e-2f), // atan P coefficients
        Vc_4(1.38776856032e-1f), // atan P coefficients
        Vc_4(1.99777106478e-1f), // atan P coefficients
        Vc_4(3.33329491539e-1f), // atan P coefficients
        Vc_4(0.f), // padding (for alignment with double)
        Vc_4(0.f), // padding (for alignment with double)
        Vc_4(0.f), // padding (for alignment with double)
        Vc_4(0.f), // padding (for alignment with double)
    // cacheline
        Vc_4(0.f), // padding (for alignment with double)
        Vc_4(0.f), // padding (for alignment with double)
        Vc_4(2.414213562373095f), // tan( 3/8 π )
        Vc_4(0.414213562373095f), // tan( 1/8 π ) lower threshold for special casing in atan
        Vc_4((floatConstant<-1, 0x3BBD2E, -25>())), // remainder of pi/2
        Vc_4(1.e-4f), // small asin input threshold
        Vc_4(0.f), // padding (for alignment with double)
        Vc_4(0.f), // padding (for alignment with double)
        Vc_4(4.2163199048e-2f), // asinCoeff0
        Vc_4(2.4181311049e-2f), // asinCoeff0
        Vc_4(4.5470025998e-2f), // asinCoeff0
        Vc_4(7.4953002686e-2f), // asinCoeff0
        Vc_4(1.6666752422e-1f), // asinCoeff0
        Vc_4(0.f), // padding (for alignment with double)
        Vc_4(0.f), // padding (for alignment with double)
        Vc_4(0.f), // padding (for alignment with double)
    // cacheline
        Vc_4(0.f), // padding (for alignment with double)
        Vc_4(0.f), // padding (for alignment with double)
        Vc_4(0.f), // padding (for alignment with double)
        Vc_4(0.f), // padding (for alignment with double)
        Vc_4(0.f), // padding (for alignment with double)
        Vc_4(0.f), // padding (for alignment with double)
        Vc_4(0.f), // padding (for alignment with double)
        Vc_4(0.f), // padding (for alignment with double)
        Vc_4(0.f), // padding (for alignment with double)
        Vc_4(0.f), // padding (for alignment with double)
        Vc_4(0.f), // padding (for alignment with double)
        Vc_4(0.f), // padding (for alignment with double)
    };
#undef Vc_4

    const unsigned       int c_general::absMaskFloat[2] = { 0xffffffffu, 0x7fffffffu };
    const unsigned       int c_general::signMaskFloat[2] = { 0x0u, 0x80000000u };
    const unsigned       int c_general::highMaskFloat = 0xfffff000u;
    const              float c_general::oneFloat = 1.f;
    const unsigned     short c_general::minShort[2] = { 0x8000u, 0x8000u };
    const unsigned     short c_general::one16[2] = { 1, 1 };
    const              float c_general::_2power31 = 1u << 31;

    // cacheline 4
    const unsigned long long c_general::highMaskDouble = 0xfffffffff8000000ull;
    const             double c_general::oneDouble = 1.;
    const unsigned long long c_general::frexpMask = 0xbfefffffffffffffull;

    alignas(64) const unsigned long long c_log<double>::data[21] = {
        0x000003ff000003ffull // bias TODO: remove
      , 0x7ff0000000000000ull // exponentMask (+inf)

      , 0x3f1ab4c293c31bb0ull // P[0]
      , 0x3fdfd6f53f5652f2ull // P[1]
      , 0x4012d2baed926911ull // P[2]
      , 0x402cff72c63eeb2eull // P[3]
      , 0x4031efd6924bc84dull // P[4]
      , 0x401ed5637d7edcf8ull // P[5]

      , 0x40269320ae97ef8eull // Q[0]
      , 0x40469d2c4e19c033ull // Q[1]
      , 0x4054bf33a326bdbdull // Q[2]
      , 0x4051c9e2eb5eae21ull // Q[3]
      , 0x4037200a9e1f25b2ull // Q[4]

      , 0xfff0000000000000ull // -inf
      , 0x0010000000000000ull // min()
      , 0x3fe6a09e667f3bcdull // 1/sqrt(2)
      , 0x3fe6300000000000ull // round(ln(2) * 512) / 512
      , 0xbf2bd0105c610ca8ull // ln(2) - round(ln(2) * 512) / 512
      , 0x3fe0000000000000ull // 0.5
      , 0x3fdbcb7b1526e50eull // log10(e)
      , 0x3ff71547652b82feull // log2(e)
    };

    template <>
    alignas(64) const unsigned int c_log<float>::data[21] = {
        0x0000007fu // bias TODO: remove
      , 0x7f800000u // exponentMask (+inf)

      , 0x3d9021bbu //  7.0376836292e-2f // P[0]
      , 0xbdebd1b8u // -1.1514610310e-1f // P[1]
      , 0x3def251au //  1.1676998740e-1f // P[2]
      , 0xbdfe5d4fu // -1.2420140846e-1f // P[3]
      , 0x3e11e9bfu //  1.4249322787e-1f // P[4]
      , 0xbe2aae50u // -1.6668057665e-1f // P[5]
      , 0x3e4cceacu //  2.0000714765e-1f // P[6]
      , 0xbe7ffffcu // -2.4999993993e-1f // P[7]
      , 0x3eaaaaaau //  3.3333331174e-1f // P[8]
      , 0           // padding because of c_log<double>
      , 0           // padding because of c_log<double>

      , 0xff800000u // -inf
      , 0x00800000u // min()
      , 0x3f3504f3u // 1/sqrt(2)
      , 0x3f318000u // round(ln(2) * 512) / 512
      , 0xb95e8083u // ln(2) - round(ln(2) * 512) / 512
      , 0x3f000000u // 0.5
      , 0x3ede5bd9u // log10(e)
      , 0x3fb8aa3bu // log2(e)
    };
}
}

namespace Vc_VERSIONED_NAMESPACE
{
namespace Common
{
    alignas(64) unsigned int RandomState[16] = {
        0x5a383a4fu, 0xc68bd45eu, 0x691d6d86u, 0xb367e14fu,
        0xd689dbaau, 0xfde442aau, 0x3d265423u, 0x1a77885cu,
        0x36ed2684u, 0xfb1f049du, 0x19e52f31u, 0x821e4dd7u,
        0x23996d25u, 0x5962725au, 0x6aced4ceu, 0xd4c610f3u
    };

    alignas(32) const unsigned int AllBitsSet[8] = {
        0xffffffffU, 0xffffffffU, 0xffffffffU, 0xffffffffU, 0xffffffffU, 0xffffffffU, 0xffffffffU, 0xffffffffU
    };

    const char LIBRARY_VERSION[] = Vc_VERSION_STRING;
    const unsigned int LIBRARY_VERSION_NUMBER = Vc_VERSION_NUMBER;
    const unsigned int LIBRARY_ABI_VERSION = Vc_LIBRARY_ABI_VERSION;

    void Vc_CDECL checkLibraryAbi(unsigned int compileTimeAbi, unsigned int versionNumber,
                                  const char *compileTimeVersion)
    {
        if (LIBRARY_ABI_VERSION != compileTimeAbi || LIBRARY_VERSION_NUMBER < versionNumber) {
            printf("The versions of libVc.a (%s) and Vc/version.h (%s) are incompatible. Aborting.\n", LIBRARY_VERSION, compileTimeVersion);
            abort();
        }
    }
}
}

namespace Vc_VERSIONED_NAMESPACE
{
namespace SSE
{
    using Detail::doubleConstant;
    using Detail::floatConstant;

    // cacheline 1
    alignas(64) const int c_general::absMaskFloat[4] = { 0x7fffffff, 0x7fffffff, 0x7fffffff, 0x7fffffff };
    alignas(16) const unsigned int c_general::signMaskFloat[4] = { 0x80000000, 0x80000000, 0x80000000, 0x80000000 };
    alignas(16) const unsigned int c_general::highMaskFloat[4] = { 0xfffff000u, 0xfffff000u, 0xfffff000u, 0xfffff000u };
    alignas(16) const short c_general::minShort[8] = { -0x8000, -0x8000, -0x8000, -0x8000, -0x8000, -0x8000, -0x8000, -0x8000 };
    alignas(16) extern const unsigned short _IndexesFromZero8[8] = { 0, 1, 2, 3, 4, 5, 6, 7 };

    // cacheline 2
    alignas(16) extern const unsigned int   _IndexesFromZero4[4] = { 0, 1, 2, 3 };
    alignas(16) const unsigned short c_general::one16[8] = { 1, 1, 1, 1, 1, 1, 1, 1 };
    alignas(16) const unsigned int c_general::one32[4] = { 1, 1, 1, 1 };
    alignas(16) const float c_general::oneFloat[4] = { 1.f, 1.f, 1.f, 1.f };

    // cacheline 3
    alignas(16) const unsigned long long c_general::highMaskDouble[2] = { 0xfffffffff8000000ull, 0xfffffffff8000000ull };
    alignas(16) const double c_general::oneDouble[2] = { 1., 1. };
    alignas(16) const long long c_general::absMaskDouble[2] = { 0x7fffffffffffffffll, 0x7fffffffffffffffll };
    alignas(16) const unsigned long long c_general::signMaskDouble[2] = { 0x8000000000000000ull, 0x8000000000000000ull };
    alignas(16) const unsigned long long c_general::frexpMask[2] = { 0xbfefffffffffffffull, 0xbfefffffffffffffull };

#define Vc_2(x) x, x
    template <>
    alignas(64) const double c_trig<double>::data[] = {
    // cacheline 4
        Vc_2((doubleConstant<1, 0x921fb54442d18ull, -1>())), // π/4
        Vc_2((doubleConstant<1, 0x921fb40000000ull, -1>())), // π/4 - 30bits precision
        Vc_2((doubleConstant<1, 0x4442d00000000ull, -25>())), // π/4 remainder1 - 32bits precision
        Vc_2((doubleConstant<1, 0x8469898cc5170ull, -49>())), // π/4 remainder2
    // cacheline 5
        Vc_2(0.0625),
        Vc_2(16.),
        Vc_2(0.), // padding
        Vc_2(0.), // padding
    // cacheline 6
        Vc_2(0.), // padding (for alignment with float)
        Vc_2((doubleConstant<1, 0x8BE60DB939105ull,  0>())), // 4/π
        Vc_2((doubleConstant<1, 0x921fb54442d18ull,  0>())), // π/2
        Vc_2((doubleConstant<1, 0x921fb54442d18ull,  1>())), // π
    // cacheline 7
        Vc_2((doubleConstant<-1, 0xc007fa1f72594ull, -1>())), // atan P coefficients
        Vc_2((doubleConstant<-1, 0x028545b6b807aull,  4>())), // atan P coefficients
        Vc_2((doubleConstant<-1, 0x2c08c36880273ull,  6>())), // atan P coefficients
        Vc_2((doubleConstant<-1, 0xeb8bf2d05ba25ull,  6>())), // atan P coefficients
    // cacheline 8
        Vc_2((doubleConstant<-1, 0x03669fd28ec8eull,  6>())), // atan P coefficients
        Vc_2((doubleConstant< 1, 0x8dbc45b14603cull,  4>())), // atan Q coefficients
        Vc_2((doubleConstant< 1, 0x4a0dd43b8fa25ull,  7>())), // atan Q coefficients
        Vc_2((doubleConstant< 1, 0xb0e18d2e2be3bull,  8>())), // atan Q coefficients
    // cacheline 9
        Vc_2((doubleConstant< 1, 0xe563f13b049eaull,  8>())), // atan Q coefficients
        Vc_2((doubleConstant< 1, 0x8519efbbd62ecull,  7>())), // atan Q coefficients
        Vc_2((doubleConstant< 1, 0x3504f333f9de6ull,  1>())), // tan( 3/8 π )
        Vc_2(0.66),                                    // lower threshold for special casing in atan
    // cacheline 10
        Vc_2((doubleConstant<1, 0x1A62633145C07ull, -54>())), // remainder of pi/2
        Vc_2(1.e-8), // small asin input threshold
        Vc_2(0.625), // large asin input threshold
        Vc_2(0.), // padding
    // cacheline 11
        Vc_2((doubleConstant< 1, 0x84fc3988e9f08ull, -9>())), // asinCoeff0
        Vc_2((doubleConstant<-1, 0x2079259f9290full, -1>())), // asinCoeff0
        Vc_2((doubleConstant< 1, 0xbdff5baf33e6aull,  2>())), // asinCoeff0
        Vc_2((doubleConstant<-1, 0x991aaac01ab68ull,  4>())), // asinCoeff0
    // cacheline 12
        Vc_2((doubleConstant< 1, 0xc896240f3081dull,  4>())), // asinCoeff0
        Vc_2((doubleConstant<-1, 0x5f2a2b6bf5d8cull,  4>())), // asinCoeff1
        Vc_2((doubleConstant< 1, 0x26219af6a7f42ull,  7>())), // asinCoeff1
        Vc_2((doubleConstant<-1, 0x7fe08959063eeull,  8>())), // asinCoeff1
    // cacheline 13
        Vc_2((doubleConstant< 1, 0x56709b0b644beull,  8>())), // asinCoeff1
        Vc_2((doubleConstant< 1, 0x16b9b0bd48ad3ull, -8>())), // asinCoeff2
        Vc_2((doubleConstant<-1, 0x34341333e5c16ull, -1>())), // asinCoeff2
        Vc_2((doubleConstant< 1, 0x5c74b178a2dd9ull,  2>())), // asinCoeff2
    // cacheline 14
        Vc_2((doubleConstant<-1, 0x04331de27907bull,  4>())), // asinCoeff2
        Vc_2((doubleConstant< 1, 0x39007da779259ull,  4>())), // asinCoeff2
        Vc_2((doubleConstant<-1, 0x0656c06ceafd5ull,  3>())), // asinCoeff2
        Vc_2((doubleConstant<-1, 0xd7b590b5e0eabull,  3>())), // asinCoeff3
    // cacheline 15
        Vc_2((doubleConstant< 1, 0x19fc025fe9054ull,  6>())), // asinCoeff3
        Vc_2((doubleConstant<-1, 0x265bb6d3576d7ull,  7>())), // asinCoeff3
        Vc_2((doubleConstant< 1, 0x1705684ffbf9dull,  7>())), // asinCoeff3
        Vc_2((doubleConstant<-1, 0x898220a3607acull,  5>())), // asinCoeff3
    };
#undef Vc_2
#define Vc_4(x) x, x, x, x
    template <>
    alignas(64) const float c_trig<float>::data[] = {
    // cacheline
        Vc_4((floatConstant< 1, 0x490FDB,  -1>())), // π/4
        Vc_4((floatConstant< 1, 0x491000,  -1>())), // π/4 - 12 bits precision
        Vc_4((floatConstant<-1, 0x157000, -19>())), // π/4 remainder1 - 12 bits precision
        Vc_4((floatConstant<-1, 0x6F4B9F, -32>())), // π/4 remainder2
    // cacheline
        Vc_4(0.0625f),
        Vc_4(16.f),
        Vc_4(0.f), // padding
        Vc_4(0.f), // padding
    // cacheline
        Vc_4(8192.f), // loss threshold
        Vc_4((floatConstant<1, 0x22F983, 0>())), // 1.27323949337005615234375 = 4/π
        Vc_4((floatConstant<1, 0x490FDB, 0>())), // π/2
        Vc_4((floatConstant<1, 0x490FDB, 1>())), // π
    // cacheline
        Vc_4(8.05374449538e-2f), // atan P coefficients
        Vc_4(1.38776856032e-1f), // atan P coefficients
        Vc_4(1.99777106478e-1f), // atan P coefficients
        Vc_4(3.33329491539e-1f), // atan P coefficients
    // cacheline
        Vc_4(0.f), // padding (for alignment with double)
        Vc_4(0.f), // padding (for alignment with double)
        Vc_4(0.f), // padding (for alignment with double)
        Vc_4(0.f), // padding (for alignment with double)
    // cacheline
        Vc_4(0.f), // padding (for alignment with double)
        Vc_4(0.f), // padding (for alignment with double)
        Vc_4(2.414213562373095f), // tan( 3/8 π )
        Vc_4(0.414213562373095f), // tan( 1/8 π ) lower threshold for special casing in atan
    // cacheline
        Vc_4((floatConstant<-1, 0x3BBD2E, -25>())), // remainder of pi/2
        Vc_4(1.e-4f), // small asin input threshold
        Vc_4(0.f), // padding (for alignment with double)
        Vc_4(0.f), // padding (for alignment with double)
    // cacheline
        Vc_4(4.2163199048e-2f), // asinCoeff0
        Vc_4(2.4181311049e-2f), // asinCoeff0
        Vc_4(4.5470025998e-2f), // asinCoeff0
        Vc_4(7.4953002686e-2f), // asinCoeff0
    // cacheline
        Vc_4(1.6666752422e-1f), // asinCoeff0
        Vc_4(0.f), // padding (for alignment with double)
        Vc_4(0.f), // padding (for alignment with double)
        Vc_4(0.f), // padding (for alignment with double)
    // cacheline
        Vc_4(0.f), // padding (for alignment with double)
        Vc_4(0.f), // padding (for alignment with double)
        Vc_4(0.f), // padding (for alignment with double)
        Vc_4(0.f), // padding (for alignment with double)
    // cacheline
        Vc_4(0.f), // padding (for alignment with double)
        Vc_4(0.f), // padding (for alignment with double)
        Vc_4(0.f), // padding (for alignment with double)
        Vc_4(0.f), // padding (for alignment with double)
    // cacheline
        Vc_4(0.f), // padding (for alignment with double)
        Vc_4(0.f), // padding (for alignment with double)
        Vc_4(0.f), // padding (for alignment with double)
        Vc_4(0.f), // padding (for alignment with double)
    };
#undef Vc_4

    // cacheline 8
    alignas(16) extern const unsigned char _IndexesFromZero16[16] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };

    alignas(64) const unsigned long long c_log<double>::data[21 * 2] = {
      /* 0*/   0x000003ff000003ffull, 0x000003ff000003ffull // bias TODO: remove
      /* 1*/ , 0x7ff0000000000000ull, 0x7ff0000000000000ull // exponentMask (+inf)

      /* 2*/ , 0x3f1ab4c293c31bb0ull, 0x3f1ab4c293c31bb0ull // P[0]
      /* 3*/ , 0x3fdfd6f53f5652f2ull, 0x3fdfd6f53f5652f2ull // P[1]
      /* 4*/ , 0x4012d2baed926911ull, 0x4012d2baed926911ull // P[2]
      /* 5*/ , 0x402cff72c63eeb2eull, 0x402cff72c63eeb2eull // P[3]
      /* 6*/ , 0x4031efd6924bc84dull, 0x4031efd6924bc84dull // P[4]
      /* 7*/ , 0x401ed5637d7edcf8ull, 0x401ed5637d7edcf8ull // P[5]

      /* 8*/ , 0x40269320ae97ef8eull, 0x40269320ae97ef8eull // Q[0]
      /* 9*/ , 0x40469d2c4e19c033ull, 0x40469d2c4e19c033ull // Q[1]
      /*10*/ , 0x4054bf33a326bdbdull, 0x4054bf33a326bdbdull // Q[2]
      /*11*/ , 0x4051c9e2eb5eae21ull, 0x4051c9e2eb5eae21ull // Q[3]
      /*12*/ , 0x4037200a9e1f25b2ull, 0x4037200a9e1f25b2ull // Q[4]

      /*13*/ , 0xfff0000000000000ull, 0xfff0000000000000ull // -inf
      /*14*/ , 0x0010000000000000ull, 0x0010000000000000ull // min()
      /*15*/ , 0x3fe6a09e667f3bcdull, 0x3fe6a09e667f3bcdull // 1/sqrt(2)
      /*16*/ , 0x3fe6300000000000ull, 0x3fe6300000000000ull // round(ln(2) * 512) / 512
      /*17*/ , 0xbf2bd0105c610ca8ull, 0xbf2bd0105c610ca8ull // ln(2) - round(ln(2) * 512) / 512
      /*18*/ , 0x3fe0000000000000ull, 0x3fe0000000000000ull // 0.5
      /*19*/ , 0x3fdbcb7b1526e50eull, 0x3fdbcb7b1526e50eull // log10(e)
      /*20*/ , 0x3ff71547652b82feull, 0x3ff71547652b82feull // log2(e)
    };

    template<> alignas(64) const unsigned int c_log<float>::data[21 * 4] = {
        0x0000007fu, 0x0000007fu, 0x0000007fu, 0x0000007fu, // bias TODO: remove
        0x7f800000u, 0x7f800000u, 0x7f800000u, 0x7f800000u, // exponentMask (+inf)

        0x3d9021bbu, 0x3d9021bbu, 0x3d9021bbu, 0x3d9021bbu, //  7.0376836292e-2f // P[0]
        0xbdebd1b8u, 0xbdebd1b8u, 0xbdebd1b8u, 0xbdebd1b8u, // -1.1514610310e-1f // P[1]
        0x3def251au, 0x3def251au, 0x3def251au, 0x3def251au, //  1.1676998740e-1f // P[2]
        0xbdfe5d4fu, 0xbdfe5d4fu, 0xbdfe5d4fu, 0xbdfe5d4fu, // -1.2420140846e-1f // P[3]
        0x3e11e9bfu, 0x3e11e9bfu, 0x3e11e9bfu, 0x3e11e9bfu, //  1.4249322787e-1f // P[4]
        0xbe2aae50u, 0xbe2aae50u, 0xbe2aae50u, 0xbe2aae50u, // -1.6668057665e-1f // P[5]
        0x3e4cceacu, 0x3e4cceacu, 0x3e4cceacu, 0x3e4cceacu, //  2.0000714765e-1f // P[6]
        0xbe7ffffcu, 0xbe7ffffcu, 0xbe7ffffcu, 0xbe7ffffcu, // -2.4999993993e-1f // P[7]
        0x3eaaaaaau, 0x3eaaaaaau, 0x3eaaaaaau, 0x3eaaaaaau, //  3.3333331174e-1f // P[8]
        0,           0,           0,           0,           // padding because of c_log<double>
        0,           0,           0,           0,           // padding because of c_log<double>

        0xff800000u, 0xff800000u, 0xff800000u, 0xff800000u, // -inf
        0x00800000u, 0x00800000u, 0x00800000u, 0x00800000u, // min()
        0x3f3504f3u, 0x3f3504f3u, 0x3f3504f3u, 0x3f3504f3u, // 1/sqrt(2)
        // ln(2) = 0x3fe62e42fefa39ef
        // ln(2) = doubleConstant< 1, 0x00062e42fefa39ef, -1>()
        //       = floatConstant< 1, 0x00317217(f7d>(), -1) + floatConstant< 1, 0x0077d1cd, -25>()
        //       = floatConstant< 1, 0x00318000(000>(), -1) + floatConstant<-1, 0x005e8083, -13>()
        0x3f318000u, 0x3f318000u, 0x3f318000u, 0x3f318000u, // round(ln(2) * 512) / 512
        0xb95e8083u, 0xb95e8083u, 0xb95e8083u, 0xb95e8083u, // ln(2) - round(ln(2) * 512) / 512
        0x3f000000u, 0x3f000000u, 0x3f000000u, 0x3f000000u, // 0.5
        0x3ede5bd9u, 0x3ede5bd9u, 0x3ede5bd9u, 0x3ede5bd9u, // log10(e)
        0x3fb8aa3bu, 0x3fb8aa3bu, 0x3fb8aa3bu, 0x3fb8aa3bu, // log2(e)
        // log10(2) = 0x3fd34413509f79ff
        //          = doubleConstant< 1, 0x00034413509f79ff, -2>()
        //          = floatConstant< 1, 0x001a209a(84fbcff8>(), -2) + floatConstant< 1, 0x0004fbcff(8>(), -26)
        //floatConstant< 1, 0x001a209a, -2>(), // log10(2)
        //floatConstant< 1, 0x001a209a, -2>(), // log10(2)
        //floatConstant< 1, 0x001a209a, -2>(), // log10(2)
        //floatConstant< 1, 0x001a209a, -2>(), // log10(2)
    };
}
}
