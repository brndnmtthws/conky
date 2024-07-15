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

#include "../common/x86_prefetches.h"
#include "limits.h"
#include "../common/bitscanintrinsics.h"
#include "../common/set.h"
#include "../common/gatherimplementation.h"
#include "../common/scatterimplementation.h"
#include "../common/transpose.h"
#include "macros.h"

namespace Vc_VERSIONED_NAMESPACE
{
namespace Detail
{
// compare operators {{{1
Vc_INTRINSIC SSE::double_m operator==(SSE::double_v a, SSE::double_v b) { return _mm_cmpeq_pd(a.data(), b.data()); }
Vc_INTRINSIC SSE:: float_m operator==(SSE:: float_v a, SSE:: float_v b) { return _mm_cmpeq_ps(a.data(), b.data()); }
Vc_INTRINSIC SSE::   int_m operator==(SSE::   int_v a, SSE::   int_v b) { return _mm_cmpeq_epi32(a.data(), b.data()); }
Vc_INTRINSIC SSE::  uint_m operator==(SSE::  uint_v a, SSE::  uint_v b) { return _mm_cmpeq_epi32(a.data(), b.data()); }
Vc_INTRINSIC SSE:: short_m operator==(SSE:: short_v a, SSE:: short_v b) { return _mm_cmpeq_epi16(a.data(), b.data()); }
Vc_INTRINSIC SSE::ushort_m operator==(SSE::ushort_v a, SSE::ushort_v b) { return _mm_cmpeq_epi16(a.data(), b.data()); }

Vc_INTRINSIC SSE::double_m operator!=(SSE::double_v a, SSE::double_v b) { return _mm_cmpneq_pd(a.data(), b.data()); }
Vc_INTRINSIC SSE:: float_m operator!=(SSE:: float_v a, SSE:: float_v b) { return _mm_cmpneq_ps(a.data(), b.data()); }
Vc_INTRINSIC SSE::   int_m operator!=(SSE::   int_v a, SSE::   int_v b) { return not_(_mm_cmpeq_epi32(a.data(), b.data())); }
Vc_INTRINSIC SSE::  uint_m operator!=(SSE::  uint_v a, SSE::  uint_v b) { return not_(_mm_cmpeq_epi32(a.data(), b.data())); }
Vc_INTRINSIC SSE:: short_m operator!=(SSE:: short_v a, SSE:: short_v b) { return not_(_mm_cmpeq_epi16(a.data(), b.data())); }
Vc_INTRINSIC SSE::ushort_m operator!=(SSE::ushort_v a, SSE::ushort_v b) { return not_(_mm_cmpeq_epi16(a.data(), b.data())); }

Vc_INTRINSIC SSE::double_m operator> (SSE::double_v a, SSE::double_v b) { return _mm_cmpgt_pd(a.data(), b.data()); }
Vc_INTRINSIC SSE:: float_m operator> (SSE:: float_v a, SSE:: float_v b) { return _mm_cmpgt_ps(a.data(), b.data()); }
Vc_INTRINSIC SSE::   int_m operator> (SSE::   int_v a, SSE::   int_v b) { return _mm_cmpgt_epi32(a.data(), b.data()); }
Vc_INTRINSIC SSE::  uint_m operator> (SSE::  uint_v a, SSE::  uint_v b) {
#ifndef USE_INCORRECT_UNSIGNED_COMPARE
    return SSE::cmpgt_epu32(a.data(), b.data());
#else
    return _mm_cmpgt_epi32(a.data(), b.data());
#endif
}
Vc_INTRINSIC SSE:: short_m operator> (SSE:: short_v a, SSE:: short_v b) { return _mm_cmpgt_epi16(a.data(), b.data()); }
Vc_INTRINSIC SSE::ushort_m operator> (SSE::ushort_v a, SSE::ushort_v b) {
#ifndef USE_INCORRECT_UNSIGNED_COMPARE
    return SSE::cmpgt_epu16(a.data(), b.data());
#else
    return _mm_cmpgt_epi16(a.data(), b.data());
#endif
}

Vc_INTRINSIC SSE::double_m operator< (SSE::double_v a, SSE::double_v b) { return _mm_cmplt_pd(a.data(), b.data()); }
Vc_INTRINSIC SSE:: float_m operator< (SSE:: float_v a, SSE:: float_v b) { return _mm_cmplt_ps(a.data(), b.data()); }
Vc_INTRINSIC SSE::   int_m operator< (SSE::   int_v a, SSE::   int_v b) { return _mm_cmplt_epi32(a.data(), b.data()); }
Vc_INTRINSIC SSE::  uint_m operator< (SSE::  uint_v a, SSE::  uint_v b) {
#ifndef USE_INCORRECT_UNSIGNED_COMPARE
    return SSE::cmplt_epu32(a.data(), b.data());
#else
    return _mm_cmplt_epi32(a.data(), b.data());
#endif
}
Vc_INTRINSIC SSE:: short_m operator< (SSE:: short_v a, SSE:: short_v b) { return _mm_cmplt_epi16(a.data(), b.data()); }
Vc_INTRINSIC SSE::ushort_m operator< (SSE::ushort_v a, SSE::ushort_v b) {
#ifndef USE_INCORRECT_UNSIGNED_COMPARE
    return SSE::cmplt_epu16(a.data(), b.data());
#else
    return _mm_cmplt_epi16(a.data(), b.data());
#endif
}

Vc_INTRINSIC SSE::double_m operator>=(SSE::double_v a, SSE::double_v b) { return _mm_cmpnlt_pd(a.data(), b.data()); }
Vc_INTRINSIC SSE:: float_m operator>=(SSE:: float_v a, SSE:: float_v b) { return _mm_cmpnlt_ps(a.data(), b.data()); }
Vc_INTRINSIC SSE::   int_m operator>=(SSE::   int_v a, SSE::   int_v b) { return !(a < b); }
Vc_INTRINSIC SSE::  uint_m operator>=(SSE::  uint_v a, SSE::  uint_v b) { return !(a < b); }
Vc_INTRINSIC SSE:: short_m operator>=(SSE:: short_v a, SSE:: short_v b) { return !(a < b); }
Vc_INTRINSIC SSE::ushort_m operator>=(SSE::ushort_v a, SSE::ushort_v b) { return !(a < b); }

Vc_INTRINSIC SSE::double_m operator<=(SSE::double_v a, SSE::double_v b) { return _mm_cmple_pd(a.data(), b.data()); }
Vc_INTRINSIC SSE:: float_m operator<=(SSE:: float_v a, SSE:: float_v b) { return _mm_cmple_ps(a.data(), b.data()); }
Vc_INTRINSIC SSE::   int_m operator<=(SSE::   int_v a, SSE::   int_v b) { return !(a > b); }
Vc_INTRINSIC SSE::  uint_m operator<=(SSE::  uint_v a, SSE::  uint_v b) { return !(a > b); }
Vc_INTRINSIC SSE:: short_m operator<=(SSE:: short_v a, SSE:: short_v b) { return !(a > b); }
Vc_INTRINSIC SSE::ushort_m operator<=(SSE::ushort_v a, SSE::ushort_v b) { return !(a > b); }

// bitwise operators {{{1
template <typename T>
Vc_INTRINSIC SSE::Vector<T> operator^(SSE::Vector<T> a, SSE::Vector<T> b)
{
    return xor_(a.data(), b.data());
}
template <typename T>
Vc_INTRINSIC SSE::Vector<T> operator&(SSE::Vector<T> a, SSE::Vector<T> b)
{
    return and_(a.data(), b.data());
}
template <typename T>
Vc_INTRINSIC SSE::Vector<T> operator|(SSE::Vector<T> a, SSE::Vector<T> b)
{
    return or_(a.data(), b.data());
}
// arithmetic operators {{{1
template <typename T>
Vc_INTRINSIC SSE::Vector<T> operator+(SSE::Vector<T> a, SSE::Vector<T> b)
{
    return add(a.data(), b.data(), T());
}
template <typename T>
Vc_INTRINSIC SSE::Vector<T> operator-(SSE::Vector<T> a, SSE::Vector<T> b)
{
    return sub(a.data(), b.data(), T());
}
template <typename T>
Vc_INTRINSIC SSE::Vector<T> operator*(SSE::Vector<T> a, SSE::Vector<T> b)
{
    return mul(a.data(), b.data(), T());
}
template <typename T>
Vc_INTRINSIC enable_if<std::is_floating_point<T>::value, SSE::Vector<T>> operator/(
    SSE::Vector<T> a, SSE::Vector<T> b)
{
    return div(a.data(), b.data(), T());
}
template <typename T>
Vc_INTRINSIC
    enable_if<std::is_same<int, T>::value || std::is_same<uint, T>::value, SSE::Vector<T>>
    operator/(SSE::Vector<T> a, SSE::Vector<T> b)
{
    return SSE::Vector<T>::generate([&](int i) { return a[i] / b[i]; });
}
template <typename T>
Vc_INTRINSIC enable_if<std::is_same<short, T>::value || std::is_same<ushort, T>::value,
                       SSE::Vector<T>>
operator/(SSE::Vector<T> a, SSE::Vector<T> b)
{
    using HT = SSE::VectorHelper<T>;
    __m128 lo = _mm_cvtepi32_ps(HT::expand0(a.data()));
    __m128 hi = _mm_cvtepi32_ps(HT::expand1(a.data()));
    lo = _mm_div_ps(lo, _mm_cvtepi32_ps(HT::expand0(b.data())));
    hi = _mm_div_ps(hi, _mm_cvtepi32_ps(HT::expand1(b.data())));
    return HT::concat(_mm_cvttps_epi32(lo), _mm_cvttps_epi32(hi));
}
template <typename T>
Vc_INTRINSIC enable_if<std::is_integral<T>::value, SSE::Vector<T>> operator%(
    SSE::Vector<T> a, SSE::Vector<T> b)
{
    return a - a / b * b;
}
// }}}1
}  // namespace Detail
// constants {{{1
template<typename T> Vc_INTRINSIC Vector<T, VectorAbi::Sse>::Vector(VectorSpecialInitializerZero)
    : d(HV::zero())
{
}

template<typename T> Vc_INTRINSIC Vector<T, VectorAbi::Sse>::Vector(VectorSpecialInitializerOne)
    : d(HT::one())
{
}

template <typename T>
Vc_INTRINSIC Vector<T, VectorAbi::Sse>::Vector(VectorSpecialInitializerIndexesFromZero)
    : d(Detail::load16(Detail::IndexesFromZero<EntryType, Size>(), Aligned))
{
#if defined Vc_GCC && Vc_GCC < 0x40903 && defined Vc_IMPL_AVX2
    // GCC 4.9.2 (at least) miscompiles SSE::short_v::IndexesFromZero() if used implicitly
    // from SimdArray<short, 9> compiling for AVX2 to vpmovsxwd (sign extending load from
    // a 8x 16-bit constant to 8x 32-bit register)
    if (std::is_same<T, short>::value) {
        asm("" ::"x"(d.v()));
    }
#endif
}

template <>
Vc_INTRINSIC Vector<float, VectorAbi::Sse>::Vector(VectorSpecialInitializerIndexesFromZero)
    : d(SSE::convert<int, float>(SSE::int_v::IndexesFromZero().data()))
{
}

template <>
Vc_INTRINSIC Vector<double, VectorAbi::Sse>::Vector(VectorSpecialInitializerIndexesFromZero)
    : d(SSE::convert<int, double>(SSE::int_v::IndexesFromZero().data()))
{
}

// load member functions {{{1
template <typename DstT>
template <typename SrcT, typename Flags>
Vc_INTRINSIC typename Vector<DstT, VectorAbi::Sse>::
#ifndef Vc_MSVC
template
#endif
load_concept<SrcT, Flags>::type Vector<DstT, VectorAbi::Sse>::load(const SrcT *mem, Flags flags)
{
    Common::handleLoadPrefetches(mem, flags);
    d.v() = Detail::load<VectorType, DstT>(mem, flags);
}

// zeroing {{{1
template<typename T> Vc_INTRINSIC void Vector<T, VectorAbi::Sse>::setZero()
{
    data() = HV::zero();
}

template<typename T> Vc_INTRINSIC void Vector<T, VectorAbi::Sse>::setZero(const Mask &k)
{
    data() = Detail::andnot_(k.data(), data());
}

template<typename T> Vc_INTRINSIC void Vector<T, VectorAbi::Sse>::setZeroInverted(const Mask &k)
{
    data() = Detail::and_(k.data(), data());
}

template<> Vc_INTRINSIC void SSE::double_v::setQnan()
{
    data() = SSE::_mm_setallone_pd();
}
template<> Vc_INTRINSIC void Vector<double, VectorAbi::Sse>::setQnan(const Mask &k)
{
    data() = _mm_or_pd(data(), k.dataD());
}
template<> Vc_INTRINSIC void SSE::float_v::setQnan()
{
    data() = SSE::_mm_setallone_ps();
}
template<> Vc_INTRINSIC void Vector<float, VectorAbi::Sse>::setQnan(const Mask &k)
{
    data() = _mm_or_ps(data(), k.dataF());
}

///////////////////////////////////////////////////////////////////////////////////////////
// stores {{{1
template <typename T>
template <typename U, typename Flags, typename>
Vc_INTRINSIC void Vector<T, VectorAbi::Sse>::store(U *mem, Flags flags) const
{
    Common::handleStorePrefetches(mem, flags);
    HV::template store<Flags>(mem, data());
}

template <typename T>
template <typename U, typename Flags, typename>
Vc_INTRINSIC void Vector<T, VectorAbi::Sse>::store(U *mem, Mask mask, Flags flags) const
{
    Common::handleStorePrefetches(mem, flags);
    HV::template store<Flags>(mem, data(), mask.data());
}

///////////////////////////////////////////////////////////////////////////////////////////
// operator- {{{1
template<typename T> Vc_ALWAYS_INLINE Vc_PURE Vector<T, VectorAbi::Sse> Vector<T, VectorAbi::Sse>::operator-() const
{
    return Detail::negate(d.v(), std::integral_constant<std::size_t, sizeof(T)>());
}
///////////////////////////////////////////////////////////////////////////////////////////
// integer ops {{{1
#ifdef Vc_IMPL_XOP
template <> Vc_ALWAYS_INLINE    SSE::int_v    SSE::int_v::operator<<(const    SSE::int_v shift) const { return _mm_sha_epi32(d.v(), shift.d.v()); }
template <> Vc_ALWAYS_INLINE   SSE::uint_v   SSE::uint_v::operator<<(const   SSE::uint_v shift) const { return _mm_shl_epi32(d.v(), shift.d.v()); }
template <> Vc_ALWAYS_INLINE  SSE::short_v  SSE::short_v::operator<<(const  SSE::short_v shift) const { return _mm_sha_epi16(d.v(), shift.d.v()); }
template <> Vc_ALWAYS_INLINE SSE::ushort_v SSE::ushort_v::operator<<(const SSE::ushort_v shift) const { return _mm_shl_epi16(d.v(), shift.d.v()); }
template <> Vc_ALWAYS_INLINE    SSE::int_v    SSE::int_v::operator>>(const    SSE::int_v shift) const { return operator<<(-shift); }
template <> Vc_ALWAYS_INLINE   SSE::uint_v   SSE::uint_v::operator>>(const   SSE::uint_v shift) const { return operator<<(-shift); }
template <> Vc_ALWAYS_INLINE  SSE::short_v  SSE::short_v::operator>>(const  SSE::short_v shift) const { return operator<<(-shift); }
template <> Vc_ALWAYS_INLINE SSE::ushort_v SSE::ushort_v::operator>>(const SSE::ushort_v shift) const { return operator<<(-shift); }
#elif defined Vc_IMPL_AVX2
template <> Vc_ALWAYS_INLINE SSE::Vector<   int> Vector<   int, VectorAbi::Sse>::operator<<(const SSE::Vector<   int> x) const { return _mm_sllv_epi32(d.v(), x.d.v()); }
template <> Vc_ALWAYS_INLINE SSE::Vector<  uint> Vector<  uint, VectorAbi::Sse>::operator<<(const SSE::Vector<  uint> x) const { return _mm_sllv_epi32(d.v(), x.d.v()); }
template <> Vc_ALWAYS_INLINE SSE::Vector<   int> Vector<   int, VectorAbi::Sse>::operator>>(const SSE::Vector<   int> x) const { return _mm_srav_epi32(d.v(), x.d.v()); }
template <> Vc_ALWAYS_INLINE SSE::Vector<  uint> Vector<  uint, VectorAbi::Sse>::operator>>(const SSE::Vector<  uint> x) const { return _mm_srlv_epi32(d.v(), x.d.v()); }
#endif

template<typename T> Vc_ALWAYS_INLINE Vector<T, VectorAbi::Sse> &Vector<T, VectorAbi::Sse>::operator>>=(int shift) {
    d.v() = HT::shiftRight(d.v(), shift);
    return *this;
}
template<typename T> Vc_ALWAYS_INLINE Vc_PURE Vector<T, VectorAbi::Sse> Vector<T, VectorAbi::Sse>::operator>>(int shift) const {
    return HT::shiftRight(d.v(), shift);
}
template<typename T> Vc_ALWAYS_INLINE Vector<T, VectorAbi::Sse> &Vector<T, VectorAbi::Sse>::operator<<=(int shift) {
    d.v() = HT::shiftLeft(d.v(), shift);
    return *this;
}
template<typename T> Vc_ALWAYS_INLINE Vc_PURE Vector<T, VectorAbi::Sse> Vector<T, VectorAbi::Sse>::operator<<(int shift) const {
    return HT::shiftLeft(d.v(), shift);
}

///////////////////////////////////////////////////////////////////////////////////////////
// isnegative {{{1
Vc_INTRINSIC Vc_CONST SSE::float_m isnegative(SSE::float_v x)
{
    return sse_cast<__m128>(_mm_srai_epi32(
        sse_cast<__m128i>(_mm_and_ps(SSE::_mm_setsignmask_ps(), x.data())), 31));
}
Vc_INTRINSIC Vc_CONST SSE::double_m isnegative(SSE::double_v x)
{
    return Mem::permute<X1, X1, X3, X3>(sse_cast<__m128>(_mm_srai_epi32(
        sse_cast<__m128i>(_mm_and_pd(SSE::_mm_setsignmask_pd(), x.data())), 31)));
}

// gathers {{{1
#define Vc_GATHER_IMPL(V_)                                                               \
    template <>                                                                          \
    template <class MT, class IT, int Scale>                                             \
    inline void SSE::V_::gatherImplementation(                                           \
        const Common::GatherArguments<MT, IT, Scale> &args)
#define Vc_M(i_) static_cast<value_type>(args.address[Scale * args.indexes[i_]])
Vc_GATHER_IMPL(double_v) { d.v() = _mm_setr_pd(Vc_M(0), Vc_M(1)); }
Vc_GATHER_IMPL(float_v)  { d.v() = _mm_setr_ps(Vc_M(0), Vc_M(1), Vc_M(2), Vc_M(3)); }
Vc_GATHER_IMPL(int_v)    { d.v() = _mm_setr_epi32(Vc_M(0), Vc_M(1), Vc_M(2), Vc_M(3)); }
Vc_GATHER_IMPL(uint_v)   { d.v() = _mm_setr_epi32(Vc_M(0), Vc_M(1), Vc_M(2), Vc_M(3)); }
Vc_GATHER_IMPL(short_v)
{
    d.v() =
        Vc::set(Vc_M(0), Vc_M(1), Vc_M(2), Vc_M(3), Vc_M(4), Vc_M(5), Vc_M(6), Vc_M(7));
}
Vc_GATHER_IMPL(ushort_v)
{
    d.v() =
        Vc::set(Vc_M(0), Vc_M(1), Vc_M(2), Vc_M(3), Vc_M(4), Vc_M(5), Vc_M(6), Vc_M(7));
}
#undef Vc_M
#undef Vc_GATHER_IMPL

template <typename T>
template <class MT, class IT, int Scale>
inline void Vector<T, VectorAbi::Sse>::gatherImplementation(
    const Common::GatherArguments<MT, IT, Scale> &args, MaskArgument mask)
{
    const auto *mem = args.address;
    const auto indexes = Scale * args.indexes;
    using Selector = std::integral_constant < Common::GatherScatterImplementation,
#ifdef Vc_USE_SET_GATHERS
          Traits::is_simd_vector<IT>::value ? Common::GatherScatterImplementation::SetIndexZero :
#endif
#ifdef Vc_USE_BSF_GATHERS
                                            Common::GatherScatterImplementation::BitScanLoop
#elif defined Vc_USE_POPCNT_BSF_GATHERS
              Common::GatherScatterImplementation::PopcntSwitch
#else
              Common::GatherScatterImplementation::SimpleLoop
#endif
                                                > ;
    Common::executeGather(Selector(), *this, mem, indexes, mask);
}

// scatters {{{1
template <typename T>
template <typename MT, typename IT>
inline void Vector<T, VectorAbi::Sse>::scatterImplementation(MT *mem, IT &&indexes) const
{
    Common::unrolled_loop<std::size_t, 0, Size>([&](std::size_t i) { mem[indexes[i]] = d.m(i); });
}

template <typename T>
template <typename MT, typename IT>
inline void Vector<T, VectorAbi::Sse>::scatterImplementation(MT *mem, IT &&indexes, MaskArgument mask) const
{
    using Selector = std::integral_constant < Common::GatherScatterImplementation,
#ifdef Vc_USE_SET_GATHERS
          Traits::is_simd_vector<IT>::value ? Common::GatherScatterImplementation::SetIndexZero :
#endif
#ifdef Vc_USE_BSF_GATHERS
                                            Common::GatherScatterImplementation::BitScanLoop
#elif defined Vc_USE_POPCNT_BSF_GATHERS
              Common::GatherScatterImplementation::PopcntSwitch
#else
              Common::GatherScatterImplementation::SimpleLoop
#endif
                                                > ;
    Common::executeScatter(Selector(), *this, mem, indexes, mask);
}

///////////////////////////////////////////////////////////////////////////////////////////
// horizontal ops {{{1
template<typename T> Vc_ALWAYS_INLINE Vector<T, VectorAbi::Sse> Vector<T, VectorAbi::Sse>::partialSum() const
{
    //   a    b    c    d    e    f    g    h
    // +      a    b    c    d    e    f    g    -> a ab bc  cd   de    ef     fg      gh
    // +           a    ab   bc   cd   de   ef   -> a ab abc abcd bcde  cdef   defg    efgh
    // +                     a    ab   abc  abcd -> a ab abc abcd abcde abcdef abcdefg abcdefgh
    Vector<T, VectorAbi::Sse> tmp = *this;
    if (Size >  1) tmp += tmp.shifted(-1);
    if (Size >  2) tmp += tmp.shifted(-2);
    if (Size >  4) tmp += tmp.shifted(-4);
    if (Size >  8) tmp += tmp.shifted(-8);
    if (Size > 16) tmp += tmp.shifted(-16);
    return tmp;
}
#ifndef Vc_IMPL_SSE4_1
// without SSE4.1 integer multiplication is slow and we rather multiply the scalars
template<> Vc_INTRINSIC Vc_PURE int SSE::int_v::product() const
{
    return (d.m(0) * d.m(1)) * (d.m(2) * d.m(3));
}
template<> Vc_INTRINSIC Vc_PURE unsigned int SSE::uint_v::product() const
{
    return (d.m(0) * d.m(1)) * (d.m(2) * d.m(3));
}
#endif
template<typename T> Vc_ALWAYS_INLINE Vc_PURE typename Vector<T, VectorAbi::Sse>::EntryType Vector<T, VectorAbi::Sse>::min(MaskArg m) const
{
    Vector<T, VectorAbi::Sse> tmp = std::numeric_limits<Vector<T, VectorAbi::Sse> >::max();
    tmp(m) = *this;
    return tmp.min();
}
template<typename T> Vc_ALWAYS_INLINE Vc_PURE typename Vector<T, VectorAbi::Sse>::EntryType Vector<T, VectorAbi::Sse>::max(MaskArg m) const
{
    Vector<T, VectorAbi::Sse> tmp = std::numeric_limits<Vector<T, VectorAbi::Sse> >::min();
    tmp(m) = *this;
    return tmp.max();
}
template<typename T> Vc_ALWAYS_INLINE Vc_PURE typename Vector<T, VectorAbi::Sse>::EntryType Vector<T, VectorAbi::Sse>::product(MaskArg m) const
{
    Vector<T, VectorAbi::Sse> tmp(Vc::One);
    tmp(m) = *this;
    return tmp.product();
}
template<typename T> Vc_ALWAYS_INLINE Vc_PURE typename Vector<T, VectorAbi::Sse>::EntryType Vector<T, VectorAbi::Sse>::sum(MaskArg m) const
{
    Vector<T, VectorAbi::Sse> tmp(Vc::Zero);
    tmp(m) = *this;
    return tmp.sum();
}

///////////////////////////////////////////////////////////////////////////////////////////
// exponent {{{1
namespace Detail
{
Vc_INTRINSIC Vc_CONST __m128 exponent(__m128 v)
{
    __m128i tmp = _mm_srli_epi32(_mm_castps_si128(v), 23);
    tmp = _mm_sub_epi32(tmp, _mm_set1_epi32(0x7f));
    return _mm_cvtepi32_ps(tmp);
}
Vc_INTRINSIC Vc_CONST __m128d exponent(__m128d v)
{
    __m128i tmp = _mm_srli_epi64(_mm_castpd_si128(v), 52);
    tmp = _mm_sub_epi32(tmp, _mm_set1_epi32(0x3ff));
    return _mm_cvtepi32_pd(_mm_shuffle_epi32(tmp, 0x08));
}
} // namespace Detail

Vc_INTRINSIC Vc_CONST SSE::float_v exponent(SSE::float_v x)
{
    using Detail::operator>=;
    Vc_ASSERT((x >= x.Zero()).isFull());
    return Detail::exponent(x.data());
}
Vc_INTRINSIC Vc_CONST SSE::double_v exponent(SSE::double_v x)
{
    using Detail::operator>=;
    Vc_ASSERT((x >= x.Zero()).isFull());
    return Detail::exponent(x.data());
}
// }}}1
// Random {{{1
static void _doRandomStep(SSE::uint_v &state0,
        SSE::uint_v &state1)
{
    using SSE::uint_v;
    using Detail::operator+;
    using Detail::operator*;
    state0.load(&Common::RandomState[0]);
    state1.load(&Common::RandomState[uint_v::Size]);
    (state1 * uint_v(0xdeece66du) + uint_v(11)).store(&Common::RandomState[uint_v::Size]);
    uint_v(_mm_xor_si128((state0 * uint_v(0xdeece66du) + uint_v(11)).data(),
                         _mm_srli_epi32(state1.data(), 16)))
        .store(&Common::RandomState[0]);
}

template<typename T> Vc_ALWAYS_INLINE Vector<T, VectorAbi::Sse> Vector<T, VectorAbi::Sse>::Random()
{
    SSE::uint_v state0, state1;
    _doRandomStep(state0, state1);
    return state0.data();
}

template<> Vc_ALWAYS_INLINE SSE::float_v SSE::float_v::Random()
{
    SSE::uint_v state0, state1;
    _doRandomStep(state0, state1);
    return _mm_sub_ps(_mm_or_ps(_mm_castsi128_ps(_mm_srli_epi32(state0.data(), 2)), HT::one()), HT::one());
}

template<> Vc_ALWAYS_INLINE SSE::double_v SSE::double_v::Random()
{
    typedef unsigned long long uint64 Vc_MAY_ALIAS;
    uint64 state0 = *reinterpret_cast<const uint64 *>(&Common::RandomState[8]);
    uint64 state1 = *reinterpret_cast<const uint64 *>(&Common::RandomState[10]);
    const __m128i state = _mm_load_si128(reinterpret_cast<const __m128i *>(&Common::RandomState[8]));
    *reinterpret_cast<uint64 *>(&Common::RandomState[ 8]) = (state0 * 0x5deece66dull + 11);
    *reinterpret_cast<uint64 *>(&Common::RandomState[10]) = (state1 * 0x5deece66dull + 11);
    return _mm_sub_pd(_mm_or_pd(_mm_castsi128_pd(_mm_srli_epi64(state, 12)), HT::one()), HT::one());
}
// shifted / rotated {{{1
template<typename T> Vc_INTRINSIC Vc_PURE Vector<T, VectorAbi::Sse> Vector<T, VectorAbi::Sse>::shifted(int amount) const
{
    enum {
        EntryTypeSizeof = sizeof(EntryType)
    };
    switch (amount) {
    case  0: return *this;
    case  1: return SSE::sse_cast<VectorType>(_mm_srli_si128(SSE::sse_cast<__m128i>(d.v()), 1 * EntryTypeSizeof));
    case  2: return SSE::sse_cast<VectorType>(_mm_srli_si128(SSE::sse_cast<__m128i>(d.v()), 2 * EntryTypeSizeof));
    case  3: return SSE::sse_cast<VectorType>(_mm_srli_si128(SSE::sse_cast<__m128i>(d.v()), 3 * EntryTypeSizeof));
    case  4: return SSE::sse_cast<VectorType>(_mm_srli_si128(SSE::sse_cast<__m128i>(d.v()), 4 * EntryTypeSizeof));
    case  5: return SSE::sse_cast<VectorType>(_mm_srli_si128(SSE::sse_cast<__m128i>(d.v()), 5 * EntryTypeSizeof));
    case  6: return SSE::sse_cast<VectorType>(_mm_srli_si128(SSE::sse_cast<__m128i>(d.v()), 6 * EntryTypeSizeof));
    case  7: return SSE::sse_cast<VectorType>(_mm_srli_si128(SSE::sse_cast<__m128i>(d.v()), 7 * EntryTypeSizeof));
    case  8: return SSE::sse_cast<VectorType>(_mm_srli_si128(SSE::sse_cast<__m128i>(d.v()), 8 * EntryTypeSizeof));
    case -1: return SSE::sse_cast<VectorType>(_mm_slli_si128(SSE::sse_cast<__m128i>(d.v()), 1 * EntryTypeSizeof));
    case -2: return SSE::sse_cast<VectorType>(_mm_slli_si128(SSE::sse_cast<__m128i>(d.v()), 2 * EntryTypeSizeof));
    case -3: return SSE::sse_cast<VectorType>(_mm_slli_si128(SSE::sse_cast<__m128i>(d.v()), 3 * EntryTypeSizeof));
    case -4: return SSE::sse_cast<VectorType>(_mm_slli_si128(SSE::sse_cast<__m128i>(d.v()), 4 * EntryTypeSizeof));
    case -5: return SSE::sse_cast<VectorType>(_mm_slli_si128(SSE::sse_cast<__m128i>(d.v()), 5 * EntryTypeSizeof));
    case -6: return SSE::sse_cast<VectorType>(_mm_slli_si128(SSE::sse_cast<__m128i>(d.v()), 6 * EntryTypeSizeof));
    case -7: return SSE::sse_cast<VectorType>(_mm_slli_si128(SSE::sse_cast<__m128i>(d.v()), 7 * EntryTypeSizeof));
    case -8: return SSE::sse_cast<VectorType>(_mm_slli_si128(SSE::sse_cast<__m128i>(d.v()), 8 * EntryTypeSizeof));
    }
    return Zero();
}
template<typename T> Vc_INTRINSIC Vector<T, VectorAbi::Sse> Vector<T, VectorAbi::Sse>::shifted(int amount, Vector shiftIn) const
{
    if (amount >= -int(size())) {
        constexpr int VectorWidth = int(size());
        constexpr int EntryTypeSizeof = sizeof(EntryType);
        const __m128i v0 = sse_cast<__m128i>(d.v());
        const __m128i v1 = sse_cast<__m128i>(shiftIn.d.v());
        auto &&fixup = sse_cast<VectorType, __m128i>;
        switch (amount) {
        case  0: return *this;
                 // alignr_epi8: [arg1 arg0] << n
        case -1: return fixup(SSE::alignr_epi8<(VectorWidth - 1) * EntryTypeSizeof>(v0, v1));
        case -2: return fixup(SSE::alignr_epi8<(VectorWidth - 2) * EntryTypeSizeof>(v0, v1));
        case -3: return fixup(SSE::alignr_epi8<(VectorWidth - 3) * EntryTypeSizeof>(v0, v1));
        case -4: return fixup(SSE::alignr_epi8<(VectorWidth - 4) * EntryTypeSizeof>(v0, v1));
        case -5: return fixup(SSE::alignr_epi8<(VectorWidth - 5) * EntryTypeSizeof>(v0, v1));
        case -6: return fixup(SSE::alignr_epi8<(VectorWidth - 6) * EntryTypeSizeof>(v0, v1));
        case -7: return fixup(SSE::alignr_epi8<(VectorWidth - 7) * EntryTypeSizeof>(v0, v1));
        case -8: return fixup(SSE::alignr_epi8<(VectorWidth - 8) * EntryTypeSizeof>(v0, v1));
        case -9: return fixup(SSE::alignr_epi8<(VectorWidth - 9) * EntryTypeSizeof>(v0, v1));
        case-10: return fixup(SSE::alignr_epi8<(VectorWidth -10) * EntryTypeSizeof>(v0, v1));
        case-11: return fixup(SSE::alignr_epi8<(VectorWidth -11) * EntryTypeSizeof>(v0, v1));
        case-12: return fixup(SSE::alignr_epi8<(VectorWidth -12) * EntryTypeSizeof>(v0, v1));
        case-13: return fixup(SSE::alignr_epi8<(VectorWidth -13) * EntryTypeSizeof>(v0, v1));
        case-14: return fixup(SSE::alignr_epi8<(VectorWidth -14) * EntryTypeSizeof>(v0, v1));
        case-15: return fixup(SSE::alignr_epi8<(VectorWidth -15) * EntryTypeSizeof>(v0, v1));
        case  1: return fixup(SSE::alignr_epi8< 1 * EntryTypeSizeof>(v1, v0));
        case  2: return fixup(SSE::alignr_epi8< 2 * EntryTypeSizeof>(v1, v0));
        case  3: return fixup(SSE::alignr_epi8< 3 * EntryTypeSizeof>(v1, v0));
        case  4: return fixup(SSE::alignr_epi8< 4 * EntryTypeSizeof>(v1, v0));
        case  5: return fixup(SSE::alignr_epi8< 5 * EntryTypeSizeof>(v1, v0));
        case  6: return fixup(SSE::alignr_epi8< 6 * EntryTypeSizeof>(v1, v0));
        case  7: return fixup(SSE::alignr_epi8< 7 * EntryTypeSizeof>(v1, v0));
        case  8: return fixup(SSE::alignr_epi8< 8 * EntryTypeSizeof>(v1, v0));
        case  9: return fixup(SSE::alignr_epi8< 9 * EntryTypeSizeof>(v1, v0));
        case 10: return fixup(SSE::alignr_epi8<10 * EntryTypeSizeof>(v1, v0));
        case 11: return fixup(SSE::alignr_epi8<11 * EntryTypeSizeof>(v1, v0));
        case 12: return fixup(SSE::alignr_epi8<12 * EntryTypeSizeof>(v1, v0));
        case 13: return fixup(SSE::alignr_epi8<13 * EntryTypeSizeof>(v1, v0));
        case 14: return fixup(SSE::alignr_epi8<14 * EntryTypeSizeof>(v1, v0));
        case 15: return fixup(SSE::alignr_epi8<15 * EntryTypeSizeof>(v1, v0));
        }
    }
    return shiftIn.shifted(int(size()) + amount);
}
template<typename T> Vc_INTRINSIC Vc_PURE Vector<T, VectorAbi::Sse> Vector<T, VectorAbi::Sse>::rotated(int amount) const
{
    enum {
        EntryTypeSizeof = sizeof(EntryType)
    };
    const __m128i v = SSE::sse_cast<__m128i>(d.v());
    switch (static_cast<unsigned int>(amount) % Size) {
    case  0: return *this;
    case  1: return SSE::sse_cast<VectorType>(SSE::alignr_epi8<1 * EntryTypeSizeof>(v, v));
    case  2: return SSE::sse_cast<VectorType>(SSE::alignr_epi8<2 * EntryTypeSizeof>(v, v));
    case  3: return SSE::sse_cast<VectorType>(SSE::alignr_epi8<3 * EntryTypeSizeof>(v, v));
             // warning "Immediate parameter to intrinsic call too large" disabled in VcMacros.cmake.
             // ICC fails to see that the modulo operation (Size == sizeof(VectorType) / sizeof(EntryType))
             // disables the following four calls unless sizeof(EntryType) == 2.
    case  4: return SSE::sse_cast<VectorType>(SSE::alignr_epi8<4 * EntryTypeSizeof>(v, v));
    case  5: return SSE::sse_cast<VectorType>(SSE::alignr_epi8<5 * EntryTypeSizeof>(v, v));
    case  6: return SSE::sse_cast<VectorType>(SSE::alignr_epi8<6 * EntryTypeSizeof>(v, v));
    case  7: return SSE::sse_cast<VectorType>(SSE::alignr_epi8<7 * EntryTypeSizeof>(v, v));
    }
    return Zero();
}
// sorted {{{1
namespace Detail
{
inline Vc_CONST SSE::double_v sorted(SSE::double_v x_)
{
    const __m128d x = x_.data();
    const __m128d y = _mm_shuffle_pd(x, x, _MM_SHUFFLE2(0, 1));
    return _mm_unpacklo_pd(_mm_min_sd(x, y), _mm_max_sd(x, y));
}
}  // namespace Detail
template <typename T>
Vc_ALWAYS_INLINE Vc_PURE Vector<T, VectorAbi::Sse> Vector<T, VectorAbi::Sse>::sorted()
    const
{
    return Detail::sorted(*this);
}
// interleaveLow/-High {{{1
template <> Vc_INTRINSIC SSE::double_v SSE::double_v::interleaveLow (SSE::double_v x) const { return _mm_unpacklo_pd(data(), x.data()); }
template <> Vc_INTRINSIC SSE::double_v SSE::double_v::interleaveHigh(SSE::double_v x) const { return _mm_unpackhi_pd(data(), x.data()); }
template <> Vc_INTRINSIC  SSE::float_v  SSE::float_v::interleaveLow ( SSE::float_v x) const { return _mm_unpacklo_ps(data(), x.data()); }
template <> Vc_INTRINSIC  SSE::float_v  SSE::float_v::interleaveHigh( SSE::float_v x) const { return _mm_unpackhi_ps(data(), x.data()); }
template <> Vc_INTRINSIC    SSE::int_v    SSE::int_v::interleaveLow (   SSE::int_v x) const { return _mm_unpacklo_epi32(data(), x.data()); }
template <> Vc_INTRINSIC    SSE::int_v    SSE::int_v::interleaveHigh(   SSE::int_v x) const { return _mm_unpackhi_epi32(data(), x.data()); }
template <> Vc_INTRINSIC   SSE::uint_v   SSE::uint_v::interleaveLow (  SSE::uint_v x) const { return _mm_unpacklo_epi32(data(), x.data()); }
template <> Vc_INTRINSIC   SSE::uint_v   SSE::uint_v::interleaveHigh(  SSE::uint_v x) const { return _mm_unpackhi_epi32(data(), x.data()); }
template <> Vc_INTRINSIC  SSE::short_v  SSE::short_v::interleaveLow ( SSE::short_v x) const { return _mm_unpacklo_epi16(data(), x.data()); }
template <> Vc_INTRINSIC  SSE::short_v  SSE::short_v::interleaveHigh( SSE::short_v x) const { return _mm_unpackhi_epi16(data(), x.data()); }
template <> Vc_INTRINSIC SSE::ushort_v SSE::ushort_v::interleaveLow (SSE::ushort_v x) const { return _mm_unpacklo_epi16(data(), x.data()); }
template <> Vc_INTRINSIC SSE::ushort_v SSE::ushort_v::interleaveHigh(SSE::ushort_v x) const { return _mm_unpackhi_epi16(data(), x.data()); }
// }}}1
// generate {{{1
template <> template <typename G> Vc_INTRINSIC SSE::double_v SSE::double_v::generate(G gen)
{
    const auto tmp0 = gen(0);
    const auto tmp1 = gen(1);
    return _mm_setr_pd(tmp0, tmp1);
}
template <> template <typename G> Vc_INTRINSIC SSE::float_v SSE::float_v::generate(G gen)
{
    const auto tmp0 = gen(0);
    const auto tmp1 = gen(1);
    const auto tmp2 = gen(2);
    const auto tmp3 = gen(3);
    return _mm_setr_ps(tmp0, tmp1, tmp2, tmp3);
}
template <> template <typename G> Vc_INTRINSIC SSE::int_v SSE::int_v::generate(G gen)
{
    const auto tmp0 = gen(0);
    const auto tmp1 = gen(1);
    const auto tmp2 = gen(2);
    const auto tmp3 = gen(3);
    return _mm_setr_epi32(tmp0, tmp1, tmp2, tmp3);
}
template <> template <typename G> Vc_INTRINSIC SSE::uint_v SSE::uint_v::generate(G gen)
{
    const auto tmp0 = gen(0);
    const auto tmp1 = gen(1);
    const auto tmp2 = gen(2);
    const auto tmp3 = gen(3);
    return _mm_setr_epi32(tmp0, tmp1, tmp2, tmp3);
}
template <> template <typename G> Vc_INTRINSIC SSE::short_v SSE::short_v::generate(G gen)
{
    const auto tmp0 = gen(0);
    const auto tmp1 = gen(1);
    const auto tmp2 = gen(2);
    const auto tmp3 = gen(3);
    const auto tmp4 = gen(4);
    const auto tmp5 = gen(5);
    const auto tmp6 = gen(6);
    const auto tmp7 = gen(7);
    return _mm_setr_epi16(tmp0, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7);
}
template <> template <typename G> Vc_INTRINSIC SSE::ushort_v SSE::ushort_v::generate(G gen)
{
    const auto tmp0 = gen(0);
    const auto tmp1 = gen(1);
    const auto tmp2 = gen(2);
    const auto tmp3 = gen(3);
    const auto tmp4 = gen(4);
    const auto tmp5 = gen(5);
    const auto tmp6 = gen(6);
    const auto tmp7 = gen(7);
    return _mm_setr_epi16(tmp0, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7);
}
// }}}1
// reversed {{{1
template <> Vc_INTRINSIC Vc_PURE SSE::double_v SSE::double_v::reversed() const
{
    return Mem::permute<X1, X0>(d.v());
}
template <> Vc_INTRINSIC Vc_PURE SSE::float_v SSE::float_v::reversed() const
{
    return Mem::permute<X3, X2, X1, X0>(d.v());
}
template <> Vc_INTRINSIC Vc_PURE SSE::int_v SSE::int_v::reversed() const
{
    return Mem::permute<X3, X2, X1, X0>(d.v());
}
template <> Vc_INTRINSIC Vc_PURE SSE::uint_v SSE::uint_v::reversed() const
{
    return Mem::permute<X3, X2, X1, X0>(d.v());
}
template <> Vc_INTRINSIC Vc_PURE SSE::short_v SSE::short_v::reversed() const
{
    return sse_cast<__m128i>(
        Mem::shuffle<X1, Y0>(sse_cast<__m128d>(Mem::permuteHi<X7, X6, X5, X4>(d.v())),
                             sse_cast<__m128d>(Mem::permuteLo<X3, X2, X1, X0>(d.v()))));
}
template <> Vc_INTRINSIC Vc_PURE SSE::ushort_v SSE::ushort_v::reversed() const
{
    return sse_cast<__m128i>(
        Mem::shuffle<X1, Y0>(sse_cast<__m128d>(Mem::permuteHi<X7, X6, X5, X4>(d.v())),
                             sse_cast<__m128d>(Mem::permuteLo<X3, X2, X1, X0>(d.v()))));
}
// }}}1
// permutation via operator[] {{{1
template <>
Vc_INTRINSIC SSE::float_v SSE::float_v::operator[](const SSE::int_v &
#ifdef Vc_IMPL_AVX
                                             perm
#endif
                                         ) const
{
    /*
    const int_m cross128 = concat(_mm_cmpgt_epi32(lo128(perm.data()), _mm_set1_epi32(3)),
                                  _mm_cmplt_epi32(hi128(perm.data()), _mm_set1_epi32(4)));
    if (cross128.isNotEmpty()) {
    SSE::float_v x = _mm256_permutevar_ps(d.v(), perm.data());
        x(cross128) = _mm256_permutevar_ps(Mem::permute128<X1, X0>(d.v()), perm.data());
        return x;
    } else {
    */
#ifdef Vc_IMPL_AVX
    return _mm_permutevar_ps(d.v(), perm.data());
#else
    return *this;//TODO
#endif
}
// broadcast from constexpr index {{{1
template <> template <int Index> Vc_INTRINSIC SSE::float_v SSE::float_v::broadcast() const
{
    constexpr VecPos Inner = static_cast<VecPos>(Index & 0x3);
    return Mem::permute<Inner, Inner, Inner, Inner>(d.v());
}
template <> template <int Index> Vc_INTRINSIC SSE::double_v SSE::double_v::broadcast() const
{
    constexpr VecPos Inner = static_cast<VecPos>(Index & 0x1);
    return Mem::permute<Inner, Inner>(d.v());
}
// }}}1

namespace Common
{
// transpose_impl {{{1
Vc_ALWAYS_INLINE void transpose_impl(
    TransposeTag<4, 4>, SSE::float_v *Vc_RESTRICT r[],
    const TransposeProxy<SSE::float_v, SSE::float_v, SSE::float_v, SSE::float_v> &proxy)
{
    const auto in0 = std::get<0>(proxy.in).data();
    const auto in1 = std::get<1>(proxy.in).data();
    const auto in2 = std::get<2>(proxy.in).data();
    const auto in3 = std::get<3>(proxy.in).data();
    const auto tmp0 = _mm_unpacklo_ps(in0, in2);
    const auto tmp1 = _mm_unpacklo_ps(in1, in3);
    const auto tmp2 = _mm_unpackhi_ps(in0, in2);
    const auto tmp3 = _mm_unpackhi_ps(in1, in3);
    *r[0] = _mm_unpacklo_ps(tmp0, tmp1);
    *r[1] = _mm_unpackhi_ps(tmp0, tmp1);
    *r[2] = _mm_unpacklo_ps(tmp2, tmp3);
    *r[3] = _mm_unpackhi_ps(tmp2, tmp3);
}
// }}}1
}  // namespace Common
}

// vim: foldmethod=marker
