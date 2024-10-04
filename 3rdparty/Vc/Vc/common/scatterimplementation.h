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

#ifndef VC_COMMON_SCATTERIMPLEMENTATION_H_
#define VC_COMMON_SCATTERIMPLEMENTATION_H_

#include "gatherimplementation.h"
#include "macros.h"

namespace Vc_VERSIONED_NAMESPACE
{
namespace Common
{

template <typename V, typename MT, typename IT>
Vc_ALWAYS_INLINE void executeScatter(SetIndexZeroT,
                                    V &v,
                                    MT *mem,
                                    IT indexes,
                                    typename V::MaskArgument mask)
{
    indexes.setZeroInverted(static_cast<typename IT::Mask>(mask));
    // Huh?
    const V tmp(mem, indexes);
    where(mask) | v = tmp;
}

template <typename V, typename MT, typename IT>
Vc_ALWAYS_INLINE void executeScatter(SimpleLoopT,
                                    V &v,
                                    MT *mem,
                                    const IT &indexes,
                                    typename V::MaskArgument mask)
{
    if (Vc_IS_UNLIKELY(mask.isEmpty())) {
        return;
    }
    Common::unrolled_loop<std::size_t, 0, V::Size>([&](std::size_t i) {
        if (mask[i])
            mem[indexes[i]] = v[i];
    });
}

template <typename V, typename MT, typename IT>
Vc_ALWAYS_INLINE void executeScatter(BitScanLoopT,
                                    V &v,
                                    MT *mem,
                                    const IT &indexes,
                                    typename V::MaskArgument mask)
{
    size_t bits = mask.toInt();
    while (Vc_IS_LIKELY(bits > 0)) {
        size_t i, j;
        asm("bsf %[bits],%[i]\n\t"
            "bsr %[bits],%[j]\n\t"
            "btr %[i],%[bits]\n\t"
            "btr %[j],%[bits]\n\t"
            : [i] "=r"(i), [j] "=r"(j), [bits] "+r"(bits));
        mem[indexes[i]] = v[i];
        mem[indexes[j]] = v[j];
    }

    /* Alternative from Vc::SSE (0.7)
    int bits = mask.toInt();
    while (bits) {
        const int i = _bit_scan_forward(bits);
        bits ^= (1 << i); // btr?
        mem[indexes[i]] = v[i];
    }
    */
}

template <typename V, typename MT, typename IT>
Vc_ALWAYS_INLINE void executeScatter(PopcntSwitchT,
                                    V &v,
                                    MT *mem,
                                    const IT &indexes,
                                    typename V::MaskArgument mask,
                                    enable_if<V::Size == 16> = nullarg)
{
    unsigned int bits = mask.toInt();
    unsigned int low, high = 0;
    switch (Vc::Detail::popcnt16(bits)) {
    case 16:
        v.scatter(mem, indexes);
        break;
    case 15:
        low = _bit_scan_forward(bits);
        bits ^= 1 << low;
        mem[indexes[low]] = v[low];
    case 14:
        high = _bit_scan_reverse(bits);
        mem[indexes[high]] = v[high];
        high = (1 << high);
    case 13:
        low = _bit_scan_forward(bits);
        bits ^= high | (1 << low);
        mem[indexes[low]] = v[low];
    case 12:
        high = _bit_scan_reverse(bits);
        mem[indexes[high]] = v[high];
        high = (1 << high);
    case 11:
        low = _bit_scan_forward(bits);
        bits ^= high | (1 << low);
        mem[indexes[low]] = v[low];
    case 10:
        high = _bit_scan_reverse(bits);
        mem[indexes[high]] = v[high];
        high = (1 << high);
    case 9:
        low = _bit_scan_forward(bits);
        bits ^= high | (1 << low);
        mem[indexes[low]] = v[low];
    case 8:
        high = _bit_scan_reverse(bits);
        mem[indexes[high]] = v[high];
        high = (1 << high);
    case 7:
        low = _bit_scan_forward(bits);
        bits ^= high | (1 << low);
        mem[indexes[low]] = v[low];
    case 6:
        high = _bit_scan_reverse(bits);
        mem[indexes[high]] = v[high];
        high = (1 << high);
    case 5:
        low = _bit_scan_forward(bits);
        bits ^= high | (1 << low);
        mem[indexes[low]] = v[low];
    case 4:
        high = _bit_scan_reverse(bits);
        mem[indexes[high]] = v[high];
        high = (1 << high);
    case 3:
        low = _bit_scan_forward(bits);
        bits ^= high | (1 << low);
        mem[indexes[low]] = v[low];
    case 2:
        high = _bit_scan_reverse(bits);
        mem[indexes[high]] = v[high];
    case 1:
        low = _bit_scan_forward(bits);
        mem[indexes[low]] = v[low];
    case 0:
        break;
    }
}
template <typename V, typename MT, typename IT>
Vc_ALWAYS_INLINE void executeScatter(PopcntSwitchT,
                                    V &v,
                                    MT *mem,
                                    const IT &indexes,
                                    typename V::MaskArgument mask,
                                    enable_if<V::Size == 8> = nullarg)
{
    unsigned int bits = mask.toInt();
    unsigned int low, high = 0;
    switch (Vc::Detail::popcnt8(bits)) {
    case 8:
        v.scatter(mem, indexes);
        break;
    case 7:
        low = _bit_scan_forward(bits);
        bits ^= 1 << low;
        mem[indexes[low]] = v[low];
    case 6:
        high = _bit_scan_reverse(bits);
        mem[indexes[high]] = v[high];
        high = (1 << high);
    case 5:
        low = _bit_scan_forward(bits);
        bits ^= high | (1 << low);
        mem[indexes[low]] = v[low];
    case 4:
        high = _bit_scan_reverse(bits);
        mem[indexes[high]] = v[high];
        high = (1 << high);
    case 3:
        low = _bit_scan_forward(bits);
        bits ^= high | (1 << low);
        mem[indexes[low]] = v[low];
    case 2:
        high = _bit_scan_reverse(bits);
        mem[indexes[high]] = v[high];
    case 1:
        low = _bit_scan_forward(bits);
        mem[indexes[low]] = v[low];
    case 0:
        break;
    }
}
template <typename V, typename MT, typename IT>
Vc_ALWAYS_INLINE void executeScatter(PopcntSwitchT,
                                    V &v,
                                    MT *mem,
                                    const IT &indexes,
                                    typename V::MaskArgument mask,
                                    enable_if<V::Size == 4> = nullarg)
{
    unsigned int bits = mask.toInt();
    unsigned int low, high = 0;
    switch (Vc::Detail::popcnt4(bits)) {
    case 4:
        v.scatter(mem, indexes);
        break;
    case 3:
        low = _bit_scan_forward(bits);
        bits ^= 1 << low;
        mem[indexes[low]] = v[low];
    case 2:
        high = _bit_scan_reverse(bits);
        mem[indexes[high]] = v[high];
    case 1:
        low = _bit_scan_forward(bits);
        mem[indexes[low]] = v[low];
    case 0:
        break;
    }
}
template <typename V, typename MT, typename IT>
Vc_ALWAYS_INLINE void executeScatter(PopcntSwitchT,
                                    V &v,
                                    MT *mem,
                                    const IT &indexes,
                                    typename V::MaskArgument mask,
                                    enable_if<V::Size == 2> = nullarg)
{
    unsigned int bits = mask.toInt();
    unsigned int low;
    switch (Vc::Detail::popcnt4(bits)) {
    case 2:
        v.scatter(mem, indexes);
        break;
    case 1:
        low = _bit_scan_forward(bits);
        mem[indexes[low]] = v[low];
    case 0:
        break;
    }
}

}  // namespace Common
}  // namespace Vc

#endif // VC_COMMON_SCATTERIMPLEMENTATION_H_
