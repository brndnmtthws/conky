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

#include <cmath>
#include "../common/data.h"
#include "../common/where.h"
#include "../common/transpose.h"
#include "operators.h"
#include "macros.h"
namespace Vc_VERSIONED_NAMESPACE
{

// special value constructors{{{1
template <typename T>
Vc_INTRINSIC Vector<T, VectorAbi::Scalar>::Vector(VectorSpecialInitializerZero)
    : m_data(0)
{
}
template <typename T>
Vc_INTRINSIC Vector<T, VectorAbi::Scalar>::Vector(VectorSpecialInitializerOne)
    : m_data(1)
{
}
template <typename T>
Vc_INTRINSIC Vector<T, VectorAbi::Scalar>::Vector(VectorSpecialInitializerIndexesFromZero)
    : m_data(0)
{
}

// load member functions{{{1
template <typename T>
template <typename U, typename Flags>
Vc_INTRINSIC typename Vector<T, VectorAbi::Scalar>::
#ifndef Vc_MSVC
template
#endif
load_concept<U, Flags>::type Vector<T, VectorAbi::Scalar>::load(const U *mem, Flags)
{
    m_data = mem[0];
}

// store member functions{{{1
template <typename T>
template <typename U, typename Flags, typename>
Vc_INTRINSIC void Vector<T, VectorAbi::Scalar>::store(U *mem, Flags) const
{
    mem[0] = m_data;
}
template <typename T>
template <typename U, typename Flags, typename>
Vc_INTRINSIC void Vector<T, VectorAbi::Scalar>::store(U *mem, Mask mask, Flags) const
{
    if (mask.data())
        mem[0] = m_data;
}

// gather {{{1
template <typename T>
template <class MT, class IT, int Scale>
Vc_ALWAYS_INLINE void Vector<T, VectorAbi::Scalar>::gatherImplementation(
    const Common::GatherArguments<MT, IT, Scale> &args)
{
    m_data = args.address[Scale * args.indexes[0]];
}

template <typename T>
template <class MT, class IT, int Scale>
Vc_ALWAYS_INLINE void Vector<T, VectorAbi::Scalar>::gatherImplementation(
    const Common::GatherArguments<MT, IT, Scale> &args, MaskArgument mask)
{
    if (mask.data()) {
        m_data = args.address[Scale * args.indexes[0]];
    }
}

// scatter {{{1
template <typename T>
template <typename MT, typename IT>
Vc_ALWAYS_INLINE void Vector<T, VectorAbi::Scalar>::scatterImplementation(MT *mem,
                                                                          IT &&indexes)
    const
{
    mem[indexes[0]] = m_data;
}

template <typename T>
template <typename MT, typename IT>
Vc_ALWAYS_INLINE void Vector<T, VectorAbi::Scalar>::scatterImplementation(
    MT *mem, IT &&indexes, MaskArgument mask) const
{
    if (mask.data()) {
        mem[indexes[0]] = m_data;
    }
}

// exponent {{{1
Vc_INTRINSIC Vc_CONST Scalar::float_v exponent(Scalar::float_v x)
{
    Vc_ASSERT(x.data() >= 0.f);
    union { float f; int i; } value;
    value.f = x.data();
    return Scalar::float_v(static_cast<float>((value.i >> 23) - 0x7f));
}
Vc_INTRINSIC Vc_CONST Scalar::double_v Vc_VDECL exponent(Scalar::double_v x)
{
    Vc_ASSERT(x.data() >= 0.);
    union { double f; long long i; } value;
    value.f = x.data();
    return Scalar::double_v(static_cast<double>((value.i >> 52) - 0x3ff));
}

// Random {{{1
static Vc_ALWAYS_INLINE void _doRandomStep(Scalar::uint_v &state0, Scalar::uint_v &state1)
{
    using Scalar::uint_v;
    state0.load(&Common::RandomState[0]);
    state1.load(&Common::RandomState[uint_v::Size]);
    Detail::operator+(Detail::operator*(state1, uint_v(0xdeece66du)),
                      uint_v(11))
        .store(&Common::RandomState[uint_v::Size]);
    uint_v(Detail::operator+(Detail::operator*(state0, uint_v(0xdeece66du)), uint_v(11))
               .data() ^
           (state1.data() >> 16))
        .store(&Common::RandomState[0]);
}

template<typename T> Vc_INTRINSIC Vector<T, VectorAbi::Scalar> Vector<T, VectorAbi::Scalar>::Random()
{
    Scalar::uint_v state0, state1;
    _doRandomStep(state0, state1);
    return Vector<T, VectorAbi::Scalar>(static_cast<EntryType>(state0.data()));
}
template<> Vc_INTRINSIC Scalar::float_v Scalar::float_v::Random()
{
    Scalar::uint_v state0, state1;
    _doRandomStep(state0, state1);
    union { unsigned int i; float f; } x;
    x.i = (state0.data() & 0x0fffffffu) | 0x3f800000u;
    return Scalar::float_v(x.f - 1.f);
}
template<> Vc_INTRINSIC Scalar::double_v Scalar::double_v::Random()
{
    typedef unsigned long long uint64 Vc_MAY_ALIAS;
    uint64 state0 = *reinterpret_cast<const uint64 *>(&Common::RandomState[8]);
    state0 = (state0 * 0x5deece66dull + 11) & 0x000fffffffffffffull;
    *reinterpret_cast<uint64 *>(&Common::RandomState[8]) = state0;
    union { unsigned long long i; double f; } x;
    x.i = state0 | 0x3ff0000000000000ull;
    return Scalar::double_v(x.f - 1.);
}
// isnegative {{{1
Vc_INTRINSIC Vc_CONST Scalar::float_m isnegative(Scalar::float_v x)
{
    static_assert(sizeof(float) == sizeof(unsigned int),
                  "This code assumes float and unsigned int have the same number of "
                  "Bytes. Please file a bug report if this is a problem.");
    union { float f; unsigned int i; } u;
    u.f = x.data();
    return Scalar::float_m(0u != (u.i & 0x80000000u));
}
Vc_INTRINSIC Vc_CONST Scalar::double_m Vc_VDECL isnegative(Scalar::double_v x)
{
    static_assert(sizeof(double) == sizeof(unsigned long long),
                  "This code assumes double and unsigned long long have the same number "
                  "of Bytes. Please file a bug report if this is a problem.");
    union { double d; unsigned long long l; } u;
    u.d = x.data();
    return Scalar::double_m(0ull != (u.l & 0x8000000000000000ull));
}

// setQnan {{{1
template<typename T> Vc_INTRINSIC void Vector<T, VectorAbi::Scalar>::setQnan()
{
    union { float f; unsigned int i; } u;
    u.i = 0xffffffffu;
    m_data = u.f;
}
template<> Vc_INTRINSIC void Scalar::double_v::setQnan()
{
    union { double d; unsigned long long l; } u;
    u.l = 0xffffffffffffffffull;
    m_data = u.d;
}
template<typename T> Vc_INTRINSIC void Vector<T, VectorAbi::Scalar>::setQnan(Mask m)
{
    if (m.data()) {
        setQnan();
    }
}
template<> Vc_INTRINSIC void Scalar::double_v::setQnan(Scalar::double_v::Mask m)
{
    if (m.data()) {
        setQnan();
    }
}
// }}}1

namespace Common
{
// transpose_impl {{{1
Vc_ALWAYS_INLINE void transpose_impl(TransposeTag<1, 1>, Scalar::float_v *Vc_RESTRICT r[],
                                     const TransposeProxy<Scalar::float_v> &proxy)
{
    *r[0] = std::get<0>(proxy.in).data();
}
// }}}1
}  // namespace Common
}
// vim: foldmethod=marker
