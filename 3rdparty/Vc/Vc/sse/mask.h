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

#ifndef VC_SSE_MASK_H_
#define VC_SSE_MASK_H_

#include "intrinsics.h"
#include "../common/maskbool.h"
#include "detail.h"
#include "macros.h"

namespace Vc_VERSIONED_NAMESPACE
{
namespace Detail
{
template <size_t Size>
Vc_INTRINSIC_L Vc_CONST_L int mask_count(__m128i) Vc_INTRINSIC_R Vc_CONST_R;
template <size_t Size>
Vc_INTRINSIC_L Vc_CONST_L int mask_to_int(__m128i) Vc_INTRINSIC_R Vc_CONST_R;
template <size_t Size>
Vc_INTRINSIC_L Vc_CONST_L bool is_equal(__m128, __m128) Vc_INTRINSIC_R Vc_CONST_R;
template <size_t Size>
Vc_INTRINSIC_L Vc_CONST_L bool is_not_equal(__m128, __m128) Vc_INTRINSIC_R Vc_CONST_R;
}  // namespace Detail

using SSE::sse_cast;

template <typename T> class Mask<T, VectorAbi::Sse>
{
    using abi = VectorAbi::Sse;
    friend class Mask<  double, abi>;
    friend class Mask<   float, abi>;
    friend class Mask< int32_t, abi>;
    friend class Mask<uint32_t, abi>;
    friend class Mask< int16_t, abi>;
    friend class Mask<uint16_t, abi>;

    /**
     * A helper type for aliasing the entries in the mask but behaving like a bool.
     */
    typedef Common::MaskBool<sizeof(T)> MaskBool;

    typedef Common::Storage<T, SSE::VectorTraits<T>::Size> Storage;

public:

    /**
     * The \c EntryType of masks is always bool, independent of \c T.
     */
    typedef bool EntryType;
    using value_type = EntryType;

    /**
     * The return type of the non-const subscript operator.
     */
    using EntryReference = Detail::ElementReference<Mask>;
    using reference = EntryReference;

    /**
     * The \c VectorEntryType, in contrast to \c EntryType, reveals information about the SIMD
     * implementation. This type is useful for the \c sizeof operator in generic functions.
     */
    typedef MaskBool VectorEntryType;

    /**
     * The \c VectorType reveals the implementation-specific internal type used for the SIMD type.
     */
    using VectorType = typename Storage::VectorType;

    /**
     * The associated Vector<T> type.
     */
    using Vector = SSE::Vector<T>;

public:
    Vc_FREE_STORE_OPERATORS_ALIGNED(16);
    static constexpr size_t Size = SSE::VectorTraits<T>::Size;
    static constexpr size_t MemoryAlignment = Size;
    static constexpr std::size_t size() { return Size; }

        // abstracts the way Masks are passed to functions, it can easily be changed to const ref here
#if defined Vc_MSVC && defined _WIN32
        typedef const Mask &Argument;
#else
        typedef Mask Argument;
#endif

        Vc_INTRINSIC Mask() = default;
        Vc_INTRINSIC Mask(const Mask &) = default;
        Vc_INTRINSIC Mask &operator=(const Mask &) = default;

        Vc_INTRINSIC Mask(const __m128  &x) : d(sse_cast<VectorType>(x)) {}
        Vc_INTRINSIC Mask(const __m128d &x) : d(sse_cast<VectorType>(x)) {}
        Vc_INTRINSIC Mask(const __m128i &x) : d(sse_cast<VectorType>(x)) {}
        Vc_INTRINSIC explicit Mask(VectorSpecialInitializerZero) : Mask(_mm_setzero_ps()) {}
        Vc_INTRINSIC explicit Mask(VectorSpecialInitializerOne) : Mask(SSE::_mm_setallone_ps()) {}
        Vc_INTRINSIC explicit Mask(bool b) : Mask(b ? SSE::_mm_setallone_ps() : _mm_setzero_ps()) {}
        Vc_INTRINSIC static Mask Zero() { return Mask{Vc::Zero}; }
        Vc_INTRINSIC static Mask One() { return Mask{Vc::One}; }

        // implicit cast
        template <typename U>
        Vc_INTRINSIC Mask(
            U &&rhs, Common::enable_if_mask_converts_implicitly<Mask, T, U> = nullarg)
            : d(sse_cast<VectorType>(
                  Detail::mask_cast<Traits::simd_vector_size<U>::value, Size, __m128>(
                      rhs.dataI())))
        {
        }

#if Vc_IS_VERSION_1
        // explicit cast, implemented via simd_cast (implementation in sse/simd_cast.h)
        template <typename U>
        Vc_DEPRECATED("use simd_cast instead of explicit type casting to convert between "
                      "mask types") Vc_INTRINSIC
            explicit Mask(U &&rhs,
                          Common::enable_if_mask_converts_explicitly<T, U> = nullarg);
#endif

        Vc_ALWAYS_INLINE explicit Mask(const bool *mem) { load(mem); }
        template<typename Flags> Vc_ALWAYS_INLINE explicit Mask(const bool *mem, Flags f) { load(mem, f); }

        Vc_ALWAYS_INLINE_L void load(const bool *mem) Vc_ALWAYS_INLINE_R;
        template<typename Flags> Vc_ALWAYS_INLINE void load(const bool *mem, Flags) { load(mem); }

        Vc_ALWAYS_INLINE_L void store(bool *) const Vc_ALWAYS_INLINE_R;
        template<typename Flags> Vc_ALWAYS_INLINE void store(bool *mem, Flags) const { store(mem); }

        Vc_ALWAYS_INLINE Vc_PURE bool operator==(const Mask &rhs) const
        {
            return Detail::is_equal<Size>(dataF(), rhs.dataF());
        }
        Vc_ALWAYS_INLINE Vc_PURE bool operator!=(const Mask &rhs) const
        {
            return Detail::is_not_equal<Size>(dataF(), rhs.dataF());
        }

        Vc_ALWAYS_INLINE Vc_PURE Mask operator!() const
        {
#ifdef Vc_GCC
            return ~dataI();
#else
            return _mm_andnot_si128(dataI(), SSE::_mm_setallone_si128());
#endif
        }

        Vc_ALWAYS_INLINE Mask &operator&=(const Mask &rhs) { d.v() = SSE::sse_cast<VectorType>(_mm_and_ps(dataF(), rhs.dataF())); return *this; }
        Vc_ALWAYS_INLINE Mask &operator|=(const Mask &rhs) { d.v() = SSE::sse_cast<VectorType>(_mm_or_ps (dataF(), rhs.dataF())); return *this; }
        Vc_ALWAYS_INLINE Mask &operator^=(const Mask &rhs) { d.v() = SSE::sse_cast<VectorType>(_mm_xor_ps(dataF(), rhs.dataF())); return *this; }

        Vc_ALWAYS_INLINE Vc_PURE Mask operator&(const Mask &rhs) const { return _mm_and_ps(dataF(), rhs.dataF()); }
        Vc_ALWAYS_INLINE Vc_PURE Mask operator|(const Mask &rhs) const { return _mm_or_ps (dataF(), rhs.dataF()); }
        Vc_ALWAYS_INLINE Vc_PURE Mask operator^(const Mask &rhs) const { return _mm_xor_ps(dataF(), rhs.dataF()); }

        Vc_ALWAYS_INLINE Vc_PURE Mask operator&&(const Mask &rhs) const { return _mm_and_ps(dataF(), rhs.dataF()); }
        Vc_ALWAYS_INLINE Vc_PURE Mask operator||(const Mask &rhs) const { return _mm_or_ps (dataF(), rhs.dataF()); }

        Vc_ALWAYS_INLINE Vc_PURE bool isFull () const { return
#ifdef Vc_USE_PTEST
            _mm_testc_si128(dataI(), SSE::_mm_setallone_si128()); // return 1 if (0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff) == (~0 & d.v())
#else
            _mm_movemask_epi8(dataI()) == 0xffff;
#endif
        }
        Vc_ALWAYS_INLINE Vc_PURE bool isNotEmpty() const { return
#ifdef Vc_USE_PTEST
            0 == _mm_testz_si128(dataI(), dataI()); // return 1 if (0, 0, 0, 0) == (d.v() & d.v())
#else
            _mm_movemask_epi8(dataI()) != 0x0000;
#endif
        }
        Vc_ALWAYS_INLINE Vc_PURE bool isEmpty() const { return
#ifdef Vc_USE_PTEST
            0 != _mm_testz_si128(dataI(), dataI()); // return 1 if (0, 0, 0, 0) == (d.v() & d.v())
#else
            _mm_movemask_epi8(dataI()) == 0x0000;
#endif
        }
        Vc_ALWAYS_INLINE Vc_PURE bool isMix() const {
#ifdef Vc_USE_PTEST
            return _mm_test_mix_ones_zeros(dataI(), SSE::_mm_setallone_si128());
#else
            const int tmp = _mm_movemask_epi8(dataI());
            return tmp != 0 && (tmp ^ 0xffff) != 0;
#endif
        }

        Vc_ALWAYS_INLINE Vc_PURE int shiftMask() const { return _mm_movemask_epi8(dataI()); }

        Vc_ALWAYS_INLINE Vc_PURE int toInt() const { return Detail::mask_to_int<Size>(dataI()); }

        Vc_ALWAYS_INLINE Vc_PURE VectorType  data() const { return d.v(); }
        Vc_ALWAYS_INLINE Vc_PURE __m128  dataF() const { return SSE::sse_cast<__m128 >(d.v()); }
        Vc_ALWAYS_INLINE Vc_PURE __m128i dataI() const { return SSE::sse_cast<__m128i>(d.v()); }
        Vc_ALWAYS_INLINE Vc_PURE __m128d dataD() const { return SSE::sse_cast<__m128d>(d.v()); }

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

        Vc_ALWAYS_INLINE Vc_PURE int count() const
        {
            return Detail::mask_count<Size>(dataI());
        }

        /**
         * Returns the index of the first one in the mask.
         *
         * The return value is undefined if the mask is empty.
         */
        Vc_ALWAYS_INLINE_L Vc_PURE_L int firstOne() const Vc_ALWAYS_INLINE_R Vc_PURE_R;

        template <typename G> static Vc_INTRINSIC_L Mask generate(G &&gen) Vc_INTRINSIC_R;
        Vc_INTRINSIC_L Vc_PURE_L Mask shifted(int amount) const Vc_INTRINSIC_R Vc_PURE_R;

    private:
#ifdef Vc_COMPILE_BENCHMARKS
    public:
#endif
        Storage d;
};
template <typename T> constexpr size_t Mask<T, VectorAbi::Sse>::Size;
template <typename T> constexpr size_t Mask<T, VectorAbi::Sse>::MemoryAlignment;

}  // namespace Vc

#include "mask.tcc"

#endif // VC_SSE_MASK_H_
