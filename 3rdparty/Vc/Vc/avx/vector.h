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

#ifndef VC_AVX_VECTOR_H_
#define VC_AVX_VECTOR_H_

#include "intrinsics.h"
#include "casts.h"
#include "../sse/vector.h"
#include "shuffle.h"
#include "vectorhelper.h"
#include "mask.h"
#include <algorithm>
#include <cmath>
#include "../common/aliasingentryhelper.h"
#include "../common/memoryfwd.h"
#include "../common/where.h"
#include "macros.h"

#ifdef isfinite
#undef isfinite
#endif
#ifdef isnan
#undef isnan
#endif

namespace Vc_VERSIONED_NAMESPACE
{
namespace Detail
{
template <typename T, typename Abi> struct VectorTraits
{
    using mask_type = Vc::Mask<T, Abi>;
    using vector_type = Vc::Vector<T, Abi>;
    using writemasked_vector_type = Common::WriteMaskedVector<vector_type, mask_type>;
    using intrinsic_type = typename AVX::VectorTypeHelper<T>::Type;
};
}  // namespace Detail

#define Vc_CURRENT_CLASS_NAME Vector
template <typename T> class Vector<T, VectorAbi::Avx>
{
public:
    using abi = VectorAbi::Avx;

private:
    using traits_type = Detail::VectorTraits<T, abi>;
    static_assert(
        std::is_arithmetic<T>::value,
        "Vector<T> only accepts arithmetic builtin types as template parameter T.");

    using WriteMaskedVector = typename traits_type::writemasked_vector_type;

public:
    using VectorType = typename traits_type::intrinsic_type;
    using vector_type = VectorType;

    using mask_type = typename traits_type::mask_type;
    using Mask = mask_type;
    using MaskType = mask_type;
    using MaskArg Vc_DEPRECATED_ALIAS("Use MaskArgument instead.") = typename Mask::AsArg;
    using MaskArgument = typename Mask::AsArg;
    using reference = Detail::ElementReference<Vector>;

    Vc_FREE_STORE_OPERATORS_ALIGNED(alignof(VectorType));

    using EntryType = T;
        using value_type = EntryType;
        typedef EntryType VectorEntryType;
        static constexpr size_t Size = sizeof(VectorType) / sizeof(EntryType);
        static constexpr size_t MemoryAlignment = alignof(VectorType);
    using IndexType = fixed_size_simd<int, Size>;
    using index_type = IndexType;
        typedef Vector<T, abi> AsArg;
        typedef VectorType VectorTypeArg;

    protected:
        template <typename U> using V = Vector<U, abi>;

        // helper that specializes on VectorType
        typedef AVX::VectorHelper<VectorType> HV;

        // helper that specializes on T
        typedef AVX::VectorHelper<T> HT;

        // cast any m256/m128 to VectorType
        template <typename V> static Vc_INTRINSIC VectorType _cast(V v)
        {
            return AVX::avx_cast<VectorType>(v);
        }

        typedef Common::VectorMemoryUnion<VectorType, EntryType> StorageType;
        StorageType d;

        using WidthT = Common::WidthT<VectorType>;
        // ICC can't compile this:
        // static constexpr WidthT Width = WidthT();

    public:
#include "../common/generalinterface.h"

        static Vc_ALWAYS_INLINE_L Vector Random() Vc_ALWAYS_INLINE_R;

        ///////////////////////////////////////////////////////////////////////////////////////////
        // internal: required to enable returning objects of VectorType
        Vc_ALWAYS_INLINE Vector(VectorTypeArg x) : d(x) {}

        // implict conversion from compatible Vector<U, abi>
        template <typename U>
        Vc_INTRINSIC Vector(
            V<U> x, typename std::enable_if<Traits::is_implicit_cast_allowed<U, T>::value,
                                            void *>::type = nullptr)
            : d(AVX::convert<U, T>(x.data()))
        {
        }

#if Vc_IS_VERSION_1
        // static_cast from the remaining Vector<U, abi>
        template <typename U>
        Vc_DEPRECATED("use simd_cast instead of explicit type casting to convert between "
                      "vector types") Vc_INTRINSIC explicit Vector(
            V<U> x,
            typename std::enable_if<!Traits::is_implicit_cast_allowed<U, T>::value,
                                    void *>::type = nullptr)
            : d(Detail::zeroExtendIfNeeded(AVX::convert<U, T>(x.data())))
        {
        }

        // static_cast from other types, implemented via the non-member simd_cast function in
        // simd_cast_caller.tcc
        template <typename U,
                  typename = enable_if<Traits::is_simd_vector<U>::value &&
                                       !std::is_same<Vector, Traits::decay<U>>::value>>
        Vc_DEPRECATED("use simd_cast instead of explicit type casting to convert between "
                      "vector types") Vc_INTRINSIC_L
            explicit Vector(U &&x) Vc_INTRINSIC_R;
#endif

        Vc_INTRINSIC explicit Vector(reference a) : Vector(static_cast<EntryType>(a)) {}

        ///////////////////////////////////////////////////////////////////////////////////////////
        // broadcast
        Vc_INTRINSIC Vector(EntryType a) : d(Detail::avx_broadcast(a)) {}
        template <typename U>
        Vc_INTRINSIC Vector(U a,
                            typename std::enable_if<std::is_same<U, int>::value &&
                                                        !std::is_same<U, EntryType>::value,
                                                    void *>::type = nullptr)
            : Vector(static_cast<EntryType>(a))
        {
        }

        //template<typename U>
        explicit Vector(std::initializer_list<EntryType>)
        {
            static_assert(std::is_same<EntryType, void>::value,
                          "A SIMD vector object cannot be initialized from an initializer list "
                          "because the number of entries in the vector is target-dependent.");
        }

#include "../common/loadinterface.h"
#include "../common/storeinterface.h"

        ///////////////////////////////////////////////////////////////////////////////////////////
        // zeroing
        Vc_INTRINSIC_L void setZero() Vc_INTRINSIC_R;
        Vc_INTRINSIC_L void setZero(const Mask &k) Vc_INTRINSIC_R;
        Vc_INTRINSIC_L void setZeroInverted(const Mask &k) Vc_INTRINSIC_R;

        Vc_INTRINSIC_L void setQnan() Vc_INTRINSIC_R;
        Vc_INTRINSIC_L void setQnan(MaskArgument k) Vc_INTRINSIC_R;

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
            d.v() = AVX::gather<sizeof(T) * Scale>(
                args.address,
                simd_cast<conditional_t<Size == 4, SSE::int_v, AVX2::int_v>>(args.indexes)
                    .data());
        }

        // masked overload
        template <class U, class A, int Scale, int N = Vector<U, A>::size(),
                  class = enable_if<(Vector<U, A>::size() >= size() && sizeof(T) >= 4)>>
        Vc_INTRINSIC void gatherImplementation(
            const Common::GatherArguments<T, Vector<U, A>, Scale> &args, MaskArgument k)
        {
            d.v() = AVX::gather<sizeof(T) * Scale>(
                d.v(), k.data(), args.address,
                simd_cast<conditional_t<Size == 4, SSE::int_v, AVX2::int_v>>(args.indexes)
                    .data());
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
            const auto idx0 = simd_cast<int_v, 0>(args.indexes).data();
            const auto idx1 = simd_cast<int_v, 1>(args.indexes).data();
            *this = simd_cast<Vector>(int_v(AVX::gather<sizeof(MT) * Scale>(
                                          aliasing_cast<int>(args.address), idx0)),
                                      int_v(AVX::gather<sizeof(MT) * Scale>(
                                          aliasing_cast<int>(args.address), idx1)));
            if (sizeof(MT) == 1) {
                if (std::is_signed<MT>::value) {
                    using Signed = AVX2::Vector<typename std::make_signed<T>::type>;
                    *this = (simd_cast<Signed>(*this) << 8) >> 8;  // sign extend
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
            const auto idx0 = simd_cast<int_v, 0>(args.indexes).data();
            const auto idx1 = simd_cast<int_v, 1>(args.indexes).data();
            const auto k0 = simd_cast<AVX2::int_m, 0>(k).data();
            const auto k1 = simd_cast<AVX2::int_m, 1>(k).data();
            auto v = simd_cast<Vector>(
                int_v(AVX::gather<sizeof(MT) * Scale>(
                    _mm256_setzero_si256(), k0, aliasing_cast<int>(args.address), idx0)),
                int_v(AVX::gather<sizeof(MT) * Scale>(
                    _mm256_setzero_si256(), k1, aliasing_cast<int>(args.address), idx1)));
            if (sizeof(MT) == 1) {
                if (std::is_signed<MT>::value) {
                    using Signed = AVX2::Vector<typename std::make_signed<T>::type>;
                    v = (simd_cast<Signed>(v) << 8) >> 8;  // sign extend
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

        ///////////////////////////////////////////////////////////////////////////////////////////
        //prefix
        Vc_ALWAYS_INLINE Vector &operator++() { data() = Detail::add(data(), Detail::one(T()), T()); return *this; }
        Vc_ALWAYS_INLINE Vector &operator--() { data() = Detail::sub(data(), Detail::one(T()), T()); return *this; }
        //postfix
        Vc_ALWAYS_INLINE Vector operator++(int) { const Vector r = *this; data() = Detail::add(data(), Detail::one(T()), T()); return r; }
        Vc_ALWAYS_INLINE Vector operator--(int) { const Vector r = *this; data() = Detail::sub(data(), Detail::one(T()), T()); return r; }

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
            return o.d.set(i, v);
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

        Vc_INTRINSIC_L Vc_PURE_L Vector operator[](Permutation::ReversedTag) const Vc_INTRINSIC_R Vc_PURE_R;
        Vc_INTRINSIC_L Vc_PURE_L Vector operator[](const IndexType &perm) const Vc_INTRINSIC_R Vc_PURE_R;

        Vc_INTRINSIC Vc_PURE Mask operator!() const
        {
            return *this == Zero();
        }
        Vc_ALWAYS_INLINE Vector operator~() const
        {
#ifndef Vc_ENABLE_FLOAT_BIT_OPERATORS
            static_assert(std::is_integral<T>::value,
                          "bit-complement can only be used with Vectors of integral type");
#endif
            return Detail::andnot_(data(), Detail::allone<VectorType>());
        }
        Vc_ALWAYS_INLINE_L Vc_PURE_L Vector operator-() const Vc_ALWAYS_INLINE_R Vc_PURE_R;
        Vc_INTRINSIC Vc_PURE Vector operator+() const { return *this; }

        // shifts
#define Vc_OP_VEC(op)                                                                    \
    Vc_INTRINSIC Vector &operator op##=(AsArg x);                                        \
    Vc_INTRINSIC Vc_PURE Vector operator op(AsArg x) const                               \
    {                                                                                    \
        static_assert(                                                                   \
            std::is_integral<T>::value,                                                  \
            "bitwise-operators can only be used with Vectors of integral type");         \
    }
    Vc_ALL_SHIFTS(Vc_OP_VEC);
#undef Vc_OP_VEC

        Vc_ALWAYS_INLINE_L Vector &operator>>=(int x) Vc_ALWAYS_INLINE_R;
        Vc_ALWAYS_INLINE_L Vector &operator<<=(int x) Vc_ALWAYS_INLINE_R;
        Vc_ALWAYS_INLINE_L Vector operator>>(int x) const Vc_ALWAYS_INLINE_R;
        Vc_ALWAYS_INLINE_L Vector operator<<(int x) const Vc_ALWAYS_INLINE_R;

        Vc_DEPRECATED("use isnegative(x) instead") Vc_INTRINSIC Vc_PURE Mask
            isNegative() const
        {
            return Vc::isnegative(*this);
        }

        Vc_ALWAYS_INLINE void assign( const Vector &v, const Mask &mask ) {
            data() = Detail::blend(data(), v.data(), mask.data());
        }

        template <typename V2>
        Vc_DEPRECATED("Use simd_cast instead of Vector::staticCast") Vc_ALWAYS_INLINE V2
            staticCast() const
        {
            return V2(*this);
        }
        template <typename V2>
        Vc_DEPRECATED("use reinterpret_components_cast instead") Vc_ALWAYS_INLINE V2
            reinterpretCast() const
        {
            return AVX::avx_cast<typename V2::VectorType>(data());
        }

        Vc_ALWAYS_INLINE WriteMaskedVector operator()(const Mask &k)
        {
            return {*this, k};
        }

        Vc_ALWAYS_INLINE VectorType &data() { return d.v(); }
        Vc_ALWAYS_INLINE const VectorType &data() const { return d.v(); }

        template<int Index>
        Vc_INTRINSIC_L Vector broadcast() const Vc_INTRINSIC_R;

        Vc_INTRINSIC_L std::pair<Vector, int> minIndex() const Vc_INTRINSIC_R;
        Vc_INTRINSIC_L std::pair<Vector, int> maxIndex() const Vc_INTRINSIC_R;

        Vc_ALWAYS_INLINE EntryType min() const { return Detail::min(data(), T()); }
        Vc_ALWAYS_INLINE EntryType max() const { return Detail::max(data(), T()); }
        Vc_ALWAYS_INLINE EntryType product() const { return Detail::mul(data(), T()); }
        Vc_ALWAYS_INLINE EntryType sum() const { return Detail::add(data(), T()); }
        Vc_ALWAYS_INLINE_L Vector partialSum() const Vc_ALWAYS_INLINE_R;
        //template<typename BinaryOperation> Vc_ALWAYS_INLINE_L Vector partialSum(BinaryOperation op) const Vc_ALWAYS_INLINE_R;
        Vc_ALWAYS_INLINE_L EntryType min(MaskArgument m) const Vc_ALWAYS_INLINE_R;
        Vc_ALWAYS_INLINE_L EntryType max(MaskArgument m) const Vc_ALWAYS_INLINE_R;
        Vc_ALWAYS_INLINE_L EntryType product(MaskArgument m) const Vc_ALWAYS_INLINE_R;
        Vc_ALWAYS_INLINE_L EntryType sum(MaskArgument m) const Vc_ALWAYS_INLINE_R;

        Vc_INTRINSIC_L Vector shifted(int amount, Vector shiftIn) const Vc_INTRINSIC_R;
        Vc_INTRINSIC_L Vector shifted(int amount) const Vc_INTRINSIC_R;
        Vc_INTRINSIC_L Vector rotated(int amount) const Vc_INTRINSIC_R;
        Vc_INTRINSIC_L Vc_PURE_L Vector reversed() const Vc_INTRINSIC_R Vc_PURE_R;
        Vc_ALWAYS_INLINE_L Vc_PURE_L Vector sorted() const Vc_ALWAYS_INLINE_R Vc_PURE_R;

        template <typename F> void callWithValuesSorted(F &&f)
        {
            EntryType value = d.m(0);
            f(value);
            for (size_t i = 1; i < Size; ++i) {
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
            for (size_t i : where(mask)) {
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
            Vc::exponent(*this);
        }

        Vc_INTRINSIC_L Vector interleaveLow(Vector x) const Vc_INTRINSIC_R;
        Vc_INTRINSIC_L Vector interleaveHigh(Vector x) const Vc_INTRINSIC_R;
};
#undef Vc_CURRENT_CLASS_NAME
template <typename T> constexpr size_t Vector<T, VectorAbi::Avx>::Size;
template <typename T> constexpr size_t Vector<T, VectorAbi::Avx>::MemoryAlignment;

#define Vc_CONDITIONAL_ASSIGN(name_, op_)                                                \
    template <Operator O, typename T, typename M, typename U>                            \
    Vc_INTRINSIC enable_if<O == Operator::name_, void> conditional_assign(               \
        AVX2::Vector<T> &lhs, M &&mask, U &&rhs)                                         \
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
    Vc_INTRINSIC enable_if<O == Operator::name_, AVX2::Vector<T>> conditional_assign(    \
        AVX2::Vector<T> &lhs, M &&mask)                                                  \
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

#endif // VC_AVX_VECTOR_H_
