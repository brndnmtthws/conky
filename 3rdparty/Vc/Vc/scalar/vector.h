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

#ifndef VC_SCALAR_VECTOR_H_
#define VC_SCALAR_VECTOR_H_

#include <assert.h>
#include <algorithm>
#include <cmath>

#ifdef _MSC_VER
#include <float.h>
#endif

#include "types.h"
#include "detail.h"
#include "mask.h"

#include "macros.h"

namespace Vc_VERSIONED_NAMESPACE
{
#define Vc_CURRENT_CLASS_NAME Vector
template <typename T> class Vector<T, VectorAbi::Scalar>
{
    static_assert(std::is_arithmetic<T>::value,
                  "Vector<T> only accepts arithmetic builtin types as template parameter T.");

    public:
        using abi = VectorAbi::Scalar;
        using EntryType = T;
        using VectorEntryType = EntryType;
        using value_type = EntryType;
        using VectorType = EntryType;
        using vector_type = VectorType;
        using reference = Detail::ElementReference<Vector>;

    protected:
        VectorType m_data = VectorType();
        template <typename U> using V = Vector<U, abi>;

    public:
        typedef Scalar::Mask<T> Mask;
        using MaskType = Mask;
        using mask_type = Mask;
        typedef Mask MaskArgument;
        typedef Vector AsArg;

        Vc_ALWAYS_INLINE VectorType &data() { return m_data; }
        Vc_ALWAYS_INLINE const VectorType &data() const { return m_data; }

        static constexpr size_t Size = 1;
        static constexpr size_t MemoryAlignment = alignof(VectorType);
        using IndexType = fixed_size_simd<int, 1>;
        using index_type = IndexType;

#include "../common/generalinterface.h"

        static Vc_INTRINSIC_L Vector Random() Vc_INTRINSIC_R;

        // implict conversion from compatible Vector<U, abi>
        template <typename U>
        Vc_INTRINSIC Vector(
            V<U> x, typename std::enable_if<Traits::is_implicit_cast_allowed<U, T>::value,
                                            void *>::type = nullptr)
            : m_data(static_cast<EntryType>(x.data()))
        {
        }

#if Vc_IS_VERSION_1
        // static_cast from the remaining Vector<U, abi>
        template <typename U>
        Vc_DEPRECATED("use simd_cast instead of explicit type casting to convert between "
                      "vector types") Vc_INTRINSIC
            explicit Vector(
                V<U> x,
                typename std::enable_if<!Traits::is_implicit_cast_allowed<U, T>::value,
                                        void *>::type = nullptr)
            : m_data(static_cast<EntryType>(x.data()))
        {
        }
#endif

        Vc_INTRINSIC explicit Vector(reference a) : Vector(static_cast<EntryType>(a)) {}

        ///////////////////////////////////////////////////////////////////////////////////////////
        // broadcast
        Vc_INTRINSIC Vector(EntryType a) : m_data(a) {}
        template <typename U>
        Vc_INTRINSIC Vector(U a,
                            typename std::enable_if<std::is_same<U, int>::value &&
                                                        !std::is_same<U, EntryType>::value,
                                                    void *>::type = nullptr)
            : Vector(static_cast<EntryType>(a))
        {
        }

#include "../common/loadinterface.h"
#include "../common/storeinterface.h"

        ///////////////////////////////////////////////////////////////////////////////////////////
        // zeroing
        Vc_ALWAYS_INLINE void setZero() { m_data = 0; }
        Vc_ALWAYS_INLINE void setZero(Mask k) { if (k.data()) m_data = 0; }
        Vc_ALWAYS_INLINE void setZeroInverted(Mask k) { if (!k.data()) m_data = 0; }

        Vc_INTRINSIC_L void setQnan() Vc_INTRINSIC_R;
        Vc_INTRINSIC_L void setQnan(Mask m) Vc_INTRINSIC_R;

#include "../common/gatherinterface.h"
#include "../common/scatterinterface.h"

        //prefix
        Vc_ALWAYS_INLINE Vector &operator++() { ++m_data; return *this; }
        Vc_ALWAYS_INLINE Vector &operator--() { --m_data; return *this; }
        //postfix
        Vc_ALWAYS_INLINE Vector operator++(int) { return m_data++; }
        Vc_ALWAYS_INLINE Vector operator--(int) { return m_data--; }

    private:
        friend reference;
        Vc_INTRINSIC static value_type get(const Vector &o, int i) noexcept
        {
            Vc_ASSERT(i == 0); if (i) {}
            return o.m_data;
        }
        template <typename U>
        Vc_INTRINSIC static void set(Vector &o, int i, U &&v) noexcept(
            noexcept(std::declval<value_type &>() = v))
        {
            Vc_ASSERT(i == 0); if (i) {}
            o.m_data = v;
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
            Vc_ASSERT(index == 0); if (index) {}
            return m_data;
        }

        Vc_ALWAYS_INLINE Mask operator!() const
        {
            return Mask(!m_data);
        }
        Vc_ALWAYS_INLINE Vector operator~() const
        {
#ifndef Vc_ENABLE_FLOAT_BIT_OPERATORS
            static_assert(std::is_integral<T>::value, "bit-complement can only be used with Vectors of integral type");
#endif
            return Vector(~m_data);
        }

        Vc_ALWAYS_INLINE Vector operator-() const
        {
            return -m_data;
        }
        Vc_INTRINSIC Vector Vc_PURE operator+() const { return *this; }

#define Vc_OP(symbol) \
        Vc_ALWAYS_INLINE Vc_PURE Vector operator symbol(const Vector &x) const { return Vector(m_data symbol x.m_data); }
        Vc_ALL_SHIFTS(Vc_OP);
#undef Vc_OP

        Vc_DEPRECATED("use isnegative(x) instead") Vc_INTRINSIC Vc_PURE Mask
            isNegative() const
        {
            return Vc::isnegative(*this);
        }

        Vc_ALWAYS_INLINE void assign(const Vector &v, const Mask &m) {
          if (m.data()) m_data = v.m_data;
        }

        template <typename V2>
        Vc_DEPRECATED("Use simd_cast instead of Vector::staticCast") Vc_ALWAYS_INLINE V2
            staticCast() const
        {
            return V2(static_cast<typename V2::EntryType>(m_data));
        }
//        template <typename V2>
//        Vc_DEPRECATED("use reinterpret_components_cast instead") Vc_ALWAYS_INLINE V2
//            reinterpretCast() const
//        {
//            typedef typename V2::EntryType AliasT2 Vc_MAY_ALIAS;
//            return V2(*reinterpret_cast<const AliasT2 *>(&m_data));
//        }

        Vc_ALWAYS_INLINE Common::WriteMaskedVector<Vector, Mask> operator()(Mask m)
        {
            return {*this, m};
        }

        Vc_ALWAYS_INLINE EntryType min() const { return m_data; }
        Vc_ALWAYS_INLINE EntryType max() const { return m_data; }
        Vc_ALWAYS_INLINE EntryType product() const { return m_data; }
        Vc_ALWAYS_INLINE EntryType sum() const { return m_data; }
        Vc_ALWAYS_INLINE Vector partialSum() const { return *this; }
        Vc_ALWAYS_INLINE EntryType min(Mask) const { return m_data; }
        Vc_ALWAYS_INLINE EntryType max(Mask) const { return m_data; }
        Vc_ALWAYS_INLINE EntryType product(Mask m) const
        {
            if (m.data()) {
                return m_data;
            } else {
                return EntryType(1);
            }
        }
        Vc_ALWAYS_INLINE EntryType sum(Mask m) const { if (m.data()) return m_data; return static_cast<EntryType>(0); }

        Vc_INTRINSIC Vector Vc_VDECL shifted(int amount, Vector shiftIn) const {
            Vc_ASSERT(amount >= -1 && amount <= 1);
            return amount == 0 ? *this : shiftIn;
        }
        Vc_INTRINSIC Vector shifted(int amount) const { return amount == 0 ? *this : Zero(); }
        Vc_INTRINSIC Vector rotated(int) const { return *this; }
        Vc_INTRINSIC Vector reversed() const { return *this; }
        Vc_INTRINSIC Vector sorted() const { return *this; }

        template <typename F> void callWithValuesSorted(F &&f) { f(m_data); }

        template <typename F> Vc_INTRINSIC void call(F &&f) const { f(m_data); }

        template <typename F> Vc_INTRINSIC void call(F &&f, Mask mask) const
        {
            if (mask.data()) {
                f(m_data);
            }
        }

        template <typename F> Vc_INTRINSIC Vector apply(F &&f) const { return Vector(f(m_data)); }

        template <typename F> Vc_INTRINSIC Vector apply(F &&f, Mask mask) const
        {
            if (mask.data()) {
                return Vector(f(m_data));
            } else {
                return *this;
            }
        }

        template<typename IndexT> Vc_INTRINSIC void fill(EntryType (&f)(IndexT)) {
            m_data = f(0);
        }
        Vc_INTRINSIC void fill(EntryType (&f)()) {
            m_data = f();
        }

        template <typename G> static Vc_INTRINSIC Vector generate(G gen)
        {
            return gen(0);
        }

        Vc_DEPRECATED("use copysign(x, y) instead") Vc_INTRINSIC Vector Vc_VDECL
            copySign(Vector x) const
        {
            return Vc::copysign(*this, x);
        }

        Vc_DEPRECATED("use exponent(x) instead") Vc_INTRINSIC Vector exponent() const
        {
            return Vc::exponent(*this);
        }

        Vc_INTRINSIC Vector Vc_VDECL interleaveLow(Vector) const { return *this; }
        Vc_INTRINSIC Vector Vc_VDECL interleaveHigh(Vector x) const { return x; }
};
#undef Vc_CURRENT_CLASS_NAME
template <typename T> constexpr size_t Vector<T, VectorAbi::Scalar>::Size;
template <typename T> constexpr size_t Vector<T, VectorAbi::Scalar>::MemoryAlignment;

#define Vc_OP(symbol)                                                                    \
    template <typename T, typename U,                                                    \
              typename = decltype(std::declval<T &>() symbol## = std::declval<T>())>     \
    Vc_INTRINSIC enable_if<std::is_convertible<U, Vector<T, VectorAbi::Scalar>>::value,  \
                           Vector<T, VectorAbi::Scalar>>                                 \
        &operator symbol##=(Vector<T, VectorAbi::Scalar> &lhs, U &&rhs)                  \
    {                                                                                    \
        lhs.data() symbol## = Vector<T, VectorAbi::Scalar>(std::forward<U>(rhs)).data(); \
        return lhs;                                                                      \
    }
Vc_ALL_SHIFTS(Vc_OP);
#undef Vc_OP

#define Vc_CONDITIONAL_ASSIGN(name_, op_)                                                \
    template <Operator O, typename T, typename M, typename U>                            \
    Vc_INTRINSIC enable_if<O == Operator::name_, void> conditional_assign(               \
        Vector<T, VectorAbi::Scalar> &lhs, M &&mask, U &&rhs)                            \
    {                                                                                    \
        if (mask.isFull()) {                                                             \
            lhs op_ std::forward<U>(rhs);                                                \
        }                                                                                \
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
    Vc_INTRINSIC enable_if<O == Operator::name_, Vector<T, VectorAbi::Scalar>>           \
    conditional_assign(Vector<T, VectorAbi::Scalar> &lhs, M &&mask)                      \
    {                                                                                    \
        return mask.isFull() ? (expr_) : lhs;                                            \
    }                                                                                    \
    Vc_NOTHING_EXPECTING_SEMICOLON
Vc_CONDITIONAL_ASSIGN(PostIncrement, lhs++);
Vc_CONDITIONAL_ASSIGN( PreIncrement, ++lhs);
Vc_CONDITIONAL_ASSIGN(PostDecrement, lhs--);
Vc_CONDITIONAL_ASSIGN( PreDecrement, --lhs);
#undef Vc_CONDITIONAL_ASSIGN

}  // namespace Vc

#include "vector.tcc"
#include "simd_cast.h"

#endif // VC_SCALAR_VECTOR_H_
