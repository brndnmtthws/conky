/*  This file is part of the Vc library. {{{
Copyright © 2010-2015 Matthias Kretz <kretz@kde.org>

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

#include <Vc/global.h>
#include <Vc/cpuid.h>
#include <Vc/support.h>

#ifdef Vc_MSVC
#include <intrin.h>
#endif

#if defined(Vc_GCC) && Vc_GCC >= 0x40400
#define Vc_TARGET_NO_SIMD __attribute__((target("no-sse2,no-avx")))
#else
#define Vc_TARGET_NO_SIMD
#endif

namespace Vc_VERSIONED_NAMESPACE
{

Vc_TARGET_NO_SIMD
static inline bool xgetbvCheck(unsigned int bits)
{
#if defined(Vc_MSVC) && Vc_MSVC >= 160040219 // MSVC 2010 SP1 introduced _xgetbv
    unsigned long long xcrFeatureMask = _xgetbv(_XCR_XFEATURE_ENABLED_MASK);
    return (xcrFeatureMask & bits) == bits;
#elif defined(Vc_GNU_ASM) && !defined(Vc_NO_XGETBV)
    unsigned int eax;
    asm("xgetbv" : "=a"(eax) : "c"(0) : "edx");
    return (eax & bits) == bits;
#else
    // can't check, but if OSXSAVE is true let's assume it'll work
    return bits > 0; // ignore 'warning: unused parameter'
#endif
}

Vc_TARGET_NO_SIMD
bool Vc_VDECL isImplementationSupported(Implementation impl)
{
    CpuId::init();

    switch (impl) {
    case ScalarImpl:
        return true;
    case SSE2Impl:
        return CpuId::hasSse2();
    case SSE3Impl:
        return CpuId::hasSse3();
    case SSSE3Impl:
        return CpuId::hasSsse3();
    case SSE41Impl:
        return CpuId::hasSse41();
    case SSE42Impl:
        return CpuId::hasSse42();
    case AVXImpl:
        return CpuId::hasOsxsave() && CpuId::hasAvx() && xgetbvCheck(0x6);
    case AVX2Impl:
        return CpuId::hasOsxsave() && CpuId::hasAvx2() && xgetbvCheck(0x6);
    case MICImpl:
        return CpuId::processorFamily() == 0xB && CpuId::processorModel() == 0x1
            && CpuId::isIntel();
    case ImplementationMask:
        return false;
    }
    return false;
}

Vc_TARGET_NO_SIMD
Vc::Implementation bestImplementationSupported()
{
    CpuId::init();

    if (CpuId::processorFamily() == 0xB && CpuId::processorModel() == 0x1
            && CpuId::isIntel()) {
        return Vc::MICImpl;
    }
    if (!CpuId::hasSse2 ()) return Vc::ScalarImpl;
    if (!CpuId::hasSse3 ()) return Vc::SSE2Impl;
    if (!CpuId::hasSsse3()) return Vc::SSE3Impl;
    if (!CpuId::hasSse41()) return Vc::SSSE3Impl;
    if (!CpuId::hasSse42()) return Vc::SSE41Impl;
    if (CpuId::hasAvx() && CpuId::hasOsxsave() && xgetbvCheck(0x6)) {
        if (!CpuId::hasAvx2()) return Vc::AVXImpl;
        return Vc::AVX2Impl;
    }
    return Vc::SSE42Impl;
}

Vc_TARGET_NO_SIMD
unsigned int extraInstructionsSupported()
{
    unsigned int flags = 0;
    if (CpuId::hasF16c()) flags |= Vc::Float16cInstructions;
    if (CpuId::hasFma4()) flags |= Vc::Fma4Instructions;
    if (CpuId::hasXop ()) flags |= Vc::XopInstructions;
    if (CpuId::hasPopcnt()) flags |= Vc::PopcntInstructions;
    if (CpuId::hasSse4a()) flags |= Vc::Sse4aInstructions;
    if (CpuId::hasFma ()) flags |= Vc::FmaInstructions;
    if (CpuId::hasBmi2()) flags |= Vc::Bmi2Instructions;
    if (CpuId::hasOsxsave() && CpuId::hasAvx() && xgetbvCheck(0x6)) flags |= Vc::VexInstructions;
    //if (CpuId::hasPclmulqdq()) flags |= Vc::PclmulqdqInstructions;
    //if (CpuId::hasAes()) flags |= Vc::AesInstructions;
    //if (CpuId::hasRdrand()) flags |= Vc::RdrandInstructions;
    return flags;
}

}

#undef Vc_TARGET_NO_SIMD

// vim: sw=4 sts=4 et tw=100
