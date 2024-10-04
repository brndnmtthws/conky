/*  This file is part of the Vc library. {{{
Copyright © 2011-2015 Matthias Kretz <kretz@kde.org>

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

#include <Vc/cpuid.h>
#include <Vc/global.h>

namespace Vc_VERSIONED_NAMESPACE
{

CpuId::uint   CpuId::s_ecx0 = 0;
CpuId::uint   CpuId::s_logicalProcessors = 0;
CpuId::uint   CpuId::s_processorFeaturesC = 0;
CpuId::uint   CpuId::s_processorFeaturesD = 0;
CpuId::uint   CpuId::s_processorFeatures7B = 0;
CpuId::uint   CpuId::s_processorFeatures7C = 0;
CpuId::uint   CpuId::s_processorFeatures8C = 0;
CpuId::uint   CpuId::s_processorFeatures8D = 0;
CpuId::uint   CpuId::s_L1Instruction = 0;
CpuId::uint   CpuId::s_L1Data = 0;
CpuId::uint   CpuId::s_L2Data = 0;
CpuId::uint   CpuId::s_L3Data = 0;
CpuId::ushort CpuId::s_L1InstructionLineSize = 0;
CpuId::ushort CpuId::s_L1DataLineSize = 0;
CpuId::ushort CpuId::s_L2DataLineSize = 0;
CpuId::ushort CpuId::s_L3DataLineSize = 0;
CpuId::uint   CpuId::s_L1Associativity = 0;
CpuId::uint   CpuId::s_L2Associativity = 0;
CpuId::uint   CpuId::s_L3Associativity = 0;
CpuId::ushort CpuId::s_prefetch = 32; // The Intel ORM says that if Vc_CPUID(2) doesn't set the prefetch size it is 32
CpuId::uchar  CpuId::s_brandIndex = 0;
CpuId::uchar  CpuId::s_cacheLineSize = 0;
CpuId::uchar  CpuId::s_processorModel = 0;
CpuId::uchar  CpuId::s_processorFamily = 0;
CpuId::ProcessorType CpuId::s_processorType = CpuId::IntelReserved;
bool   CpuId::s_noL2orL3 = false;

#ifdef _WIN32

}
// better not include intrin.h inside the Vc namespace :)
#include <intrin.h>
namespace Vc_VERSIONED_NAMESPACE
{

#define Vc_CPUID(leaf) \
    do { \
        int out[4]; \
        __cpuid(out, leaf); \
        eax = out[0]; \
        ebx = out[1]; \
        ecx = out[2]; \
        edx = out[3]; \
    } while (false)
#define Vc_CPUID_C(leaf, ecx_)                                                           \
    do {                                                                                 \
        int out[4];                                                                      \
        __cpuidex(out, leaf, ecx_);                                                      \
        eax = out[0];                                                                    \
        ebx = out[1];                                                                    \
        ecx = out[2];                                                                    \
        edx = out[3];                                                                    \
    } while (false)
#elif defined(__i386__) && defined(__PIC__)
// %ebx may be the PIC register.
static inline void _Vc_cpuid(int leaf, unsigned int &eax, unsigned int &ebx, unsigned int &ecx, unsigned int &edx)
{
    int tmpb;
    asm("mov %%ebx, %[tmpb]\n\t"
        "cpuid\n\t"
        "mov %%ebx, %[ebx]\n\t"
        "mov %[tmpb], %%ebx\n\t"
        : [tmpb]"=m"(tmpb), "=a"(eax), [ebx] "=m"(ebx), "+c"(ecx), "=d"(edx)
        : [leaf] "a"(leaf)
      );
}
#define Vc_CPUID(leaf) \
    ecx = 0; \
    _Vc_cpuid(leaf, eax, ebx, ecx, edx)
#define Vc_CPUID_C(leaf, ecx_)                                                           \
    ecx = ecx_;                                                                          \
    _Vc_cpuid(leaf, eax, ebx, ecx, edx)
#else
#define Vc_CPUID(leaf) \
    __asm__("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(leaf))
#define Vc_CPUID_C(leaf, ecx_)                                                           \
    __asm__("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(leaf), "c"(ecx_))
#endif
static unsigned int CpuIdAmdAssociativityTable(int bits)
{
    switch (bits) {
    case 0x0: return 0;
    case 0x1: return 1;
    case 0x2: return 2;
    case 0x4: return 4;
    case 0x6: return 8;
    case 0x8: return 16;
    case 0xA: return 32;
    case 0xB: return 48;
    case 0xC: return 64;
    case 0xD: return 96;
    case 0xE: return 128;
    case 0xF: return 0xff;
    }
    return 0xffffffffu;
}

void CpuId::init()
{
    {
        static bool done = false;
        if (done) return;
        done = true;
    }
    uint eax, ebx, ecx, edx;

    Vc_CPUID(0);
    s_ecx0 = ecx;

    Vc_CPUID(1);
    s_processorFeaturesC = ecx;
    s_processorFeaturesD = edx;
    s_processorModel  = (eax & 0x000000f0) >> 4;
    s_processorFamily = (eax & 0x00000f00) >> 8;
    if (isAmd()) {
        if (s_processorFamily >= 0xf) {
            const uchar processorFamilyExt = (eax & 0x0ff00000) >> 16;
            s_processorFamily += processorFamilyExt;
            const uchar processorModelExt = (eax & 0x000f0000) >> 12;
            s_processorModel += processorModelExt;
        }
    } else if (s_processorFamily == 0xf) {
        const uchar processorFamilyExt = (eax & 0x0ff00000) >> 16;
        s_processorFamily += processorFamilyExt;
        const uchar processorModelExt = (eax & 0x000f0000) >> 12;
        s_processorModel += processorModelExt;
    } else if (s_processorFamily == 0x6) {
        const uchar processorModelExt = (eax & 0x000f0000) >> 12;
        s_processorModel += processorModelExt;
    } else if (s_processorFamily == 0xB) {
        const uchar processorFamilyExt = (eax & 0x0ff00000) >> 16;
        s_processorFamily += processorFamilyExt;
        const uchar processorModelExt = (eax & 0x000f0000) >> 12;
        s_processorModel += processorModelExt;
    }
    s_processorType = static_cast<ProcessorType>((eax & 0x00003000) >> 12);

    s_brandIndex = ebx & 0xff;
    ebx >>= 8;
    s_cacheLineSize = ebx & 0xff;
    ebx >>= 8;
    s_logicalProcessors = ebx & 0xff;

    Vc_CPUID_C(7, 0);
    s_processorFeatures7B = ebx;
    s_processorFeatures7C = ecx;

    Vc_CPUID(0x80000001);
    s_processorFeatures8C = ecx;
    s_processorFeatures8D = edx;

    if (isAmd()) {
        s_prefetch = cacheLineSize();

        Vc_CPUID(0x80000005);
        s_L1DataLineSize = ecx & 0xff;
        s_L1Data = (ecx >> 24) * 1024;
        s_L1Associativity = (ecx >> 16) & 0xff;
        s_L1InstructionLineSize = edx & 0xff;
        s_L1Instruction = (edx >> 24) * 1024;

        Vc_CPUID(0x80000006);
        s_L2DataLineSize = ecx & 0xff;
        s_L2Data = (ecx >> 16) * 1024;
        s_L2Associativity = CpuIdAmdAssociativityTable((ecx >> 12) & 0xf);
        s_L3DataLineSize = edx & 0xff;
        s_L3Data = (edx >> 18) * 512 * 1024;
        s_L3Associativity = CpuIdAmdAssociativityTable((ecx >> 12) & 0xf);
        return;
    }

    // Intel only
    int repeat = 0;
    bool checkLeaf4 = false;
    do {
        Vc_CPUID(2);
        if (repeat == 0) {
            if (eax == 0 && ebx == 0 && ecx == 0 && edx == 0) {
                // this is the case on MIC
                checkLeaf4 = true;
                break;
            }
            repeat = eax & 0xff;
        }
        if (0 == (0x80000000u & eax)) {
            for (int i = 0; i < 3; ++i) {
                eax >>= 8;
                interpret(eax & 0xff, &checkLeaf4);
            }
        }
        if (0 == (0x80000000u & ebx)) {
            for (int i = 0; i < 4; ++i) {
                interpret(ebx & 0xff, &checkLeaf4);
                ebx >>= 8;
            }
        }
        if (0 == (0x80000000u & ecx)) {
            for (int i = 0; i < 4; ++i) {
                interpret(ecx & 0xff, &checkLeaf4);
                ecx >>= 8;
            }
        }
        if (0 == (0x80000000u & edx)) {
            for (int i = 0; i < 4; ++i) {
                interpret(edx & 0xff, &checkLeaf4);
                edx >>= 8;
            }
        }
    } while (--repeat > 0);
    if (checkLeaf4) {
        s_prefetch = cacheLineSize();
        if (s_prefetch == 0) {
            s_prefetch = 64;
        }
        eax = 1;
        for (int i = 0; eax & 0x1f; ++i) {
            Vc_CPUID_C(4, i);
            const int cacheLevel = (eax >> 5) & 7;
            //const int sharedBy = 1 + ((eax >> 14) & 0xfff);
            const int linesize = 1 + (ebx & 0xfff);   ebx >>= 12;
            const int partitions = 1 + (ebx & 0x3ff); ebx >>= 10;
            const int ways = 1 + (ebx & 0x3ff);
            const int sets = 1 + ecx;
            const int size = ways * partitions * linesize * sets;
            switch (eax & 0x1f) {
                case 1: // data cache
                    switch (cacheLevel) {
                        case 1:
                            s_L1Data = size;
                            s_L1DataLineSize = linesize;
                            s_L1Associativity = ways;
                            break;
                        case 2:
                            s_L2Data = size;
                            s_L2DataLineSize = linesize;
                            s_L2Associativity = ways;
                            break;
                        case 3:
                            s_L3Data = size;
                            s_L3DataLineSize = linesize;
                            s_L3Associativity = ways;
                            break;
                    }
                    break;
                case 2: // instruction cache
                    switch (cacheLevel) {
                        case 1:
                            s_L1Instruction = size;
                            s_L1InstructionLineSize = linesize;
                            break;
                    }
                    break;
                case 3: // unified cache
                    switch (cacheLevel) {
                        case 1:
                            s_L1Data = size;// / sharedBy;
                            s_L1DataLineSize = linesize;
                            s_L1Associativity = ways;
                            break;
                        case 2:
                            s_L2Data = size;// / sharedBy;
                            s_L2DataLineSize = linesize;
                            s_L2Associativity = ways;
                            break;
                        case 3:
                            s_L3Data = size;// / sharedBy;
                            s_L3DataLineSize = linesize;
                            s_L3Associativity = ways;
                            break;
                    }
                    break;
                case 0: // no more caches
                    break;
                default: // reserved
                    break;
            }
        }
    }
}

void CpuId::interpret(uchar byte, bool *checkLeaf4)
{
    switch (byte) {
    case 0x06:
        s_L1Instruction = 8 * 1024;
        s_L1InstructionLineSize = 32;
        s_L1Associativity = 4;
        break;
    case 0x08:
        s_L1Instruction = 16 * 1024;
        s_L1InstructionLineSize = 32;
        s_L1Associativity = 4;
        break;
    case 0x09:
        s_L1Instruction = 32 * 1024;
        s_L1InstructionLineSize = 64;
        s_L1Associativity = 4;
        break;
    case 0x0A:
        s_L1Data = 8 * 1024;
        s_L1DataLineSize = 32;
        s_L1Associativity = 2;
        break;
    case 0x0C:
        s_L1Data = 16 * 1024;
        s_L1DataLineSize = 32;
        s_L1Associativity = 4;
        break;
    case 0x0D:
        s_L1Data = 16 * 1024;
        s_L1DataLineSize = 64;
        s_L1Associativity = 4;
        break;
    case 0x0E:
        s_L1Data = 24 * 1024;
        s_L1DataLineSize = 64;
        s_L1Associativity = 6;
        break;
    case 0x21:
        s_L2Data = 256 * 1024;
        s_L2DataLineSize = 64;
        s_L2Associativity = 8;
        break;
    case 0x22:
        s_L3Data = 512 * 1024;
        s_L3DataLineSize = 64;
        s_L3Associativity = 4;
        break;
    case 0x23:
        s_L3Data = 1024 * 1024;
        s_L3DataLineSize = 64;
        s_L3Associativity = 8;
        break;
    case 0x25:
        s_L3Data = 2 * 1024 * 1024;
        s_L3DataLineSize = 64;
        s_L3Associativity = 8;
        break;
    case 0x29:
        s_L3Data = 4 * 1024 * 1024;
        s_L3DataLineSize = 64;
        s_L3Associativity = 8;
        break;
    case 0x2C:
        s_L1Data = 32 * 1024;
        s_L1DataLineSize = 64;
        s_L1Associativity = 8;
        break;
    case 0x30:
        s_L1Data = 32 * 1024;
        s_L1DataLineSize = 64;
        s_L1Associativity = 8;
        break;
    case 0x40:
        s_noL2orL3 = true;
        break;
    case 0x41:
        s_L2Data = 128 * 1024;
        s_L2DataLineSize = 32;
        s_L2Associativity = 4;
        break;
    case 0x42:
        s_L2Data = 256 * 1024;
        s_L2DataLineSize = 32;
        s_L2Associativity = 4;
        break;
    case 0x43:
        s_L2Data = 512 * 1024;
        s_L2DataLineSize = 32;
        s_L2Associativity = 4;
        break;
    case 0x44:
        s_L2Data = 1024 * 1024;
        s_L2DataLineSize = 32;
        s_L2Associativity = 4;
        break;
    case 0x45:
        s_L2Data = 2 * 1024 * 1024;
        s_L2DataLineSize = 32;
        s_L2Associativity = 4;
        break;
    case 0x46:
        s_L3Data = 4 * 1024 * 1024;
        s_L3DataLineSize = 64;
        s_L3Associativity = 4;
        break;
    case 0x47:
        s_L3Data = 8 * 1024 * 1024;
        s_L3DataLineSize = 64;
        s_L3Associativity = 8;
        break;
    case 0x48:
        s_L2Data = 3 * 1024 * 1024;
        s_L2DataLineSize = 64;
        s_L2Associativity = 12;
        break;
    case 0x49:
        if (s_processorFamily == 0xf && s_processorModel == 0x6) {
            s_L3Data = 4 * 1024 * 1024;
            s_L3DataLineSize = 64;
            s_L3Associativity = 16;
        } else {
            s_L2Data = 4 * 1024 * 1024;
            s_L2DataLineSize = 64;
            s_L2Associativity = 16;
        }
        break;
    case 0x4A:
        s_L3Data = 6 * 1024 * 1024;
        s_L3DataLineSize = 64;
        s_L3Associativity = 12;
        break;
    case 0x4B:
        s_L3Data = 8 * 1024 * 1024;
        s_L3DataLineSize = 64;
        s_L3Associativity = 16;
        break;
    case 0x4C:
        s_L3Data = 12 * 1024 * 1024;
        s_L3DataLineSize = 64;
        s_L3Associativity = 12;
        break;
    case 0x4D:
        s_L3Data = 16 * 1024 * 1024;
        s_L3DataLineSize = 64;
        s_L3Associativity = 16;
        break;
    case 0x4E:
        s_L2Data = 6 * 1024 * 1024;
        s_L2DataLineSize = 64;
        s_L2Associativity = 24;
        break;
    case 0x60:
        s_L1Data = 16 * 1024;
        s_L1DataLineSize = 64;
        s_L1Associativity = 8;
        break;
    case 0x66:
        s_L1Data = 8 * 1024;
        s_L1DataLineSize = 64;
        s_L1Associativity = 4;
        break;
    case 0x67:
        s_L1Data = 16 * 1024;
        s_L1DataLineSize = 64;
        s_L1Associativity = 4;
        break;
    case 0x68:
        s_L1Data = 32 * 1024;
        s_L1DataLineSize = 64;
        s_L1Associativity = 4;
        break;
    case 0x78:
        s_L2Data = 1024 * 1024;
        s_L2DataLineSize = 64;
        s_L2Associativity = 4;
        break;
    case 0x79:
        s_L2Data = 128 * 1024;
        s_L2DataLineSize = 64;
        s_L2Associativity = 8;
        break;
    case 0x7A:
        s_L2Data = 256 * 1024;
        s_L2DataLineSize = 64;
        s_L2Associativity = 8;
        break;
    case 0x7B:
        s_L2Data = 512 * 1024;
        s_L2DataLineSize = 64;
        s_L2Associativity = 8;
        break;
    case 0x7C:
        s_L2Data = 1024 * 1024;
        s_L2DataLineSize = 64;
        s_L2Associativity = 8;
        break;
    case 0x7D:
        s_L2Data = 2 * 1024 * 1024;
        s_L2DataLineSize = 64;
        s_L2Associativity = 8;
        break;
    case 0x7F:
        s_L2Data = 512 * 1024;
        s_L2DataLineSize = 64;
        s_L2Associativity = 2;
        break;
    case 0x80:
        s_L2Data = 512 * 1024;
        s_L2DataLineSize = 64;
        s_L2Associativity = 8;
        break;
    case 0x82:
        s_L2Data = 256 * 1024;
        s_L2DataLineSize = 32;
        s_L2Associativity = 8;
        break;
    case 0x83:
        s_L2Data = 512 * 1024;
        s_L2DataLineSize = 32;
        s_L2Associativity = 8;
        break;
    case 0x84:
        s_L2Data = 1024 * 1024;
        s_L2DataLineSize = 32;
        s_L2Associativity = 8;
        break;
    case 0x85:
        s_L2Data = 2 * 1024 * 1024;
        s_L2DataLineSize = 32;
        s_L2Associativity = 8;
        break;
    case 0x86:
        s_L2Data = 512 * 1024;
        s_L2DataLineSize = 64;
        s_L2Associativity = 4;
        break;
    case 0x87:
        s_L2Data = 1024 * 1024;
        s_L2DataLineSize = 64;
        s_L2Associativity = 8;
        break;
    case 0xD0:
        s_L3Data = 512 * 1024;
        s_L3DataLineSize = 64;
        s_L3Associativity = 4;
        break;
    case 0xD1:
        s_L3Data = 1024 * 1024;
        s_L3DataLineSize = 64;
        s_L3Associativity = 4;
        break;
    case 0xD2:
        s_L3Data = 2 * 1024 * 1024;
        s_L3DataLineSize = 64;
        s_L3Associativity = 4;
        break;
    case 0xD6:
        s_L3Data = 1024 * 1024;
        s_L3DataLineSize = 64;
        s_L3Associativity = 8;
        break;
    case 0xD7:
        s_L3Data = 2 * 1024 * 1024;
        s_L3DataLineSize = 64;
        s_L3Associativity = 8;
        break;
    case 0xD8:
        s_L3Data = 4 * 1024 * 1024;
        s_L3DataLineSize = 64;
        s_L3Associativity = 8;
        break;
    case 0xDC:
        s_L3Data = 3 * 512 * 1024;
        s_L3DataLineSize = 64;
        s_L3Associativity = 12;
        break;
    case 0xDD:
        s_L3Data = 3 * 1024 * 1024;
        s_L3DataLineSize = 64;
        s_L3Associativity = 12;
        break;
    case 0xDE:
        s_L3Data = 6 * 1024 * 1024;
        s_L3DataLineSize = 64;
        s_L3Associativity = 12;
        break;
    case 0xE2:
        s_L3Data = 2 * 1024 * 1024;
        s_L3DataLineSize = 64;
        s_L3Associativity = 16;
        break;
    case 0xE3:
        s_L3Data = 4 * 1024 * 1024;
        s_L3DataLineSize = 64;
        s_L3Associativity = 16;
        break;
    case 0xE4:
        s_L3Data = 8 * 1024 * 1024;
        s_L3DataLineSize = 64;
        s_L3Associativity = 16;
        break;
    case 0xEA:
        s_L3Data = 12 * 1024 * 1024;
        s_L3DataLineSize = 64;
        s_L3Associativity = 24;
        break;
    case 0xEB:
        s_L3Data = 18 * 1024 * 1024;
        s_L3DataLineSize = 64;
        s_L3Associativity = 24;
        break;
    case 0xEC:
        s_L3Data = 24 * 1024 * 1024;
        s_L3DataLineSize = 64;
        s_L3Associativity = 24;
        break;
    case 0xF0:
        s_prefetch = 64;
        break;
    case 0xF1:
        s_prefetch = 128;
        break;
    case 0xFF:
        // we have to use Vc_CPUID(4) to find out
        *checkLeaf4 = true;
        break;
    default:
        break;
    }
}

}

// vim: sw=4 sts=4 et tw=100
