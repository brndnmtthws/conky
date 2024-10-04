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

#ifndef VC_AVX_MASK_H_
#define VC_AVX_MASK_H_

#include <array>

#include "intrinsics.h"
#include "../common/storage.h"
#include "../common/bitscanintrinsics.h"
#include "../common/maskbool.h"
#include "detail.h"
#include "macros.h"

namespace Vc_VERSIONED_NAMESPACE
{
template <typename T> class Mask<T, VectorAbi::Avx>
{
public:
    using abi = VectorAbi::Avx;

    /**
     * The \c EntryType of masks is always bool, independent of \c T.
     */
    typedef bool EntryType;
    using value_type = EntryType;

    using MaskBool = Common::MaskBool<sizeof(T)>;
    /**
     * The \c VectorEntryType, in contrast to \c EntryType, reveals information about the SIMD
     * implementation. This type is useful for the \c sizeof operator in generic functions.
     */
    using VectorEntryType = MaskBool;

    /**
     * The associated Vector<T> type.
     */
    using Vector = AVX2::Vector<T>;

    ///\internal
    using VectorTypeF = AVX::FloatVectorType<typename AVX::VectorTypeHelper<T>::Type>;
    ///\internal
    using VectorTypeD = AVX::DoubleVectorType<VectorTypeF>;
    ///\internal
    using VectorTypeI = AVX::IntegerVectorType<VectorTypeF>;

private:
    typedef const VectorTypeF VArg;
    typedef const VectorTypeD VdArg;
    typedef const VectorTypeI ViArg;

public:
    static constexpr size_t Size = sizeof(VectorTypeF) / sizeof(T);
    static constexpr size_t MemoryAlignment = Size;
    static constexpr std::size_t size() { return Size; }
    Vc_FREE_STORE_OPERATORS_ALIGNED(alignof(VectorType));

private:
    typedef Common::Storage<T, Size> Storage;

public:
    /**
     * The \c VectorType reveals the implementation-specific internal type used for the
     * SIMD type.
     */
    using VectorType = typename Storage::VectorType;

    using EntryReference = Vc::Detail::ElementReference<Mask>;
    using reference = EntryReference;

        // abstracts the way Masks are passed to functions, it can easily be changed to const ref here
#if defined Vc_MSVC && defined _WIN32
        typedef const Mask &AsArg;
#else
        typedef const Mask AsArg;
#endif

        Vc_INTRINSIC Mask() {}
        Vc_INTRINSIC Mask(VArg  x) : d(AVX::avx_cast<VectorType>(x)) {}
        Vc_INTRINSIC Mask(VdArg x) : d(AVX::avx_cast<VectorType>(x)) {}
        Vc_INTRINSIC Mask(ViArg x) : d(AVX::avx_cast<VectorType>(x)) {}
        Vc_INTRINSIC explicit Mask(VectorSpecialInitializerZero) : d(Detail::zero<VectorType>()) {}
        Vc_INTRINSIC explicit Mask(VectorSpecialInitializerOne) : d(Detail::allone<VectorType>()) {}
        Vc_INTRINSIC explicit Mask(bool b)
            : d(b ? Detail::allone<VectorType>() : Detail::zero<VectorType>())
        {
        }
        Vc_INTRINSIC static Mask Zero() { return Mask{Vc::Zero}; }
        Vc_INTRINSIC static Mask One() { return Mask{Vc::One}; }

        // implicit cast
        template <typename U>
        Vc_INTRINSIC Mask(
            U &&rhs, Common::enable_if_mask_converts_implicitly<Mask, T, U> = nullarg)
            : d(AVX::avx_cast<VectorType>(
                  Detail::mask_cast<Traits::decay<U>::Size, Size, VectorTypeF>(
                      rhs.dataI())))
        {
        }

#if Vc_IS_VERSION_1
        // explicit cast, implemented via simd_cast (in avx/simd_cast_caller.h)
        template <typename U>
        Vc_DEPRECATED("use simd_cast instead of explicit type casting to convert between "
                      "mask types") Vc_INTRINSIC
            explicit Mask(U &&rhs,
                          Common::enable_if_mask_converts_explicitly<T, U> = nullarg);
#endif

        template<typename Flags = DefaultLoadTag> Vc_INTRINSIC explicit Mask(const bool *mem, Flags f = Flags()) { load(mem, f); }

        template<typename Flags = DefaultLoadTag> Vc_INTRINSIC void load(const bool *mem, Flags = Flags());

        template<typename Flags = DefaultLoadTag> Vc_INTRINSIC void store(bool *mem, Flags = Flags()) const;

        Vc_INTRINSIC Mask &operator=(const Mask &) = default;
        Vc_INTRINSIC_L Mask &operator=(const std::array<bool, Size> &values) Vc_INTRINSIC_R;
        Vc_INTRINSIC_L operator std::array<bool, Size>() const Vc_INTRINSIC_R;

        // specializations in mask.tcc
        Vc_INTRINSIC Vc_PURE bool operator==(const Mask &rhs) const
        { return Detail::movemask(d.v()) == Detail::movemask(rhs.d.v()); }

        Vc_INTRINSIC Vc_PURE bool operator!=(const Mask &rhs) const
        { return !operator==(rhs); }

        Vc_INTRINSIC Mask operator!() const
        {
#ifdef Vc_GCC
            return ~dataI();
#else
            return Detail::andnot_(dataF(), Detail::allone<VectorTypeF>());
#endif
        }

        Vc_INTRINSIC Mask &operator&=(const Mask &rhs) { d.v() = AVX::avx_cast<VectorType>(Detail::and_(data(), rhs.data())); return *this; }
        Vc_INTRINSIC Mask &operator|=(const Mask &rhs) { d.v() = AVX::avx_cast<VectorType>(Detail::or_ (data(), rhs.data())); return *this; }
        Vc_INTRINSIC Mask &operator^=(const Mask &rhs) { d.v() = AVX::avx_cast<VectorType>(Detail::xor_(data(), rhs.data())); return *this; }

        Vc_INTRINSIC Vc_PURE Mask operator&(const Mask &rhs) const { return Detail::and_(data(), rhs.data()); }
        Vc_INTRINSIC Vc_PURE Mask operator|(const Mask &rhs) const { return Detail::or_(data(), rhs.data()); }
        Vc_INTRINSIC Vc_PURE Mask operator^(const Mask &rhs) const { return Detail::xor_(data(), rhs.data()); }

        Vc_INTRINSIC Vc_PURE Mask operator&&(const Mask &rhs) const { return Detail::and_(data(), rhs.data()); }
        Vc_INTRINSIC Vc_PURE Mask operator||(const Mask &rhs) const { return Detail::or_(data(), rhs.data()); }

        // no need for expression template optimizations because cmp(n)eq for floats are not bitwise
        // compares
        Vc_INTRINSIC_L bool isNotEmpty() const Vc_INTRINSIC_R;
        Vc_INTRINSIC_L bool isEmpty() const Vc_INTRINSIC_R;
        Vc_INTRINSIC_L bool isFull() const Vc_INTRINSIC_R;
        Vc_INTRINSIC_L bool isMix() const Vc_INTRINSIC_R;

        Vc_INTRINSIC Vc_PURE int shiftMask() const { return Detail::movemask(dataI()); }
        Vc_INTRINSIC Vc_PURE int toInt() const { return Detail::mask_to_int<Size>(dataI()); }

        Vc_INTRINSIC VectorType  data () const { return d.v(); }
        Vc_INTRINSIC VectorTypeF dataF() const { return AVX::avx_cast<VectorTypeF>(d.v()); }
        Vc_INTRINSIC VectorTypeI dataI() const { return AVX::avx_cast<VectorTypeI>(d.v()); }
        Vc_INTRINSIC VectorTypeD dataD() const { return AVX::avx_cast<VectorTypeD>(d.v()); }

private:
    friend reference;
    static Vc_INTRINSIC Vc_PURE value_type get(const Mask &m, int i) noexcept
    {
        return m.toInt() & (1 << i);
    }
    template <typename U>
    static Vc_INTRINSIC void set(Mask &m, int i,
                                 U &&v) noexcept(noexcept(MaskBool(std::declval<U>())))
    {
        m.d.set(i, MaskBool(std::forward<U>(v)));
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
        return {*this, int(index)};
    }
    Vc_ALWAYS_INLINE Vc_PURE value_type operator[](size_t index) const noexcept
    {
        return get(*this, index);
    }

        Vc_INTRINSIC Vc_PURE int count() const { return Detail::popcnt16(toInt()); }
        Vc_INTRINSIC Vc_PURE int firstOne() const { return _bit_scan_forward(toInt()); }

        template <typename G> static Vc_INTRINSIC_L Mask generate(G &&gen) Vc_INTRINSIC_R;
        Vc_INTRINSIC_L Vc_PURE_L Mask shifted(int amount) const Vc_INTRINSIC_R Vc_PURE_R;

    private:
#ifdef Vc_COMPILE_BENCHMARKS
    public:
#endif
        Storage d;
};
template <typename T> constexpr size_t Mask<T, VectorAbi::Avx>::Size;
template <typename T> constexpr size_t Mask<T, VectorAbi::Avx>::MemoryAlignment;

}  // namespace Vc

#include "mask.tcc"

#endif // VC_AVX_MASK_H_
