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

#ifndef VC_SSE_VECTOR_H_
#define VC_SSE_VECTOR_H_

#include "../scalar/vector.h"
#include "intrinsics.h"
#include "types.h"
#include "vectorhelper.h"
#include "mask.h"
#include "../common/writemaskedvector.h"
#include "../common/aliasingentryhelper.h"
#include "../common/memoryfwd.h"
#include "../common/loadstoreflags.h"
#include <algorithm>
#include <cmath>
#include "detail.h"

#include "macros.h"

#ifdef isfinite
#undef isfinite
#endif
#ifdef isnan
#undef isnan
#endif

namespace Vc_VERSIONED_NAMESPACE
{

#define Vc_CURRENT_CLASS_NAME Vector
template <typename T> class Vector<T, VectorAbi::Sse>
{
    static_assert(std::is_arithmetic<T>::value,
                  "Vector<T> only accepts arithmetic builtin types as template parameter T.");

    protected:
#ifdef Vc_COMPILE_BENCHMARKS
    public:
#endif
        typedef typename SSE::VectorTraits<T>::StorageType StorageType;
        StorageType d;
        typedef typename SSE::VectorTraits<T>::GatherMaskType GatherMask;
        typedef SSE::VectorHelper<typename SSE::VectorTraits<T>::VectorType> HV;
        typedef SSE::VectorHelper<T> HT;
    public:
        Vc_FREE_STORE_OPERATORS_ALIGNED(16);

        typedef typename SSE::VectorTraits<T>::VectorType VectorType;
        using vector_type = VectorType;
        static constexpr size_t Size = SSE::VectorTraits<T>::Size;
        static constexpr size_t MemoryAlignment = alignof(VectorType);
        typedef typename SSE::VectorTraits<T>::EntryType EntryType;
        using value_type = EntryType;
        using VectorEntryType = EntryType;
        using IndexType = fixed_size_simd<int, Size>;
        using index_type = IndexType;
        typedef typename SSE::VectorTraits<T>::MaskType Mask;
        using MaskType = Mask;
        using mask_type = Mask;
        typedef typename Mask::Argument MaskArg;
        typedef typename Mask::Argument MaskArgument;
        typedef const Vector AsArg;
        using abi = VectorAbi::Sse;
        using WriteMaskedVector = Common::WriteMaskedVector<Vector, Mask>;
        template <typename U> using V = Vector<U, abi>;

        using reference = Detail::ElementReference<Vector>;

#include "../common/generalinterface.h"

        static Vc_INTRINSIC_L Vector Random() Vc_INTRINSIC_R;

        ///////////////////////////////////////////////////////////////////////////////////////////
        // internal: required to enable returning objects of VectorType
        Vc_ALWAYS_INLINE Vector(VectorType x) : d(x) {}

        // implict conversion from compatible Vector<U>
        template <typename U>
        Vc_INTRINSIC Vector(
            V<U> x, typename std::enable_if<Traits::is_implicit_cast_allowed<U, T>::value,
                                            void *>::type = nullptr)
            : d(SSE::convert<U, T>(x.data()))
        {
        }

#if Vc_IS_VERSION_1
        // static_cast from the remaining Vector<U>
        template <typename U>
        Vc_DEPRECATED("use simd_cast instead of explicit type casting to convert between "
                      "vector types") Vc_INTRINSIC
            explicit Vector(
                V<U> x,
                typename std::enable_if<!Traits::is_implicit_cast_allowed<U, T>::value,
                                        void *>::type = nullptr)
            : d(SSE::convert<U, T>(x.data()))
        {
        }
#endif

        ///////////////////////////////////////////////////////////////////////////////////////////
        // broadcast
        Vc_INTRINSIC Vector(EntryType a) : d(HT::set(a)) {}
        template <typename U>
        Vc_INTRINSIC Vector(U a,
                            typename std::enable_if<std::is_same<U, int>::value &&
                                                        !std::is_same<U, EntryType>::value,
                                                    void *>::type = nullptr)
            : Vector(static_cast<EntryType>(a))
        {
        }

        Vc_INTRINSIC explicit Vector(reference a) : Vector(static_cast<EntryType>(a)) {}

#include "../common/loadinterface.h"
#include "../common/storeinterface.h"

        ///////////////////////////////////////////////////////////////////////////////////////////
        // zeroing
        Vc_INTRINSIC_L void setZero() Vc_INTRINSIC_R;
        Vc_INTRINSIC_L void setZero(const Mask &k) Vc_INTRINSIC_R;
        Vc_INTRINSIC_L void setZeroInverted(const Mask &k) Vc_INTRINSIC_R;

        Vc_INTRINSIC_L void setQnan() Vc_INTRINSIC_R;
        Vc_INTRINSIC_L void setQnan(const Mask &k) Vc_INTRINSIC_R;

#include "../common/gatherinterface.h"
#include "../common/scatterinterface.h"
#if defined Vc_IMPL_AVX2 && !defined Vc_MSVC
        // skip this code for MSVC because it fails to do overload resolution correctly

        ////////////////////////////////////////////////////////////////////////////////
        // non-converting pd, ps, and epi32 gathers
        template <class U, class A, int Scale, int N = Vector<U, A>::size(),
                  class = enable_if<(Vector<U, A>::size() >= size() && sizeof(T) >= 4)>>
        Vc_INTRINSIC void gatherImplementation(
            const Common::GatherArguments<T, Vector<U, A>, Scale> &args)
        {
            d.v() = SSE::gather<sizeof(T) * Scale>(
                args.address, simd_cast<SSE::int_v>(args.indexes).data());
        }

        // masked overload
        template <class U, class A, int Scale, int N = Vector<U, A>::size(),
                  class = enable_if<(Vector<U, A>::size() >= size() && sizeof(T) >= 4)>>
        Vc_INTRINSIC void gatherImplementation(
            const Common::GatherArguments<T, Vector<U, A>, Scale> &args, MaskArgument k)
        {
            d.v() = SSE::gather<sizeof(T) * Scale>(
                d.v(), k.data(), args.address,
                simd_cast<SSE::int_v>(args.indexes).data());
        }

        ////////////////////////////////////////////////////////////////////////////////
        // converting (from 8-bit and 16-bit integers only) epi16 gather emulation via
        // epi32 gathers
        template <
            class MT, class U, class A, int Scale,
            class = enable_if<(sizeof(T) == 2 && std::is_integral<MT>::value &&
                               (sizeof(MT) <= 2) && Vector<U, A>::size() >= size())>>
        Vc_INTRINSIC void gatherImplementation(
            const Common::GatherArguments<MT, Vector<U, A>, Scale> &args)
        {
            using AVX2::int_v;
            const auto idx = simd_cast<int_v>(args.indexes).data();
            *this = simd_cast<Vector>(int_v(
                AVX::gather<sizeof(MT) * Scale>(aliasing_cast<int>(args.address), idx)));
            if (sizeof(MT) == 1) {
                if (std::is_signed<MT>::value) {
                    d.v() = _mm_srai_epi16(_mm_slli_epi16(d.v(), 8), 8);
                } else {
                    *this &= 0xff;
                }
            }
        }

        // masked overload
        template <
            class MT, class U, class A, int Scale,
            class = enable_if<(sizeof(T) == 2 && std::is_integral<MT>::value &&
                               (sizeof(MT) <= 2) && Vector<U, A>::size() >= size())>>
        Vc_INTRINSIC void gatherImplementation(
            const Common::GatherArguments<MT, Vector<U, A>, Scale> &args, MaskArgument k)
        {
            using AVX2::int_v;
            auto v = simd_cast<Vector>(int_v(AVX::gather<sizeof(MT) * Scale>(
                _mm256_setzero_si256(), simd_cast<AVX2::int_m>(k).data(),
                aliasing_cast<int>(args.address),
                simd_cast<int_v>(args.indexes).data())));
            if (sizeof(MT) == 1) {
                if (std::is_signed<MT>::value) {
                    v.data() = _mm_srai_epi16(_mm_slli_epi16(v.data(), 8), 8);
                } else {
                    v &= 0xff;
                }
            }
            assign(v, k);
        }

        ////////////////////////////////////////////////////////////////////////////////
        // all remaining converting gathers
        template <class MT, class U, class A, int Scale>
        Vc_INTRINSIC enable_if<((sizeof(T) != 2 || sizeof(MT) > 2) &&
                                Traits::is_valid_vector_argument<MT>::value &&
                                !std::is_same<MT, T>::value &&
                                Vector<U, A>::size() >= size()),
                               void>
        gatherImplementation(const Common::GatherArguments<MT, Vector<U, A>, Scale> &args)
        {
            *this = simd_cast<Vector>(fixed_size_simd<MT, Size>(args));
        }

        // masked overload
        template <class MT, class U, class A, int Scale>
        Vc_INTRINSIC enable_if<((sizeof(T) != 2 || sizeof(MT) > 2) &&
                                Traits::is_valid_vector_argument<MT>::value &&
                                !std::is_same<MT, T>::value &&
                                Vector<U, A>::size() >= size()),
                               void>
        gatherImplementation(const Common::GatherArguments<MT, Vector<U, A>, Scale> &args,
                             MaskArgument k)
        {
            assign(simd_cast<Vector>(fixed_size_simd<MT, Size>(args, k)), k);
        }
#endif  // Vc_IMPL_AVX2 && !MSVC

        //prefix
        Vc_INTRINSIC Vector &operator++() { data() = HT::add(data(), HT::one()); return *this; }
        Vc_INTRINSIC Vector &operator--() { data() = HT::sub(data(), HT::one()); return *this; }
        //postfix
        Vc_INTRINSIC Vector operator++(int) { const Vector r = *this; data() = HT::add(data(), HT::one()); return r; }
        Vc_INTRINSIC Vector operator--(int) { const Vector r = *this; data() = HT::sub(data(), HT::one()); return r; }

    private:
        friend reference;
        Vc_INTRINSIC static value_type get(const Vector &o, int i) noexcept
        {
            return o.d.m(i);
        }
        template <typename U>
        Vc_INTRINSIC static void set(Vector &o, int i, U &&v) noexcept(
            noexcept(std::declval<value_type &>() = v))
        {
            o.d.set(i, v);
        }

    public:
        /**
         * \note the returned object models the concept of a reference and
         * as such it can exist longer than the data it is referencing.
         * \note to avoid lifetime issues, we strongly advice not to store
         * any reference objects.
         */
        Vc_ALWAYS_INLINE reference operator[](size_t index) noexcept
        {
            static_assert(noexcept(reference{std::declval<Vector &>(), int()}), "");
            return {*this, int(index)};
        }
        Vc_ALWAYS_INLINE value_type operator[](size_t index) const noexcept
        {
            return d.m(index);
        }

        Vc_INTRINSIC_L Vector Vc_VDECL operator[](const SSE::int_v &perm) const Vc_INTRINSIC_R;

        Vc_INTRINSIC Vc_PURE Mask operator!() const
        {
            return *this == Zero();
        }
        Vc_INTRINSIC Vc_PURE Vector operator~() const
        {
#ifndef Vc_ENABLE_FLOAT_BIT_OPERATORS
            static_assert(std::is_integral<T>::value,
                          "bit-complement can only be used with Vectors of integral type");
#endif
            return Detail::andnot_(data(), HV::allone());
        }
        Vc_ALWAYS_INLINE_L Vc_PURE_L Vector operator-() const Vc_ALWAYS_INLINE_R Vc_PURE_R;
        Vc_INTRINSIC Vc_PURE Vector operator+() const { return *this; }

        Vc_ALWAYS_INLINE Vector  Vc_VDECL operator<< (AsArg shift) const { return generate([&](int i) { return get(*this, i) << get(shift, i); }); }
        Vc_ALWAYS_INLINE Vector  Vc_VDECL operator>> (AsArg shift) const { return generate([&](int i) { return get(*this, i) >> get(shift, i); }); }
        Vc_ALWAYS_INLINE Vector &Vc_VDECL operator<<=(AsArg shift) { return *this = *this << shift; }
        Vc_ALWAYS_INLINE Vector &Vc_VDECL operator>>=(AsArg shift) { return *this = *this >> shift; }

        Vc_INTRINSIC_L Vector &Vc_VDECL operator<<=(  int shift)       Vc_INTRINSIC_R;
        Vc_INTRINSIC_L Vector  Vc_VDECL operator<< (  int shift) const Vc_INTRINSIC_R;
        Vc_INTRINSIC_L Vector &Vc_VDECL operator>>=(  int shift)       Vc_INTRINSIC_R;
        Vc_INTRINSIC_L Vector  Vc_VDECL operator>> (  int shift) const Vc_INTRINSIC_R;

        Vc_DEPRECATED("use isnegative(x) instead") Vc_INTRINSIC Vc_PURE Mask
            isNegative() const
        {
            return Vc::isnegative(*this);
        }

        Vc_ALWAYS_INLINE void assign(const Vector &v, const Mask &mask)
        {
            data() = HV::blend(data(), v.data(), mask.data());
        }

        template <typename V2>
        Vc_DEPRECATED("Use simd_cast instead of Vector::staticCast")
            Vc_ALWAYS_INLINE Vc_PURE V2 staticCast() const
        {
            return SSE::convert<T, typename V2::EntryType>(data());
        }
        template <typename V2>
        Vc_DEPRECATED("use reinterpret_components_cast instead")
            Vc_ALWAYS_INLINE Vc_PURE V2 reinterpretCast() const
        {
            return SSE::sse_cast<typename V2::VectorType>(data());
        }

        Vc_INTRINSIC WriteMaskedVector operator()(const Mask &k) { return {*this, k}; }

        Vc_ALWAYS_INLINE Vc_PURE VectorType &data() { return d.v(); }
        Vc_ALWAYS_INLINE Vc_PURE const VectorType &data() const { return d.v(); }

        template<int Index>
        Vc_INTRINSIC_L Vector broadcast() const Vc_INTRINSIC_R;

        Vc_INTRINSIC EntryType min() const { return HT::min(data()); }
        Vc_INTRINSIC EntryType max() const { return HT::max(data()); }
        Vc_INTRINSIC EntryType product() const { return HT::mul(data()); }
        Vc_INTRINSIC EntryType sum() const { return HT::add(data()); }
        Vc_INTRINSIC_L Vector partialSum() const Vc_INTRINSIC_R;
        Vc_INTRINSIC_L EntryType min(MaskArg m) const Vc_INTRINSIC_R;
        Vc_INTRINSIC_L EntryType max(MaskArg m) const Vc_INTRINSIC_R;
        Vc_INTRINSIC_L EntryType product(MaskArg m) const Vc_INTRINSIC_R;
        Vc_INTRINSIC_L EntryType sum(MaskArg m) const Vc_INTRINSIC_R;

        Vc_INTRINSIC_L Vector shifted(int amount, Vector shiftIn) const Vc_INTRINSIC_R;
        Vc_INTRINSIC_L Vector shifted(int amount) const Vc_INTRINSIC_R;
        Vc_INTRINSIC_L Vector rotated(int amount) const Vc_INTRINSIC_R;
        Vc_INTRINSIC_L Vc_PURE_L Vector reversed() const Vc_INTRINSIC_R Vc_PURE_R;
        Vc_ALWAYS_INLINE_L Vc_PURE_L Vector sorted() const Vc_ALWAYS_INLINE_R Vc_PURE_R;

        template <typename F> void callWithValuesSorted(F &&f)
        {
            EntryType value = d.m(0);
            f(value);
            for (std::size_t i = 1; i < Size; ++i) {
                if (d.m(i) != value) {
                    value = d.m(i);
                    f(value);
                }
            }
        }

        template <typename F> Vc_INTRINSIC void call(F &&f) const
        {
            Common::for_all_vector_entries<Size>([&](size_t i) { f(EntryType(d.m(i))); });
        }

        template <typename F> Vc_INTRINSIC void call(F &&f, const Mask &mask) const
        {
            for(size_t i : where(mask)) {
                f(EntryType(d.m(i)));
            }
        }

        template <typename F> Vc_INTRINSIC Vector apply(F &&f) const
        {
            Vector r;
            Common::for_all_vector_entries<Size>(
                [&](size_t i) { r.d.set(i, f(EntryType(d.m(i)))); });
            return r;
        }
        template <typename F> Vc_INTRINSIC Vector apply(F &&f, const Mask &mask) const
        {
            Vector r(*this);
            for (size_t i : where(mask)) {
                r.d.set(i, f(EntryType(r.d.m(i))));
            }
            return r;
        }

        template<typename IndexT> Vc_INTRINSIC void fill(EntryType (&f)(IndexT)) {
            Common::for_all_vector_entries<Size>([&](size_t i) { d.set(i, f(i)); });
        }
        Vc_INTRINSIC void fill(EntryType (&f)()) {
            Common::for_all_vector_entries<Size>([&](size_t i) { d.set(i, f()); });
        }

        template <typename G> static Vc_INTRINSIC_L Vector generate(G gen) Vc_INTRINSIC_R;

        Vc_DEPRECATED("use copysign(x, y) instead") Vc_INTRINSIC Vector
            copySign(AsArg x) const
        {
            return Vc::copysign(*this, x);
        }

        Vc_DEPRECATED("use exponent(x) instead") Vc_INTRINSIC Vector exponent() const
        {
            return Vc::exponent(*this);
        }

        Vc_INTRINSIC_L Vector interleaveLow(Vector x) const Vc_INTRINSIC_R;
        Vc_INTRINSIC_L Vector interleaveHigh(Vector x) const Vc_INTRINSIC_R;
};
#undef Vc_CURRENT_CLASS_NAME
template <typename T> constexpr size_t Vector<T, VectorAbi::Sse>::Size;
template <typename T> constexpr size_t Vector<T, VectorAbi::Sse>::MemoryAlignment;

static Vc_ALWAYS_INLINE Vc_PURE SSE::int_v    min(const SSE::int_v    &x, const SSE::int_v    &y) { return SSE::min_epi32(x.data(), y.data()); }
static Vc_ALWAYS_INLINE Vc_PURE SSE::uint_v   min(const SSE::uint_v   &x, const SSE::uint_v   &y) { return SSE::min_epu32(x.data(), y.data()); }
static Vc_ALWAYS_INLINE Vc_PURE SSE::short_v  min(const SSE::short_v  &x, const SSE::short_v  &y) { return _mm_min_epi16(x.data(), y.data()); }
static Vc_ALWAYS_INLINE Vc_PURE SSE::ushort_v min(const SSE::ushort_v &x, const SSE::ushort_v &y) { return SSE::min_epu16(x.data(), y.data()); }
static Vc_ALWAYS_INLINE Vc_PURE SSE::float_v  min(const SSE::float_v  &x, const SSE::float_v  &y) { return _mm_min_ps(x.data(), y.data()); }
static Vc_ALWAYS_INLINE Vc_PURE SSE::double_v min(const SSE::double_v &x, const SSE::double_v &y) { return _mm_min_pd(x.data(), y.data()); }
static Vc_ALWAYS_INLINE Vc_PURE SSE::int_v    max(const SSE::int_v    &x, const SSE::int_v    &y) { return SSE::max_epi32(x.data(), y.data()); }
static Vc_ALWAYS_INLINE Vc_PURE SSE::uint_v   max(const SSE::uint_v   &x, const SSE::uint_v   &y) { return SSE::max_epu32(x.data(), y.data()); }
static Vc_ALWAYS_INLINE Vc_PURE SSE::short_v  max(const SSE::short_v  &x, const SSE::short_v  &y) { return _mm_max_epi16(x.data(), y.data()); }
static Vc_ALWAYS_INLINE Vc_PURE SSE::ushort_v max(const SSE::ushort_v &x, const SSE::ushort_v &y) { return SSE::max_epu16(x.data(), y.data()); }
static Vc_ALWAYS_INLINE Vc_PURE SSE::float_v  max(const SSE::float_v  &x, const SSE::float_v  &y) { return _mm_max_ps(x.data(), y.data()); }
static Vc_ALWAYS_INLINE Vc_PURE SSE::double_v max(const SSE::double_v &x, const SSE::double_v &y) { return _mm_max_pd(x.data(), y.data()); }

template <typename T,
          typename = enable_if<std::is_same<T, double>::value || std::is_same<T, float>::value ||
                               std::is_same<T, short>::value ||
                               std::is_same<T, int>::value>>
Vc_ALWAYS_INLINE Vc_PURE Vector<T, VectorAbi::Sse> abs(Vector<T, VectorAbi::Sse> x)
{
    return SSE::VectorHelper<T>::abs(x.data());
}

  template<typename T> Vc_ALWAYS_INLINE Vc_PURE Vector<T, VectorAbi::Sse> sqrt (const Vector<T, VectorAbi::Sse> &x) { return SSE::VectorHelper<T>::sqrt(x.data()); }
  template<typename T> Vc_ALWAYS_INLINE Vc_PURE Vector<T, VectorAbi::Sse> rsqrt(const Vector<T, VectorAbi::Sse> &x) { return SSE::VectorHelper<T>::rsqrt(x.data()); }
  template<typename T> Vc_ALWAYS_INLINE Vc_PURE Vector<T, VectorAbi::Sse> reciprocal(const Vector<T, VectorAbi::Sse> &x) { return SSE::VectorHelper<T>::reciprocal(x.data()); }
  template<typename T> Vc_ALWAYS_INLINE Vc_PURE Vector<T, VectorAbi::Sse> round(const Vector<T, VectorAbi::Sse> &x) { return SSE::VectorHelper<T>::round(x.data()); }

  template<typename T> Vc_ALWAYS_INLINE Vc_PURE typename Vector<T, VectorAbi::Sse>::Mask isfinite(const Vector<T, VectorAbi::Sse> &x) { return SSE::VectorHelper<T>::isFinite(x.data()); }
  template<typename T> Vc_ALWAYS_INLINE Vc_PURE typename Vector<T, VectorAbi::Sse>::Mask isinf(const Vector<T, VectorAbi::Sse> &x) { return SSE::VectorHelper<T>::isInfinite(x.data()); }
  template<typename T> Vc_ALWAYS_INLINE Vc_PURE typename Vector<T, VectorAbi::Sse>::Mask isnan(const Vector<T, VectorAbi::Sse> &x) { return SSE::VectorHelper<T>::isNaN(x.data()); }

#define Vc_CONDITIONAL_ASSIGN(name_, op_)                                                \
    template <Operator O, typename T, typename M, typename U>                            \
    Vc_INTRINSIC enable_if<O == Operator::name_, void> conditional_assign(               \
        Vector<T, VectorAbi::Sse> &lhs, M &&mask, U &&rhs)                               \
    {                                                                                    \
        lhs(mask) op_ rhs;                                                               \
    }                                                                                    \
    Vc_NOTHING_EXPECTING_SEMICOLON
Vc_CONDITIONAL_ASSIGN(          Assign,  =);
Vc_CONDITIONAL_ASSIGN(      PlusAssign, +=);
Vc_CONDITIONAL_ASSIGN(     MinusAssign, -=);
Vc_CONDITIONAL_ASSIGN(  MultiplyAssign, *=);
Vc_CONDITIONAL_ASSIGN(    DivideAssign, /=);
Vc_CONDITIONAL_ASSIGN( RemainderAssign, %=);
Vc_CONDITIONAL_ASSIGN(       XorAssign, ^=);
Vc_CONDITIONAL_ASSIGN(       AndAssign, &=);
Vc_CONDITIONAL_ASSIGN(        OrAssign, |=);
Vc_CONDITIONAL_ASSIGN( LeftShiftAssign,<<=);
Vc_CONDITIONAL_ASSIGN(RightShiftAssign,>>=);
#undef Vc_CONDITIONAL_ASSIGN

#define Vc_CONDITIONAL_ASSIGN(name_, expr_)                                              \
    template <Operator O, typename T, typename M>                                        \
    Vc_INTRINSIC enable_if<O == Operator::name_, Vector<T, VectorAbi::Sse>>              \
    conditional_assign(Vector<T, VectorAbi::Sse> &lhs, M &&mask)                         \
    {                                                                                    \
        return expr_;                                                                    \
    }                                                                                    \
    Vc_NOTHING_EXPECTING_SEMICOLON
Vc_CONDITIONAL_ASSIGN(PostIncrement, lhs(mask)++);
Vc_CONDITIONAL_ASSIGN( PreIncrement, ++lhs(mask));
Vc_CONDITIONAL_ASSIGN(PostDecrement, lhs(mask)--);
Vc_CONDITIONAL_ASSIGN( PreDecrement, --lhs(mask));
#undef Vc_CONDITIONAL_ASSIGN

}  // namespace Vc

#include "vector.tcc"
#include "simd_cast.h"

#endif // VC_SSE_VECTOR_H_
