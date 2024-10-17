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

#ifndef VC_SSE_LIMITS_H_
#define VC_SSE_LIMITS_H_

#include "intrinsics.h"
#include "types.h"
#include "macros.h"

namespace std
{
template<> struct numeric_limits< ::Vc::SSE::ushort_v> : public numeric_limits<unsigned short>
{
    static Vc_INTRINSIC Vc_CONST ::Vc::SSE::ushort_v max()           Vc_NOEXCEPT { return ::Vc::SSE::_mm_setallone_si128(); }
    static Vc_INTRINSIC Vc_CONST ::Vc::SSE::ushort_v min()           Vc_NOEXCEPT { return ::Vc::SSE::ushort_v::Zero(); }
    static Vc_INTRINSIC Vc_CONST ::Vc::SSE::ushort_v lowest()        Vc_NOEXCEPT { return min(); }
    static Vc_INTRINSIC Vc_CONST ::Vc::SSE::ushort_v epsilon()       Vc_NOEXCEPT { return ::Vc::SSE::ushort_v::Zero(); }
    static Vc_INTRINSIC Vc_CONST ::Vc::SSE::ushort_v round_error()   Vc_NOEXCEPT { return ::Vc::SSE::ushort_v::Zero(); }
    static Vc_INTRINSIC Vc_CONST ::Vc::SSE::ushort_v infinity()      Vc_NOEXCEPT { return ::Vc::SSE::ushort_v::Zero(); }
    static Vc_INTRINSIC Vc_CONST ::Vc::SSE::ushort_v quiet_NaN()     Vc_NOEXCEPT { return ::Vc::SSE::ushort_v::Zero(); }
    static Vc_INTRINSIC Vc_CONST ::Vc::SSE::ushort_v signaling_NaN() Vc_NOEXCEPT { return ::Vc::SSE::ushort_v::Zero(); }
    static Vc_INTRINSIC Vc_CONST ::Vc::SSE::ushort_v denorm_min()    Vc_NOEXCEPT { return ::Vc::SSE::ushort_v::Zero(); }
};
template<> struct numeric_limits< ::Vc::SSE::short_v> : public numeric_limits<short>
{
    static Vc_INTRINSIC Vc_CONST ::Vc::SSE::short_v max()           Vc_NOEXCEPT { return _mm_srli_epi16(::Vc::SSE::_mm_setallone_si128(), 1); }
    static Vc_INTRINSIC Vc_CONST ::Vc::SSE::short_v min()           Vc_NOEXCEPT { return ::Vc::SSE::setmin_epi16(); }
    static Vc_INTRINSIC Vc_CONST ::Vc::SSE::short_v lowest()        Vc_NOEXCEPT { return min(); }
    static Vc_INTRINSIC Vc_CONST ::Vc::SSE::short_v epsilon()       Vc_NOEXCEPT { return ::Vc::SSE::short_v::Zero(); }
    static Vc_INTRINSIC Vc_CONST ::Vc::SSE::short_v round_error()   Vc_NOEXCEPT { return ::Vc::SSE::short_v::Zero(); }
    static Vc_INTRINSIC Vc_CONST ::Vc::SSE::short_v infinity()      Vc_NOEXCEPT { return ::Vc::SSE::short_v::Zero(); }
    static Vc_INTRINSIC Vc_CONST ::Vc::SSE::short_v quiet_NaN()     Vc_NOEXCEPT { return ::Vc::SSE::short_v::Zero(); }
    static Vc_INTRINSIC Vc_CONST ::Vc::SSE::short_v signaling_NaN() Vc_NOEXCEPT { return ::Vc::SSE::short_v::Zero(); }
    static Vc_INTRINSIC Vc_CONST ::Vc::SSE::short_v denorm_min()    Vc_NOEXCEPT { return ::Vc::SSE::short_v::Zero(); }
};
template<> struct numeric_limits< ::Vc::SSE::uint_v> : public numeric_limits<unsigned int>
{
    static Vc_INTRINSIC Vc_CONST ::Vc::SSE::uint_v max()           Vc_NOEXCEPT { return ::Vc::SSE::_mm_setallone_si128(); }
    static Vc_INTRINSIC Vc_CONST ::Vc::SSE::uint_v min()           Vc_NOEXCEPT { return ::Vc::SSE::uint_v::Zero(); }
    static Vc_INTRINSIC Vc_CONST ::Vc::SSE::uint_v lowest()        Vc_NOEXCEPT { return min(); }
    static Vc_INTRINSIC Vc_CONST ::Vc::SSE::uint_v epsilon()       Vc_NOEXCEPT { return ::Vc::SSE::uint_v::Zero(); }
    static Vc_INTRINSIC Vc_CONST ::Vc::SSE::uint_v round_error()   Vc_NOEXCEPT { return ::Vc::SSE::uint_v::Zero(); }
    static Vc_INTRINSIC Vc_CONST ::Vc::SSE::uint_v infinity()      Vc_NOEXCEPT { return ::Vc::SSE::uint_v::Zero(); }
    static Vc_INTRINSIC Vc_CONST ::Vc::SSE::uint_v quiet_NaN()     Vc_NOEXCEPT { return ::Vc::SSE::uint_v::Zero(); }
    static Vc_INTRINSIC Vc_CONST ::Vc::SSE::uint_v signaling_NaN() Vc_NOEXCEPT { return ::Vc::SSE::uint_v::Zero(); }
    static Vc_INTRINSIC Vc_CONST ::Vc::SSE::uint_v denorm_min()    Vc_NOEXCEPT { return ::Vc::SSE::uint_v::Zero(); }
};
template<> struct numeric_limits< ::Vc::SSE::int_v> : public numeric_limits<int>
{
    static Vc_INTRINSIC Vc_CONST ::Vc::SSE::int_v max()           Vc_NOEXCEPT { return _mm_srli_epi32(::Vc::SSE::_mm_setallone_si128(), 1); }
    static Vc_INTRINSIC Vc_CONST ::Vc::SSE::int_v min()           Vc_NOEXCEPT { return ::Vc::SSE::setmin_epi32(); }
    static Vc_INTRINSIC Vc_CONST ::Vc::SSE::int_v lowest()        Vc_NOEXCEPT { return min(); }
    static Vc_INTRINSIC Vc_CONST ::Vc::SSE::int_v epsilon()       Vc_NOEXCEPT { return ::Vc::SSE::int_v::Zero(); }
    static Vc_INTRINSIC Vc_CONST ::Vc::SSE::int_v round_error()   Vc_NOEXCEPT { return ::Vc::SSE::int_v::Zero(); }
    static Vc_INTRINSIC Vc_CONST ::Vc::SSE::int_v infinity()      Vc_NOEXCEPT { return ::Vc::SSE::int_v::Zero(); }
    static Vc_INTRINSIC Vc_CONST ::Vc::SSE::int_v quiet_NaN()     Vc_NOEXCEPT { return ::Vc::SSE::int_v::Zero(); }
    static Vc_INTRINSIC Vc_CONST ::Vc::SSE::int_v signaling_NaN() Vc_NOEXCEPT { return ::Vc::SSE::int_v::Zero(); }
    static Vc_INTRINSIC Vc_CONST ::Vc::SSE::int_v denorm_min()    Vc_NOEXCEPT { return ::Vc::SSE::int_v::Zero(); }
};
} // namespace std

#endif // VC_SSE_LIMITS_H_
