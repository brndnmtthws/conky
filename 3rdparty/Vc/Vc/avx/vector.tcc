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

#include "../common/x86_prefetches.h"
#include "../common/gatherimplementation.h"
#include "../common/scatterimplementation.h"
#include "limits.h"
#include "const.h"
#include "../common/set.h"
#include "macros.h"

namespace Vc_VERSIONED_NAMESPACE
{
namespace Detail
{
// compare operators {{{1
Vc_INTRINSIC AVX2::double_m operator==(AVX2::double_v a, AVX2::double_v b) { return AVX::cmpeq_pd(a.data(), b.data()); }
Vc_INTRINSIC AVX2:: float_m operator==(AVX2:: float_v a, AVX2:: float_v b) { return AVX::cmpeq_ps(a.data(), b.data()); }
Vc_INTRINSIC AVX2::double_m operator!=(AVX2::double_v a, AVX2::double_v b) { return AVX::cmpneq_pd(a.data(), b.data()); }
Vc_INTRINSIC AVX2:: float_m operator!=(AVX2:: float_v a, AVX2:: float_v b) { return AVX::cmpneq_ps(a.data(), b.data()); }
Vc_INTRINSIC AVX2::double_m operator>=(AVX2::double_v a, AVX2::double_v b) { return AVX::cmpnlt_pd(a.data(), b.data()); }
Vc_INTRINSIC AVX2:: float_m operator>=(AVX2:: float_v a, AVX2:: float_v b) { return AVX::cmpnlt_ps(a.data(), b.data()); }
Vc_INTRINSIC AVX2::double_m operator<=(AVX2::double_v a, AVX2::double_v b) { return AVX::cmple_pd(a.data(), b.data()); }
Vc_INTRINSIC AVX2:: float_m operator<=(AVX2:: float_v a, AVX2:: float_v b) { return AVX::cmple_ps(a.data(), b.data()); }
Vc_INTRINSIC AVX2::double_m operator> (AVX2::double_v a, AVX2::double_v b) { return AVX::cmpgt_pd(a.data(), b.data()); }
Vc_INTRINSIC AVX2:: float_m operator> (AVX2:: float_v a, AVX2:: float_v b) { return AVX::cmpgt_ps(a.data(), b.data()); }
Vc_INTRINSIC AVX2::double_m operator< (AVX2::double_v a, AVX2::double_v b) { return AVX::cmplt_pd(a.data(), b.data()); }
Vc_INTRINSIC AVX2:: float_m operator< (AVX2:: float_v a, AVX2:: float_v b) { return AVX::cmplt_ps(a.data(), b.data()); }

#ifdef Vc_IMPL_AVX2
Vc_INTRINSIC AVX2::   int_m operator==(AVX2::   int_v a, AVX2::   int_v b) { return AVX::cmpeq_epi32(a.data(), b.data()); }
Vc_INTRINSIC AVX2::  uint_m operator==(AVX2::  uint_v a, AVX2::  uint_v b) { return AVX::cmpeq_epi32(a.data(), b.data()); }
Vc_INTRINSIC AVX2:: short_m operator==(AVX2:: short_v a, AVX2:: short_v b) { return AVX::cmpeq_epi16(a.data(), b.data()); }
Vc_INTRINSIC AVX2::ushort_m operator==(AVX2::ushort_v a, AVX2::ushort_v b) { return AVX::cmpeq_epi16(a.data(), b.data()); }
Vc_INTRINSIC AVX2::   int_m operator!=(AVX2::   int_v a, AVX2::   int_v b) { return not_(AVX::cmpeq_epi32(a.data(), b.data())); }
Vc_INTRINSIC AVX2::  uint_m operator!=(AVX2::  uint_v a, AVX2::  uint_v b) { return not_(AVX::cmpeq_epi32(a.data(), b.data())); }
Vc_INTRINSIC AVX2:: short_m operator!=(AVX2:: short_v a, AVX2:: short_v b) { return not_(AVX::cmpeq_epi16(a.data(), b.data())); }
Vc_INTRINSIC AVX2::ushort_m operator!=(AVX2::ushort_v a, AVX2::ushort_v b) { return not_(AVX::cmpeq_epi16(a.data(), b.data())); }
Vc_INTRINSIC AVX2::   int_m operator>=(AVX2::   int_v a, AVX2::   int_v b) { return not_(AVX::cmplt_epi32(a.data(), b.data())); }
Vc_INTRINSIC AVX2::  uint_m operator>=(AVX2::  uint_v a, AVX2::  uint_v b) { return not_(AVX::cmplt_epu32(a.data(), b.data())); }
Vc_INTRINSIC AVX2:: short_m operator>=(AVX2:: short_v a, AVX2:: short_v b) { return not_(AVX::cmplt_epi16(a.data(), b.data())); }
Vc_INTRINSIC AVX2::ushort_m operator>=(AVX2::ushort_v a, AVX2::ushort_v b) { return not_(AVX::cmplt_epu16(a.data(), b.data())); }
Vc_INTRINSIC AVX2::   int_m operator<=(AVX2::   int_v a, AVX2::   int_v b) { return not_(AVX::cmpgt_epi32(a.data(), b.data())); }
Vc_INTRINSIC AVX2::  uint_m operator<=(AVX2::  uint_v a, AVX2::  uint_v b) { return not_(AVX::cmpgt_epu32(a.data(), b.data())); }
Vc_INTRINSIC AVX2:: short_m operator<=(AVX2:: short_v a, AVX2:: short_v b) { return not_(AVX::cmpgt_epi16(a.data(), b.data())); }
Vc_INTRINSIC AVX2::ushort_m operator<=(AVX2::ushort_v a, AVX2::ushort_v b) { return not_(AVX::cmpgt_epu16(a.data(), b.data())); }
Vc_INTRINSIC AVX2::   int_m operator> (AVX2::   int_v a, AVX2::   int_v b) { return AVX::cmpgt_epi32(a.data(), b.data()); }
Vc_INTRINSIC AVX2::  uint_m operator> (AVX2::  uint_v a, AVX2::  uint_v b) { return AVX::cmpgt_epu32(a.data(), b.data()); }
Vc_INTRINSIC AVX2:: short_m operator> (AVX2:: short_v a, AVX2:: short_v b) { return AVX::cmpgt_epi16(a.data(), b.data()); }
Vc_INTRINSIC AVX2::ushort_m operator> (AVX2::ushort_v a, AVX2::ushort_v b) { return AVX::cmpgt_epu16(a.data(), b.data()); }
Vc_INTRINSIC AVX2::   int_m operator< (AVX2::   int_v a, AVX2::   int_v b) { return AVX::cmplt_epi32(a.data(), b.data()); }
Vc_INTRINSIC AVX2::  uint_m operator< (AVX2::  uint_v a, AVX2::  uint_v b) { return AVX::cmplt_epu32(a.data(), b.data()); }
Vc_INTRINSIC AVX2:: short_m operator< (AVX2:: short_v a, AVX2:: short_v b) { return AVX::cmplt_epi16(a.data(), b.data()); }
Vc_INTRINSIC AVX2::ushort_m operator< (AVX2::ushort_v a, AVX2::ushort_v b) { return AVX::cmplt_epu16(a.data(), b.data()); }
#endif  // Vc_IMPL_AVX2

// bitwise operators {{{1
template <typename T>
Vc_INTRINSIC AVX2::Vector<T> operator^(AVX2::Vector<T> a, AVX2::Vector<T> b)
{
    return xor_(a.data(), b.data());
}
template <typename T>
Vc_INTRINSIC AVX2::Vector<T> operator&(AVX2::Vector<T> a, AVX2::Vector<T> b)
{
    return and_(a.data(), b.data());
}
template <typename T>
Vc_INTRINSIC AVX2::Vector<T> operator|(AVX2::Vector<T> a, AVX2::Vector<T> b)
{
    return or_(a.data(), b.data());
}
// }}}1
// arithmetic operators {{{1
template <typename T>
Vc_INTRINSIC AVX2::Vector<T> operator+(AVX2::Vector<T> a, AVX2::Vector<T> b)
{
    return add(a.data(), b.data(), T());
}
template <typename T>
Vc_INTRINSIC AVX2::Vector<T> operator-(AVX2::Vector<T> a, AVX2::Vector<T> b)
{
    return sub(a.data(), b.data(), T());
}
template <typename T>
Vc_INTRINSIC AVX2::Vector<T> operator*(AVX2::Vector<T> a, AVX2::Vector<T> b)
{
    return mul(a.data(), b.data(), T());
}
template <typename T>
Vc_INTRINSIC AVX2::Vector<T> operator/(AVX2::Vector<T> a, AVX2::Vector<T> b)
{
    return div(a.data(), b.data(), T());
}
Vc_INTRINSIC AVX2::Vector<ushort> operator/(AVX2::Vector<ushort> a,
                                            AVX2::Vector<ushort> b)
{
    using namespace AVX;
    const __m256 lo = _mm256_div_ps(convert<ushort, float>(lo128(a.data())),
                                    convert<ushort, float>(lo128(b.data())));
    const __m256 hi = _mm256_div_ps(convert<ushort, float>(hi128(a.data())),
                                    convert<ushort, float>(hi128(b.data())));
    const float_v threshold = 32767.f;
    using Detail::operator>;
    const __m128i loShort = (Vc_IS_UNLIKELY((float_v(lo) > threshold).isNotEmpty()))
                                ? convert<float, ushort>(lo)
                                : convert<float, short>(lo);
    const __m128i hiShort = (Vc_IS_UNLIKELY((float_v(hi) > threshold).isNotEmpty()))
                                ? convert<float, ushort>(hi)
                                : convert<float, short>(hi);
    return concat(loShort, hiShort);
}
template <typename T>
Vc_INTRINSIC enable_if<std::is_integral<T>::value, AVX2::Vector<T>> operator%(
    AVX2::Vector<T> a, AVX2::Vector<T> b)
{
    return a - a / b * b;
}
// }}}1
}  // namespace Detail
///////////////////////////////////////////////////////////////////////////////////////////
// generate {{{1
template <> template <typename G> Vc_INTRINSIC AVX2::double_v AVX2::double_v::generate(G gen)
{
    const auto tmp0 = gen(0);
    const auto tmp1 = gen(1);
    const auto tmp2 = gen(2);
    const auto tmp3 = gen(3);
    return _mm256_setr_pd(tmp0, tmp1, tmp2, tmp3);
}
template <> template <typename G> Vc_INTRINSIC AVX2::float_v AVX2::float_v::generate(G gen)
{
    const auto tmp0 = gen(0);
    const auto tmp1 = gen(1);
    const auto tmp2 = gen(2);
    const auto tmp3 = gen(3);
    const auto tmp4 = gen(4);
    const auto tmp5 = gen(5);
    const auto tmp6 = gen(6);
    const auto tmp7 = gen(7);
    return _mm256_setr_ps(tmp0, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7);
}
#ifdef Vc_IMPL_AVX2
template <> template <typename G> Vc_INTRINSIC AVX2::int_v AVX2::int_v::generate(G gen)
{
    const auto tmp0 = gen(0);
    const auto tmp1 = gen(1);
    const auto tmp2 = gen(2);
    const auto tmp3 = gen(3);
    const auto tmp4 = gen(4);
    const auto tmp5 = gen(5);
    const auto tmp6 = gen(6);
    const auto tmp7 = gen(7);
    return _mm256_setr_epi32(tmp0, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7);
}
template <> template <typename G> Vc_INTRINSIC AVX2::uint_v AVX2::uint_v::generate(G gen)
{
    const auto tmp0 = gen(0);
    const auto tmp1 = gen(1);
    const auto tmp2 = gen(2);
    const auto tmp3 = gen(3);
    const auto tmp4 = gen(4);
    const auto tmp5 = gen(5);
    const auto tmp6 = gen(6);
    const auto tmp7 = gen(7);
    return _mm256_setr_epi32(tmp0, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7);
}
template <> template <typename G> Vc_INTRINSIC AVX2::short_v AVX2::short_v::generate(G gen)
{
    const auto tmp0 = gen(0);
    const auto tmp1 = gen(1);
    const auto tmp2 = gen(2);
    const auto tmp3 = gen(3);
    const auto tmp4 = gen(4);
    const auto tmp5 = gen(5);
    const auto tmp6 = gen(6);
    const auto tmp7 = gen(7);
    const auto tmp8 = gen(8);
    const auto tmp9 = gen(9);
    const auto tmp10 = gen(10);
    const auto tmp11 = gen(11);
    const auto tmp12 = gen(12);
    const auto tmp13 = gen(13);
    const auto tmp14 = gen(14);
    const auto tmp15 = gen(15);
    return _mm256_setr_epi16(tmp0, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7, tmp8, tmp9, tmp10, tmp11, tmp12, tmp13, tmp14, tmp15);
}
template <> template <typename G> Vc_INTRINSIC AVX2::ushort_v AVX2::ushort_v::generate(G gen)
{
    const auto tmp0 = gen(0);
    const auto tmp1 = gen(1);
    const auto tmp2 = gen(2);
    const auto tmp3 = gen(3);
    const auto tmp4 = gen(4);
    const auto tmp5 = gen(5);
    const auto tmp6 = gen(6);
    const auto tmp7 = gen(7);
    const auto tmp8 = gen(8);
    const auto tmp9 = gen(9);
    const auto tmp10 = gen(10);
    const auto tmp11 = gen(11);
    const auto tmp12 = gen(12);
    const auto tmp13 = gen(13);
    const auto tmp14 = gen(14);
    const auto tmp15 = gen(15);
    return _mm256_setr_epi16(tmp0, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7, tmp8, tmp9, tmp10, tmp11, tmp12, tmp13, tmp14, tmp15);
}
#endif

// constants {{{1
template <typename T> Vc_INTRINSIC Vector<T, VectorAbi::Avx>::Vector(VectorSpecialInitializerZero) : d{} {}

template <> Vc_INTRINSIC Vector<double, VectorAbi::Avx>::Vector(VectorSpecialInitializerOne) : d(AVX::setone_pd()) {}
template <> Vc_INTRINSIC Vector< float, VectorAbi::Avx>::Vector(VectorSpecialInitializerOne) : d(AVX::setone_ps()) {}
#ifdef Vc_IMPL_AVX2
template <> Vc_INTRINSIC Vector<   int, VectorAbi::Avx>::Vector(VectorSpecialInitializerOne) : d(AVX::setone_epi32()) {}
template <> Vc_INTRINSIC Vector<  uint, VectorAbi::Avx>::Vector(VectorSpecialInitializerOne) : d(AVX::setone_epu32()) {}
template <> Vc_INTRINSIC Vector< short, VectorAbi::Avx>::Vector(VectorSpecialInitializerOne) : d(AVX::setone_epi16()) {}
template <> Vc_INTRINSIC Vector<ushort, VectorAbi::Avx>::Vector(VectorSpecialInitializerOne) : d(AVX::setone_epu16()) {}
template <> Vc_INTRINSIC Vector< schar, VectorAbi::Avx>::Vector(VectorSpecialInitializerOne) : d(AVX::setone_epi8()) {}
template <> Vc_INTRINSIC Vector< uchar, VectorAbi::Avx>::Vector(VectorSpecialInitializerOne) : d(AVX::setone_epu8()) {}
#endif

template <typename T>
Vc_ALWAYS_INLINE Vector<T, VectorAbi::Avx>::Vector(
    VectorSpecialInitializerIndexesFromZero)
    : Vector(AVX::IndexesFromZeroData<T>::address(), Vc::Aligned)
{
}

template <>
Vc_ALWAYS_INLINE Vector<float, VectorAbi::Avx>::Vector(VectorSpecialInitializerIndexesFromZero)
    : Vector(AVX::IndexesFromZeroData<int>::address(), Vc::Aligned)
{
}
template <>
Vc_ALWAYS_INLINE Vector<double, VectorAbi::Avx>::Vector(VectorSpecialInitializerIndexesFromZero)
    : Vector(AVX::IndexesFromZeroData<int>::address(), Vc::Aligned)
{
}

///////////////////////////////////////////////////////////////////////////////////////////
// load member functions {{{1
// general load, implemented via LoadHelper {{{2
template <typename DstT>
template <typename SrcT, typename Flags>
Vc_INTRINSIC typename Vector<DstT, VectorAbi::Avx>::
#ifndef Vc_MSVC
template
#endif
load_concept<SrcT, Flags>::type Vector<DstT, VectorAbi::Avx>::load(const SrcT *mem, Flags flags)
{
    Common::handleLoadPrefetches(mem, flags);
    d.v() = Detail::load<VectorType, DstT>(mem, flags);
}

///////////////////////////////////////////////////////////////////////////////////////////
// zeroing {{{1
template<typename T> Vc_INTRINSIC void Vector<T, VectorAbi::Avx>::setZero()
{
    data() = Detail::zero<VectorType>();
}
template<typename T> Vc_INTRINSIC void Vector<T, VectorAbi::Avx>::setZero(const Mask &k)
{
    data() = Detail::andnot_(k.data(), data());
}
template<typename T> Vc_INTRINSIC void Vector<T, VectorAbi::Avx>::setZeroInverted(const Mask &k)
{
    data() = Detail::and_(k.data(), data());
}

template<> Vc_INTRINSIC void Vector<double, VectorAbi::Avx>::setQnan()
{
    data() = Detail::allone<VectorType>();
}
template<> Vc_INTRINSIC void Vector<double, VectorAbi::Avx>::setQnan(MaskArgument k)
{
    data() = _mm256_or_pd(data(), k.dataD());
}
template<> Vc_INTRINSIC void Vector<float, VectorAbi::Avx>::setQnan()
{
    data() = Detail::allone<VectorType>();
}
template<> Vc_INTRINSIC void Vector<float, VectorAbi::Avx>::setQnan(MaskArgument k)
{
    data() = _mm256_or_ps(data(), k.dataF());
}

///////////////////////////////////////////////////////////////////////////////////////////
// stores {{{1
template <typename T>
template <typename U,
          typename Flags,
          typename>
Vc_INTRINSIC void Vector<T, VectorAbi::Avx>::store(U *mem, Flags flags) const
{
    Common::handleStorePrefetches(mem, flags);
    HV::template store<Flags>(mem, data());
}

template <typename T>
template <typename U,
          typename Flags,
          typename>
Vc_INTRINSIC void Vector<T, VectorAbi::Avx>::store(U *mem, Mask mask, Flags flags) const
{
    Common::handleStorePrefetches(mem, flags);
    HV::template store<Flags>(mem, data(), mask.data());
}

///////////////////////////////////////////////////////////////////////////////////////////
// integer ops {{{1
#ifdef Vc_IMPL_AVX2
template <> Vc_ALWAYS_INLINE AVX2::Vector<   int> Vector<   int, VectorAbi::Avx>::operator<<(AsArg x) const { return _mm256_sllv_epi32(d.v(), x.d.v()); }
template <> Vc_ALWAYS_INLINE AVX2::Vector<  uint> Vector<  uint, VectorAbi::Avx>::operator<<(AsArg x) const { return _mm256_sllv_epi32(d.v(), x.d.v()); }
template <> Vc_ALWAYS_INLINE AVX2::Vector<   int> Vector<   int, VectorAbi::Avx>::operator>>(AsArg x) const { return _mm256_srav_epi32(d.v(), x.d.v()); }
template <> Vc_ALWAYS_INLINE AVX2::Vector<  uint> Vector<  uint, VectorAbi::Avx>::operator>>(AsArg x) const { return _mm256_srlv_epi32(d.v(), x.d.v()); }
template <> Vc_ALWAYS_INLINE AVX2::Vector< short> Vector< short, VectorAbi::Avx>::operator<<(AsArg x) const { return generate([&](int i) { return get(*this, i) << get(x, i); }); }
template <> Vc_ALWAYS_INLINE AVX2::Vector<ushort> Vector<ushort, VectorAbi::Avx>::operator<<(AsArg x) const { return generate([&](int i) { return get(*this, i) << get(x, i); }); }
template <> Vc_ALWAYS_INLINE AVX2::Vector< short> Vector< short, VectorAbi::Avx>::operator>>(AsArg x) const { return generate([&](int i) { return get(*this, i) >> get(x, i); }); }
template <> Vc_ALWAYS_INLINE AVX2::Vector<ushort> Vector<ushort, VectorAbi::Avx>::operator>>(AsArg x) const { return generate([&](int i) { return get(*this, i) >> get(x, i); }); }
template <typename T>
Vc_ALWAYS_INLINE AVX2::Vector<T> &Vector<T, VectorAbi::Avx>::operator<<=(AsArg x)
{
    static_assert(std::is_integral<T>::value,
                  "bitwise-operators can only be used with Vectors of integral type");
    return *this = *this << x;
}
template <typename T>
Vc_ALWAYS_INLINE AVX2::Vector<T> &Vector<T, VectorAbi::Avx>::operator>>=(AsArg x)
{
    static_assert(std::is_integral<T>::value,
                  "bitwise-operators can only be used with Vectors of integral type");
    return *this = *this >> x;
}
#endif

template<typename T> Vc_ALWAYS_INLINE AVX2::Vector<T> &Vector<T, VectorAbi::Avx>::operator>>=(int shift) {
    d.v() = Detail::shiftRight(d.v(), shift, T());
    return *static_cast<AVX2::Vector<T> *>(this);
}
template<typename T> Vc_ALWAYS_INLINE Vc_PURE AVX2::Vector<T> Vector<T, VectorAbi::Avx>::operator>>(int shift) const {
    return Detail::shiftRight(d.v(), shift, T());
}
template<typename T> Vc_ALWAYS_INLINE AVX2::Vector<T> &Vector<T, VectorAbi::Avx>::operator<<=(int shift) {
    d.v() = Detail::shiftLeft(d.v(), shift, T());
    return *static_cast<AVX2::Vector<T> *>(this);
}
template<typename T> Vc_ALWAYS_INLINE Vc_PURE AVX2::Vector<T> Vector<T, VectorAbi::Avx>::operator<<(int shift) const {
    return Detail::shiftLeft(d.v(), shift, T());
}

// isnegative {{{1
Vc_INTRINSIC Vc_CONST AVX2::float_m isnegative(AVX2::float_v x)
{
    return AVX::avx_cast<__m256>(AVX::srai_epi32<31>(
        AVX::avx_cast<__m256i>(_mm256_and_ps(AVX::setsignmask_ps(), x.data()))));
}
Vc_INTRINSIC Vc_CONST AVX2::double_m isnegative(AVX2::double_v x)
{
    return Mem::permute<X1, X1, X3, X3>(AVX::avx_cast<__m256>(AVX::srai_epi32<31>(
        AVX::avx_cast<__m256i>(_mm256_and_pd(AVX::setsignmask_pd(), x.data())))));
}
// gathers {{{1
#define Vc_GATHER_IMPL(V_)                                                               \
    template <>                                                                          \
    template <class MT, class IT, int Scale>                                             \
    inline void AVX2::V_::gatherImplementation(                                          \
        const Common::GatherArguments<MT, IT, Scale> &args)
#define Vc_M(i_) static_cast<value_type>(args.address[Scale * args.indexes[i_]])
Vc_GATHER_IMPL(double_v) { d.v() = _mm256_setr_pd(Vc_M(0), Vc_M(1), Vc_M(2), Vc_M(3)); }

Vc_GATHER_IMPL(float_v)
{
    d.v() = _mm256_setr_ps(Vc_M(0), Vc_M(1), Vc_M(2), Vc_M(3), Vc_M(4), Vc_M(5), Vc_M(6),
                           Vc_M(7));
}

#ifdef Vc_IMPL_AVX2
Vc_GATHER_IMPL(int_v)
{
    d.v() = _mm256_setr_epi32(Vc_M(0), Vc_M(1), Vc_M(2), Vc_M(3), Vc_M(4), Vc_M(5),
                              Vc_M(6), Vc_M(7));
}

Vc_GATHER_IMPL(uint_v)
{
    d.v() = _mm256_setr_epi32(Vc_M(0), Vc_M(1), Vc_M(2), Vc_M(3), Vc_M(4), Vc_M(5),
                              Vc_M(6), Vc_M(7));
}

Vc_GATHER_IMPL(short_v)
{
    d.v() = _mm256_setr_epi16(Vc_M(0), Vc_M(1), Vc_M(2), Vc_M(3), Vc_M(4), Vc_M(5),
                              Vc_M(6), Vc_M(7), Vc_M(8), Vc_M(9), Vc_M(10), Vc_M(11),
                              Vc_M(12), Vc_M(13), Vc_M(14), Vc_M(15));
}

Vc_GATHER_IMPL(ushort_v)
{
    d.v() = _mm256_setr_epi16(Vc_M(0), Vc_M(1), Vc_M(2), Vc_M(3), Vc_M(4), Vc_M(5),
                              Vc_M(6), Vc_M(7), Vc_M(8), Vc_M(9), Vc_M(10), Vc_M(11),
                              Vc_M(12), Vc_M(13), Vc_M(14), Vc_M(15));
}
#endif
#undef Vc_M
#undef Vc_GATHER_IMPL

template <class T>
template <class MT, class IT, int Scale>
inline void Vector<T, VectorAbi::Avx>::gatherImplementation(
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

template <typename T>
template <typename MT, typename IT>
inline void Vector<T, VectorAbi::Avx>::scatterImplementation(MT *mem, IT &&indexes) const
{
    Common::unrolled_loop<std::size_t, 0, Size>([&](std::size_t i) { mem[indexes[i]] = d.m(i); });
}

template <typename T>
template <typename MT, typename IT>
inline void Vector<T, VectorAbi::Avx>::scatterImplementation(MT *mem, IT &&indexes, MaskArgument mask) const
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
    Common::executeScatter(Selector(), *this, mem, std::forward<IT>(indexes), mask);
}

///////////////////////////////////////////////////////////////////////////////////////////
// operator- {{{1
#ifdef Vc_USE_BUILTIN_VECTOR_TYPES
template<typename T> Vc_ALWAYS_INLINE Vc_PURE AVX2::Vector<T> Vector<T, VectorAbi::Avx>::operator-() const
{
    return VectorType(-d.builtin());
}
#else
template<typename T> Vc_ALWAYS_INLINE Vc_PURE AVX2::Vector<T> Vector<T, VectorAbi::Avx>::operator-() const
{
    return Detail::negate(d.v(), std::integral_constant<std::size_t, sizeof(T)>());
}
#endif

///////////////////////////////////////////////////////////////////////////////////////////
// horizontal ops {{{1
template <typename T>
Vc_INTRINSIC std::pair<Vector<T, VectorAbi::Avx>, int>
Vector<T, VectorAbi::Avx>::minIndex() const
{
    AVX2::Vector<T> x = min();
    return std::make_pair(x, (*this == x).firstOne());
}
template <typename T>
Vc_INTRINSIC std::pair<Vector<T, VectorAbi::Avx>, int>
Vector<T, VectorAbi::Avx>::maxIndex() const
{
    AVX2::Vector<T> x = max();
    return std::make_pair(x, (*this == x).firstOne());
}
template <> Vc_INTRINSIC std::pair<AVX2::float_v, int> AVX2::float_v::minIndex() const
{
    /*
    // 28 cycles latency:
    __m256 x = _mm256_min_ps(Mem::permute128<X1, X0>(d.v()), d.v());
    x = _mm256_min_ps(x, Reg::permute<X2, X3, X0, X1>(x));
    AVX2::float_v xx = _mm256_min_ps(x, Reg::permute<X1, X0, X3, X2>(x));
    AVX2::uint_v idx = AVX2::uint_v::IndexesFromZero();
    idx = _mm256_castps_si256(
        _mm256_or_ps((*this != xx).data(), _mm256_castsi256_ps(idx.data())));
    return std::make_pair(xx, (*this == xx).firstOne());

    __m128 loData = AVX::lo128(d.v());
    __m128 hiData = AVX::hi128(d.v());
    const __m128 less2 = _mm_cmplt_ps(hiData, loData);
    loData = _mm_min_ps(loData, hiData);
    hiData = Mem::permute<X2, X3, X0, X1>(loData);
    const __m128 less1 = _mm_cmplt_ps(hiData, loData);
    loData = _mm_min_ps(loData, hiData);
    hiData = Mem::permute<X1, X0, X3, X2>(loData);
    const __m128 less0 = _mm_cmplt_ps(hiData, loData);
    unsigned bits = _mm_movemask_ps(less0) & 0x1;
    bits |= ((_mm_movemask_ps(less1) << 1) - bits) & 0x2;
    bits |= ((_mm_movemask_ps(less2) << 3) - bits) & 0x4;
    loData = _mm_min_ps(loData, hiData);
    return std::make_pair(AVX::concat(loData, loData), bits);
    */

    // 28 cycles Latency:
    __m256 x = d.v();
    __m256 idx = Vector<float>::IndexesFromZero().data();
    __m256 y = Mem::permute128<X1, X0>(x);
    __m256 idy = Mem::permute128<X1, X0>(idx);
    __m256 less = AVX::cmplt_ps(x, y);

    x = _mm256_blendv_ps(y, x, less);
    idx = _mm256_blendv_ps(idy, idx, less);
    y = Reg::permute<X2, X3, X0, X1>(x);
    idy = Reg::permute<X2, X3, X0, X1>(idx);
    less = AVX::cmplt_ps(x, y);

    x = _mm256_blendv_ps(y, x, less);
    idx = _mm256_blendv_ps(idy, idx, less);
    y = Reg::permute<X1, X0, X3, X2>(x);
    idy = Reg::permute<X1, X0, X3, X2>(idx);
    less = AVX::cmplt_ps(x, y);

    idx = _mm256_blendv_ps(idy, idx, less);

    const auto index = _mm_cvtsi128_si32(AVX::avx_cast<__m128i>(idx));
#ifdef Vc_GNU_ASM
    __asm__ __volatile__(""); // help GCC to order the instructions better
#endif
    x = _mm256_blendv_ps(y, x, less);
    return std::make_pair(x, index);
}
template<typename T> Vc_ALWAYS_INLINE AVX2::Vector<T> Vector<T, VectorAbi::Avx>::partialSum() const
{
    //   a    b    c    d    e    f    g    h
    // +      a    b    c    d    e    f    g    -> a ab bc  cd   de    ef     fg      gh
    // +           a    ab   bc   cd   de   ef   -> a ab abc abcd bcde  cdef   defg    efgh
    // +                     a    ab   abc  abcd -> a ab abc abcd abcde abcdef abcdefg abcdefgh
    AVX2::Vector<T> tmp = *this;
    if (Size >  1) tmp += tmp.shifted(-1);
    if (Size >  2) tmp += tmp.shifted(-2);
    if (Size >  4) tmp += tmp.shifted(-4);
    if (Size >  8) tmp += tmp.shifted(-8);
    if (Size > 16) tmp += tmp.shifted(-16);
    return tmp;
}

/* This function requires correct masking because the neutral element of \p op is not necessarily 0
 *
template<typename T> template<typename BinaryOperation> Vc_ALWAYS_INLINE AVX2::Vector<T> Vector<T, VectorAbi::Avx>::partialSum(BinaryOperation op) const
{
    //   a    b    c    d    e    f    g    h
    // +      a    b    c    d    e    f    g    -> a ab bc  cd   de    ef     fg      gh
    // +           a    ab   bc   cd   de   ef   -> a ab abc abcd bcde  cdef   defg    efgh
    // +                     a    ab   abc  abcd -> a ab abc abcd abcde abcdef abcdefg abcdefgh
    AVX2::Vector<T> tmp = *this;
    Mask mask(true);
    if (Size >  1) tmp(mask) = op(tmp, tmp.shifted(-1));
    if (Size >  2) tmp(mask) = op(tmp, tmp.shifted(-2));
    if (Size >  4) tmp(mask) = op(tmp, tmp.shifted(-4));
    if (Size >  8) tmp(mask) = op(tmp, tmp.shifted(-8));
    if (Size > 16) tmp(mask) = op(tmp, tmp.shifted(-16));
    return tmp;
}
*/

template<typename T> Vc_ALWAYS_INLINE typename Vector<T, VectorAbi::Avx>::EntryType Vector<T, VectorAbi::Avx>::min(MaskArgument m) const
{
    AVX2::Vector<T> tmp = std::numeric_limits<AVX2::Vector<T> >::max();
    tmp(m) = *this;
    return tmp.min();
}
template<typename T> Vc_ALWAYS_INLINE typename Vector<T, VectorAbi::Avx>::EntryType Vector<T, VectorAbi::Avx>::max(MaskArgument m) const
{
    AVX2::Vector<T> tmp = std::numeric_limits<AVX2::Vector<T> >::min();
    tmp(m) = *this;
    return tmp.max();
}
template<typename T> Vc_ALWAYS_INLINE typename Vector<T, VectorAbi::Avx>::EntryType Vector<T, VectorAbi::Avx>::product(MaskArgument m) const
{
    AVX2::Vector<T> tmp(Vc::One);
    tmp(m) = *this;
    return tmp.product();
}
template<typename T> Vc_ALWAYS_INLINE typename Vector<T, VectorAbi::Avx>::EntryType Vector<T, VectorAbi::Avx>::sum(MaskArgument m) const
{
    AVX2::Vector<T> tmp(Vc::Zero);
    tmp(m) = *this;
    return tmp.sum();
}//}}}
// exponent {{{1
namespace Detail
{
Vc_INTRINSIC Vc_CONST __m256 exponent(__m256 v)
{
    using namespace AVX;
    __m128i tmp0 = _mm_srli_epi32(avx_cast<__m128i>(v), 23);
    __m128i tmp1 = _mm_srli_epi32(avx_cast<__m128i>(hi128(v)), 23);
    tmp0 = _mm_sub_epi32(tmp0, _mm_set1_epi32(0x7f));
    tmp1 = _mm_sub_epi32(tmp1, _mm_set1_epi32(0x7f));
    return _mm256_cvtepi32_ps(concat(tmp0, tmp1));
}
Vc_INTRINSIC Vc_CONST __m256d exponent(__m256d v)
{
    using namespace AVX;
    __m128i tmp0 = _mm_srli_epi64(avx_cast<__m128i>(v), 52);
    __m128i tmp1 = _mm_srli_epi64(avx_cast<__m128i>(hi128(v)), 52);
    tmp0 = _mm_sub_epi32(tmp0, _mm_set1_epi32(0x3ff));
    tmp1 = _mm_sub_epi32(tmp1, _mm_set1_epi32(0x3ff));
    return _mm256_cvtepi32_pd(avx_cast<__m128i>(Mem::shuffle<X0, X2, Y0, Y2>(avx_cast<__m128>(tmp0), avx_cast<__m128>(tmp1))));
}
} // namespace Detail

Vc_INTRINSIC Vc_CONST AVX2::float_v exponent(AVX2::float_v x)
{
    using Detail::operator>=;
    Vc_ASSERT((x >= x.Zero()).isFull());
    return Detail::exponent(x.data());
}
Vc_INTRINSIC Vc_CONST AVX2::double_v exponent(AVX2::double_v x)
{
    using Detail::operator>=;
    Vc_ASSERT((x >= x.Zero()).isFull());
    return Detail::exponent(x.data());
}
// }}}1
// Random {{{1
static Vc_ALWAYS_INLINE __m256i _doRandomStep()
{
    using Detail::operator*;
    using Detail::operator+;
#ifdef Vc_IMPL_AVX2
    using AVX2::uint_v;
    uint_v state0(&Common::RandomState[0]);
    uint_v state1(&Common::RandomState[uint_v::Size]);
    (state1 * uint_v(0xdeece66du) + uint_v(11)).store(&Common::RandomState[uint_v::Size]);
    uint_v(Detail::xor_((state0 * uint_v(0xdeece66du) + uint_v(11)).data(),
                        _mm256_srli_epi32(state1.data(), 16)))
        .store(&Common::RandomState[0]);
    return state0.data();
#else
    using SSE::uint_v;
    uint_v state0(&Common::RandomState[0]);
    uint_v state1(&Common::RandomState[uint_v::Size]);
    uint_v state2(&Common::RandomState[2 * uint_v::Size]);
    uint_v state3(&Common::RandomState[3 * uint_v::Size]);
    (state2 * uint_v(0xdeece66du) + uint_v(11))
        .store(&Common::RandomState[2 * uint_v::Size]);
    (state3 * uint_v(0xdeece66du) + uint_v(11))
        .store(&Common::RandomState[3 * uint_v::Size]);
    uint_v(Detail::xor_((state0 * uint_v(0xdeece66du) + uint_v(11)).data(),
                        _mm_srli_epi32(state2.data(), 16)))
        .store(&Common::RandomState[0]);
    uint_v(Detail::xor_((state1 * uint_v(0xdeece66du) + uint_v(11)).data(),
                        _mm_srli_epi32(state3.data(), 16)))
        .store(&Common::RandomState[uint_v::Size]);
    return AVX::concat(state0.data(), state1.data());
#endif
}

#ifdef Vc_IMPL_AVX2
template<typename T> Vc_ALWAYS_INLINE AVX2::Vector<T> Vector<T, VectorAbi::Avx>::Random()
{
    return {_doRandomStep()};
}
#endif

template <> Vc_ALWAYS_INLINE AVX2::float_v AVX2::float_v::Random()
{
    return HT::sub(Detail::or_(_cast(AVX::srli_epi32<2>(_doRandomStep())), HT::one()),
                   HT::one());
}

template<> Vc_ALWAYS_INLINE AVX2::double_v AVX2::double_v::Random()
{
    const __m256i state = Detail::load(&Common::RandomState[0], Vc::Aligned,
                                       Detail::LoadTag<__m256i, int>());
    for (size_t k = 0; k < 8; k += 2) {
        typedef unsigned long long uint64 Vc_MAY_ALIAS;
        const uint64 stateX = *aliasing_cast<uint64>(&Common::RandomState[k]);
        *aliasing_cast<uint64>(&Common::RandomState[k]) = (stateX * 0x5deece66dull + 11);
    }
    return HT::sub(Detail::or_(_cast(AVX::srli_epi64<12>(state)), HT::one()), HT::one());
}
// }}}1
// shifted / rotated {{{1
template<typename T> Vc_INTRINSIC AVX2::Vector<T> Vector<T, VectorAbi::Avx>::shifted(int amount) const
{
    return Detail::shifted<EntryType>(d.v(), amount);
}

template <typename VectorType>
Vc_INTRINSIC Vc_CONST VectorType shifted_shortcut(VectorType left, VectorType right, Common::WidthT<__m128>)
{
    return Mem::shuffle<X2, X3, Y0, Y1>(left, right);
}
template <typename VectorType>
Vc_INTRINSIC Vc_CONST VectorType shifted_shortcut(VectorType left, VectorType right, Common::WidthT<__m256>)
{
    return Mem::shuffle128<X1, Y0>(left, right);
}

template<typename T> Vc_INTRINSIC AVX2::Vector<T> Vector<T, VectorAbi::Avx>::shifted(int amount, Vector shiftIn) const
{
#ifdef __GNUC__
    if (__builtin_constant_p(amount)) {
        const __m256i a = AVX::avx_cast<__m256i>(d.v());
        const __m256i b = AVX::avx_cast<__m256i>(shiftIn.d.v());
        if (amount * 2 == int(Size)) {
            return shifted_shortcut(d.v(), shiftIn.d.v(), WidthT());
        }
        if (amount * 2 == -int(Size)) {
            return shifted_shortcut(shiftIn.d.v(), d.v(), WidthT());
        }
        switch (amount) {
        case 1:
            return AVX::avx_cast<VectorType>(
#ifdef Vc_IMPL_AVX2
                _mm256_alignr_epi8(_mm256_permute2x128_si256(a, b, 0x21), a,
                                   sizeof(EntryType))
#else  // Vc_IMPL_AVX2
                AVX::concat(
                    _mm_alignr_epi8(AVX::hi128(a), AVX::lo128(a), sizeof(EntryType)),
                    _mm_alignr_epi8(AVX::lo128(b), AVX::hi128(a), sizeof(EntryType)))
#endif  // Vc_IMPL_AVX2
                    );
        case 2:
            return AVX::avx_cast<VectorType>(
#ifdef Vc_IMPL_AVX2
                _mm256_alignr_epi8(_mm256_permute2x128_si256(a, b, 0x21), a,
                                   2 * sizeof(EntryType))
#else  // Vc_IMPL_AVX2
                AVX::concat(
                    _mm_alignr_epi8(AVX::hi128(a), AVX::lo128(a), 2 * sizeof(EntryType)),
                    _mm_alignr_epi8(AVX::lo128(b), AVX::hi128(a), 2 * sizeof(EntryType)))
#endif  // Vc_IMPL_AVX2
                    );
        case 3:
            if (6u < Size) {
                return AVX::avx_cast<VectorType>(
#ifdef Vc_IMPL_AVX2
                    _mm256_alignr_epi8(_mm256_permute2x128_si256(a, b, 0x21), a,
                                       3 * sizeof(EntryType))
#else   // Vc_IMPL_AVX2
                    AVX::concat(_mm_alignr_epi8(AVX::hi128(a), AVX::lo128(a),
                                                3 * sizeof(EntryType)),
                                _mm_alignr_epi8(AVX::lo128(b), AVX::hi128(a),
                                                3 * sizeof(EntryType)))
#endif  // Vc_IMPL_AVX2
                        );
            // TODO: } else {
            }
        }
    }
#endif
    using Detail::operator|;
    return shifted(amount) | (amount > 0 ?
                              shiftIn.shifted(amount - Size) :
                              shiftIn.shifted(Size + amount));
}
template<typename T> Vc_INTRINSIC AVX2::Vector<T> Vector<T, VectorAbi::Avx>::rotated(int amount) const
{
    return Detail::rotated<EntryType, size()>(d.v(), amount);
}
// sorted {{{1
template <typename T>
Vc_ALWAYS_INLINE Vc_PURE Vector<T, VectorAbi::Avx> Vector<T, VectorAbi::Avx>::sorted()
    const
{
    return Detail::sorted(*this);
}
// interleaveLow/-High {{{1
template <> Vc_INTRINSIC AVX2::double_v AVX2::double_v::interleaveLow(AVX2::double_v x) const
{
    return Mem::shuffle128<X0, Y0>(_mm256_unpacklo_pd(data(), x.data()),
                                   _mm256_unpackhi_pd(data(), x.data()));
}
template <> Vc_INTRINSIC AVX2::double_v AVX2::double_v::interleaveHigh(AVX2::double_v x) const
{
    return Mem::shuffle128<X1, Y1>(_mm256_unpacklo_pd(data(), x.data()),
                                   _mm256_unpackhi_pd(data(), x.data()));
}
template <> Vc_INTRINSIC AVX2::float_v AVX2::float_v::interleaveLow(AVX2::float_v x) const
{
    return Mem::shuffle128<X0, Y0>(_mm256_unpacklo_ps(data(), x.data()),
                                   _mm256_unpackhi_ps(data(), x.data()));
}
template <> Vc_INTRINSIC AVX2::float_v AVX2::float_v::interleaveHigh(AVX2::float_v x) const
{
    return Mem::shuffle128<X1, Y1>(_mm256_unpacklo_ps(data(), x.data()),
                                   _mm256_unpackhi_ps(data(), x.data()));
}
#ifdef Vc_IMPL_AVX2
template <> Vc_INTRINSIC    AVX2::int_v    AVX2::int_v::interleaveLow (   AVX2::int_v x) const {
    return Mem::shuffle128<X0, Y0>(_mm256_unpacklo_epi32(data(), x.data()),
                                   _mm256_unpackhi_epi32(data(), x.data()));
}
template <> Vc_INTRINSIC    AVX2::int_v    AVX2::int_v::interleaveHigh(   AVX2::int_v x) const {
    return Mem::shuffle128<X1, Y1>(_mm256_unpacklo_epi32(data(), x.data()),
                                   _mm256_unpackhi_epi32(data(), x.data()));
}
template <> Vc_INTRINSIC   AVX2::uint_v   AVX2::uint_v::interleaveLow (  AVX2::uint_v x) const {
    return Mem::shuffle128<X0, Y0>(_mm256_unpacklo_epi32(data(), x.data()),
                                   _mm256_unpackhi_epi32(data(), x.data()));
}
template <> Vc_INTRINSIC   AVX2::uint_v   AVX2::uint_v::interleaveHigh(  AVX2::uint_v x) const {
    return Mem::shuffle128<X1, Y1>(_mm256_unpacklo_epi32(data(), x.data()),
                                   _mm256_unpackhi_epi32(data(), x.data()));
}
template <> Vc_INTRINSIC  AVX2::short_v  AVX2::short_v::interleaveLow ( AVX2::short_v x) const {
    return Mem::shuffle128<X0, Y0>(_mm256_unpacklo_epi16(data(), x.data()),
                                   _mm256_unpackhi_epi16(data(), x.data()));
}
template <> Vc_INTRINSIC  AVX2::short_v  AVX2::short_v::interleaveHigh( AVX2::short_v x) const {
    return Mem::shuffle128<X1, Y1>(_mm256_unpacklo_epi16(data(), x.data()),
                                   _mm256_unpackhi_epi16(data(), x.data()));
}
template <> Vc_INTRINSIC AVX2::ushort_v AVX2::ushort_v::interleaveLow (AVX2::ushort_v x) const {
    return Mem::shuffle128<X0, Y0>(_mm256_unpacklo_epi16(data(), x.data()),
                                   _mm256_unpackhi_epi16(data(), x.data()));
}
template <> Vc_INTRINSIC AVX2::ushort_v AVX2::ushort_v::interleaveHigh(AVX2::ushort_v x) const {
    return Mem::shuffle128<X1, Y1>(_mm256_unpacklo_epi16(data(), x.data()),
                                   _mm256_unpackhi_epi16(data(), x.data()));
}
#endif
// permutation via operator[] {{{1
template <> Vc_INTRINSIC Vc_PURE AVX2::double_v AVX2::double_v::operator[](Permutation::ReversedTag) const
{
    return Mem::permute128<X1, X0>(Mem::permute<X1, X0, X3, X2>(d.v()));
}
template <> Vc_INTRINSIC Vc_PURE AVX2::float_v AVX2::float_v::operator[](Permutation::ReversedTag) const
{
    return Mem::permute128<X1, X0>(Mem::permute<X3, X2, X1, X0>(d.v()));
}
#ifdef Vc_IMPL_AVX2
template <>
Vc_INTRINSIC Vc_PURE AVX2::int_v AVX2::int_v::operator[](Permutation::ReversedTag) const
{
    return Mem::permute128<X1, X0>(Mem::permute<X3, X2, X1, X0>(d.v()));
}
template <>
Vc_INTRINSIC Vc_PURE AVX2::uint_v AVX2::uint_v::operator[](Permutation::ReversedTag) const
{
    return Mem::permute128<X1, X0>(Mem::permute<X3, X2, X1, X0>(d.v()));
}
template <>
Vc_INTRINSIC Vc_PURE AVX2::short_v AVX2::short_v::operator[](
    Permutation::ReversedTag) const
{
    return Mem::permute128<X1, X0>(AVX::avx_cast<__m256i>(Mem::shuffle<X1, Y0, X3, Y2>(
        AVX::avx_cast<__m256d>(Mem::permuteHi<X7, X6, X5, X4>(d.v())),
        AVX::avx_cast<__m256d>(Mem::permuteLo<X3, X2, X1, X0>(d.v())))));
}
template <>
Vc_INTRINSIC Vc_PURE AVX2::ushort_v AVX2::ushort_v::operator[](
    Permutation::ReversedTag) const
{
    return Mem::permute128<X1, X0>(AVX::avx_cast<__m256i>(Mem::shuffle<X1, Y0, X3, Y2>(
        AVX::avx_cast<__m256d>(Mem::permuteHi<X7, X6, X5, X4>(d.v())),
        AVX::avx_cast<__m256d>(Mem::permuteLo<X3, X2, X1, X0>(d.v())))));
}
#endif
template <> Vc_INTRINSIC AVX2::float_v Vector<float, VectorAbi::Avx>::operator[](const IndexType &/*perm*/) const
{
    // TODO
    return *this;
#ifdef Vc_IMPL_AVX2
#else
    /*
    const int_m cross128 = AVX::concat(_mm_cmpgt_epi32(AVX::lo128(perm.data()), _mm_set1_epi32(3)),
                                  _mm_cmplt_epi32(AVX::hi128(perm.data()), _mm_set1_epi32(4)));
    if (cross128.isNotEmpty()) {
    AVX2::float_v x = _mm256_permutevar_ps(d.v(), perm.data());
        x(cross128) = _mm256_permutevar_ps(Mem::permute128<X1, X0>(d.v()), perm.data());
        return x;
    } else {
    */
#endif
}

// reversed {{{1
template <typename T>
Vc_INTRINSIC Vc_PURE Vector<T, VectorAbi::Avx> Vector<T, VectorAbi::Avx>::reversed() const
{
    return (*this)[Permutation::Reversed];
}

// broadcast from constexpr index {{{1
template <> template <int Index> Vc_INTRINSIC AVX2::float_v AVX2::float_v::broadcast() const
{
    constexpr VecPos Inner = static_cast<VecPos>(Index & 0x3);
    constexpr VecPos Outer = static_cast<VecPos>((Index & 0x4) / 4);
    return Mem::permute<Inner, Inner, Inner, Inner>(Mem::permute128<Outer, Outer>(d.v()));
}
template <> template <int Index> Vc_INTRINSIC AVX2::double_v AVX2::double_v::broadcast() const
{
    constexpr VecPos Inner = static_cast<VecPos>(Index & 0x1);
    constexpr VecPos Outer = static_cast<VecPos>((Index & 0x2) / 2);
    return Mem::permute<Inner, Inner>(Mem::permute128<Outer, Outer>(d.v()));
}
// }}}1
}  // namespace Vc

// vim: foldmethod=marker
