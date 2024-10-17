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

#ifndef VC_COMMON_GATHERIMPLEMENTATION_H_
#define VC_COMMON_GATHERIMPLEMENTATION_H_

#include "macros.h"

namespace Vc_VERSIONED_NAMESPACE
{
namespace Common
{

enum class GatherScatterImplementation : int {
    SimpleLoop,
    SetIndexZero,
    BitScanLoop,
    PopcntSwitch
};

using SimpleLoopT   = std::integral_constant<GatherScatterImplementation, GatherScatterImplementation::SimpleLoop>;
using SetIndexZeroT = std::integral_constant<GatherScatterImplementation, GatherScatterImplementation::SetIndexZero>;
using BitScanLoopT  = std::integral_constant<GatherScatterImplementation, GatherScatterImplementation::BitScanLoop>;
using PopcntSwitchT = std::integral_constant<GatherScatterImplementation, GatherScatterImplementation::PopcntSwitch>;

template <typename V, typename MT, typename IT>
Vc_ALWAYS_INLINE void executeGather(SetIndexZeroT,
                                    V &v,
                                    const MT *mem,
                                    IT &&indexes_,
                                    typename V::MaskArgument mask)
{
    auto indexes = std::forward<IT>(indexes_);
    indexes.setZeroInverted(static_cast<decltype(!indexes)>(mask));
    const V tmp(mem, indexes);
    where(mask) | v = tmp;
}

template <typename V, typename MT, typename IT>
Vc_ALWAYS_INLINE void executeGather(SimpleLoopT, V &v, const MT *mem, const IT &indexes,
                                    const typename V::MaskArgument mask)
{
    if (Vc_IS_UNLIKELY(mask.isEmpty())) {
        return;
    }
#if defined Vc_GCC && Vc_GCC >= 0x40900
    // GCC 4.8 doesn't support dependent type and constexpr vector_size argument
    constexpr std::size_t Sizeof = sizeof(V);
    using Builtin [[gnu::vector_size(Sizeof)]] = typename V::value_type;
    Builtin tmp = reinterpret_cast<Builtin>(v.data());
    Common::unrolled_loop<std::size_t, 0, V::Size>([&](std::size_t i) {
        if (mask[i]) {
            tmp[i] = mem[indexes[i]];
        }
    });
    v.data() = reinterpret_cast<typename V::VectorType>(tmp);
#else
    Common::unrolled_loop<std::size_t, 0, V::Size>([&](std::size_t i) {
        if (mask[i])
            v[i] = mem[indexes[i]];
    });
#endif
}

template <typename V, typename MT, typename IT>
Vc_ALWAYS_INLINE void executeGather(BitScanLoopT,
                                    V &v,
                                    const MT *mem,
                                    const IT &indexes,
                                    typename V::MaskArgument mask)
{
#ifdef Vc_GNU_ASM
    size_t bits = mask.toInt();
    while (Vc_IS_LIKELY(bits > 0)) {
        size_t i, j;
        asm("bsf %[bits],%[i]\n\t"
            "bsr %[bits],%[j]\n\t"
            "btr %[i],%[bits]\n\t"
            "btr %[j],%[bits]\n\t"
            : [i] "=r"(i), [j] "=r"(j), [bits] "+r"(bits));
        v[i] = mem[indexes[i]];
        v[j] = mem[indexes[j]];
    }
#else
    // Alternative from Vc::SSE (0.7)
    int bits = mask.toInt();
    while (bits) {
        const int i = _bit_scan_forward(bits);
	bits &= bits - 1;
	v[i] = mem[indexes[i]];
    }
#endif  // Vc_GNU_ASM
}

template <typename V, typename MT, typename IT>
Vc_ALWAYS_INLINE void executeGather(PopcntSwitchT,
                                    V &v,
                                    const MT *mem,
                                    const IT &indexes,
                                    typename V::MaskArgument mask,
                                    enable_if<V::Size == 16> = nullarg)
{
    unsigned int bits = mask.toInt();
    unsigned int low, high = 0;
    switch (Vc::Detail::popcnt16(bits)) {
    case 16:
        v.gather(mem, indexes);
        break;
    case 15:
        low = _bit_scan_forward(bits);
        bits ^= 1 << low;
        v[low] = mem[indexes[low]];
        // fallthrough
    case 14:
        high = _bit_scan_reverse(bits);
        v[high] = mem[indexes[high]];
        high = (1 << high);
        // fallthrough
    case 13:
        low = _bit_scan_forward(bits);
        bits ^= high | (1 << low);
        v[low] = mem[indexes[low]];
        // fallthrough
    case 12:
        high = _bit_scan_reverse(bits);
        v[high] = mem[indexes[high]];
        high = (1 << high);
        // fallthrough
    case 11:
        low = _bit_scan_forward(bits);
        bits ^= high | (1 << low);
        v[low] = mem[indexes[low]];
        // fallthrough
    case 10:
        high = _bit_scan_reverse(bits);
        v[high] = mem[indexes[high]];
        high = (1 << high);
        // fallthrough
    case 9:
        low = _bit_scan_forward(bits);
        bits ^= high | (1 << low);
        v[low] = mem[indexes[low]];
        // fallthrough
    case 8:
        high = _bit_scan_reverse(bits);
        v[high] = mem[indexes[high]];
        high = (1 << high);
        // fallthrough
    case 7:
        low = _bit_scan_forward(bits);
        bits ^= high | (1 << low);
        v[low] = mem[indexes[low]];
        // fallthrough
    case 6:
        high = _bit_scan_reverse(bits);
        v[high] = mem[indexes[high]];
        high = (1 << high);
        // fallthrough
    case 5:
        low = _bit_scan_forward(bits);
        bits ^= high | (1 << low);
        v[low] = mem[indexes[low]];
        // fallthrough
    case 4:
        high = _bit_scan_reverse(bits);
        v[high] = mem[indexes[high]];
        high = (1 << high);
        // fallthrough
    case 3:
        low = _bit_scan_forward(bits);
        bits ^= high | (1 << low);
        v[low] = mem[indexes[low]];
        // fallthrough
    case 2:
        high = _bit_scan_reverse(bits);
        v[high] = mem[indexes[high]];
        // fallthrough
    case 1:
        low = _bit_scan_forward(bits);
        v[low] = mem[indexes[low]];
        // fallthrough
    case 0:
        break;
    }
}
template <typename V, typename MT, typename IT>
Vc_ALWAYS_INLINE void executeGather(PopcntSwitchT,
                                    V &v,
                                    const MT *mem,
                                    const IT &indexes,
                                    typename V::MaskArgument mask,
                                    enable_if<V::Size == 8> = nullarg)
{
    unsigned int bits = mask.toInt();
    unsigned int low, high = 0;
    switch (Vc::Detail::popcnt8(bits)) {
    case 8:
        v.gather(mem, indexes);
        break;
    case 7:
        low = _bit_scan_forward(bits);
        bits ^= 1 << low;
        v[low] = mem[indexes[low]];
        // fallthrough
    case 6:
        high = _bit_scan_reverse(bits);
        v[high] = mem[indexes[high]];
        high = (1 << high);
        // fallthrough
    case 5:
        low = _bit_scan_forward(bits);
        bits ^= high | (1 << low);
        v[low] = mem[indexes[low]];
        // fallthrough
    case 4:
        high = _bit_scan_reverse(bits);
        v[high] = mem[indexes[high]];
        high = (1 << high);
        // fallthrough
    case 3:
        low = _bit_scan_forward(bits);
        bits ^= high | (1 << low);
        v[low] = mem[indexes[low]];
        // fallthrough
    case 2:
        high = _bit_scan_reverse(bits);
        v[high] = mem[indexes[high]];
        // fallthrough
    case 1:
        low = _bit_scan_forward(bits);
        v[low] = mem[indexes[low]];
        // fallthrough
    case 0:
        break;
    }
}
template <typename V, typename MT, typename IT>
Vc_ALWAYS_INLINE void executeGather(PopcntSwitchT,
                                    V &v,
                                    const MT *mem,
                                    const IT &indexes,
                                    typename V::MaskArgument mask,
                                    enable_if<V::Size == 4> = nullarg)
{
    unsigned int bits = mask.toInt();
    unsigned int low, high = 0;
    switch (Vc::Detail::popcnt4(bits)) {
    case 4:
        v.gather(mem, indexes);
        break;
    case 3:
        low = _bit_scan_forward(bits);
        bits ^= 1 << low;
        v[low] = mem[indexes[low]];
        // fallthrough
    case 2:
        high = _bit_scan_reverse(bits);
        v[high] = mem[indexes[high]];
        // fallthrough
    case 1:
        low = _bit_scan_forward(bits);
        v[low] = mem[indexes[low]];
        // fallthrough
    case 0:
        break;
    }
}
template <typename V, typename MT, typename IT>
Vc_ALWAYS_INLINE void executeGather(PopcntSwitchT,
                                    V &v,
                                    const MT *mem,
                                    const IT &indexes,
                                    typename V::MaskArgument mask,
                                    enable_if<V::Size == 2> = nullarg)
{
    unsigned int bits = mask.toInt();
    unsigned int low;
    switch (Vc::Detail::popcnt4(bits)) {
    case 2:
        v.gather(mem, indexes);
        break;
    case 1:
        low = _bit_scan_forward(bits);
        v[low] = mem[indexes[low]];
        // fallthrough
    case 0:
        break;
    }
}

}  // namespace Common
}  // namespace Vc

#endif // VC_COMMON_GATHERIMPLEMENTATION_H_
