/*  This file is part of the Vc library. {{{
Copyright © 2013-2015 Matthias Kretz <kretz@kde.org>

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

#ifndef VC_COMMON_SIMDMASKARRAY_H_
#define VC_COMMON_SIMDMASKARRAY_H_

#include <type_traits>
#include <array>
#include "simdarrayhelper.h"
#include "utility.h"
#include "maskbool.h"

#include "macros.h"

namespace Vc_VERSIONED_NAMESPACE
{
/// \addtogroup SimdArray
/// @{
// atomic SimdMaskArray {{{1
/**\internal
 * Specialization of `SimdMaskArray<T, N, VectorType, VectorSize>` for the case where `N
 * == VectorSize`.
 *
 * This is specialized for implementation purposes: Since the general implementation uses
 * two SimdMaskArray data members it recurses over different SimdMaskArray instantiations.
 * The recursion is ended by this specialization, which has a single \p storage_type data
 * member to which all functions are forwarded more or less directly.
 */
template <typename T, std::size_t N, typename VectorType_>
class SimdMaskArray<T, N, VectorType_, N>
{
public:
    using VectorType = VectorType_;
    using vector_type = VectorType;
    using mask_type = typename vector_type::Mask;
    using storage_type = mask_type;

    friend storage_type &internal_data(SimdMaskArray &m) { return m.data; }
    friend const storage_type &internal_data(const SimdMaskArray &m) { return m.data; }

    static constexpr std::size_t size() { return N; }
    static constexpr std::size_t Size = size();
    static constexpr std::size_t MemoryAlignment = storage_type::MemoryAlignment;
    static_assert(Size == vector_type::Size, "size mismatch");

    using vectorentry_type = typename mask_type::VectorEntryType;
    using value_type = typename mask_type::EntryType;
    using Mask = mask_type;
    using VectorEntryType = vectorentry_type;
    using EntryType = value_type;
    using EntryReference = Vc::Detail::ElementReference<storage_type, SimdMaskArray>;
    using reference = EntryReference;
    using Vector = fixed_size_simd<T, N>;

    Vc_FREE_STORE_OPERATORS_ALIGNED(alignof(mask_type));

    // zero init
    SimdMaskArray() = default;

    // default copy ctor/operator
    SimdMaskArray(const SimdMaskArray &) = default;
    SimdMaskArray(SimdMaskArray &&) = default;
    SimdMaskArray &operator=(const SimdMaskArray &) = default;
    SimdMaskArray &operator=(SimdMaskArray &&) = default;

    // broadcasts
    Vc_INTRINSIC explicit SimdMaskArray(VectorSpecialInitializerOne one) : data(one) {}
    Vc_INTRINSIC explicit SimdMaskArray(VectorSpecialInitializerZero zero) : data(zero) {}
    Vc_INTRINSIC explicit SimdMaskArray(bool b) : data(b) {}
    Vc_INTRINSIC static SimdMaskArray Zero() { return {private_init, storage_type::Zero()}; }
    Vc_INTRINSIC static SimdMaskArray One() { return {private_init, storage_type::One()}; }

    // conversion (casts); implemented in simd_cast_caller.tcc
    template <class U, class V, class = enable_if<N == V::Size>>
    Vc_INTRINSIC_L SimdMaskArray(const SimdMaskArray<U, N, V> &x) Vc_INTRINSIC_R;
    template <class U, class V, class = enable_if<(N > V::Size && N <= 2 * V::Size)>,
              class = U>
    Vc_INTRINSIC_L SimdMaskArray(const SimdMaskArray<U, N, V> &x) Vc_INTRINSIC_R;
    template <class U, class V, class = enable_if<(N > 2 * V::Size && N <= 4 * V::Size)>,
              class = U, class = U>
    Vc_INTRINSIC_L SimdMaskArray(const SimdMaskArray<U, N, V> &x) Vc_INTRINSIC_R;

    // conversion from any Segment object (could be SimdMaskArray or Mask<T>)
    template <typename M, std::size_t Pieces, std::size_t Index>
    Vc_INTRINSIC_L SimdMaskArray(
        Common::Segment<M, Pieces, Index> &&x,
        enable_if<Traits::simd_vector_size<M>::value == Size * Pieces> = nullarg) Vc_INTRINSIC_R;

    // conversion from Mask<T>
    template <class M, class = enable_if<(Traits::is_simd_mask<M>::value &&
                                          !Traits::isSimdMaskArray<M>::value &&
                                          Traits::simd_vector_size<M>::value == Size)>>
    Vc_INTRINSIC_L SimdMaskArray(M k) Vc_INTRINSIC_R;

    // implicit conversion to Mask<U, AnyAbi> for if Mask<U, AnyAbi>::size() == N
    template <class U, class A,
              class = enable_if<Vc::Mask<U, A>::Size == N &&
                                !detail::is_fixed_size_abi<A>::value>>
    operator Vc::Mask<U, A>() const
    {
        return simd_cast<Vc::Mask<U, A>>(data);
    }
    operator fixed_size_simd_mask<T, N> &()
    {
        return static_cast<fixed_size_simd_mask<T, N> &>(*this);
    }
    operator const fixed_size_simd_mask<T, N> &() const
    {
        return static_cast<const fixed_size_simd_mask<T, N> &>(*this);
    }

    // load/store (from/to bool arrays)
    template <typename Flags = DefaultLoadTag>
    Vc_INTRINSIC explicit SimdMaskArray(const bool *mem, Flags f = Flags())
        : data(mem, f)
    {
    }

    Vc_INTRINSIC void load(const bool *mem) { data.load(mem); }
    template <typename Flags> Vc_INTRINSIC void load(const bool *mem, Flags f)
    {
        data.load(mem, f);
    }

    Vc_INTRINSIC void store(bool *mem) const { data.store(mem); }
    template <typename Flags> Vc_INTRINSIC void store(bool *mem, Flags f) const
    {
        data.store(mem, f);
    }

    // compares
    Vc_INTRINSIC Vc_PURE bool operator==(const SimdMaskArray &rhs) const
    {
        return data == rhs.data;
    }
    Vc_INTRINSIC Vc_PURE bool operator!=(const SimdMaskArray &rhs) const
    {
        return data != rhs.data;
    }

    // inversion
    Vc_INTRINSIC Vc_PURE fixed_size_simd_mask<T, N> operator!() const
    {
        return {private_init, !data};
    }

    // binary operators
    Vc_INTRINSIC SimdMaskArray &operator&=(const SimdMaskArray &rhs)
    {
        data &= rhs.data;
        return *this;
    }
    Vc_INTRINSIC SimdMaskArray &operator|=(const SimdMaskArray &rhs)
    {
        data |= rhs.data;
        return *this;
    }
    Vc_INTRINSIC SimdMaskArray &operator^=(const SimdMaskArray &rhs)
    {
        data ^= rhs.data;
        return *this;
    }

    Vc_INTRINSIC Vc_PURE fixed_size_simd_mask<T, N> operator&(
        const SimdMaskArray &rhs) const
    {
        return {private_init, data & rhs.data};
    }
    Vc_INTRINSIC Vc_PURE fixed_size_simd_mask<T, N> operator|(
        const SimdMaskArray &rhs) const
    {
        return {private_init, data | rhs.data};
    }
    Vc_INTRINSIC Vc_PURE fixed_size_simd_mask<T, N> operator^(
        const SimdMaskArray &rhs) const
    {
        return {private_init, data ^ rhs.data};
    }

    Vc_INTRINSIC Vc_PURE fixed_size_simd_mask<T, N> operator&&(
        const SimdMaskArray &rhs) const
    {
        return {private_init, data && rhs.data};
    }
    Vc_INTRINSIC Vc_PURE fixed_size_simd_mask<T, N> operator||(
        const SimdMaskArray &rhs) const
    {
        return {private_init, data || rhs.data};
    }

    Vc_INTRINSIC Vc_PURE bool isFull() const { return data.isFull(); }
    Vc_INTRINSIC Vc_PURE bool isNotEmpty() const { return data.isNotEmpty(); }
    Vc_INTRINSIC Vc_PURE bool isEmpty() const { return data.isEmpty(); }
    Vc_INTRINSIC Vc_PURE bool isMix() const { return data.isMix(); }

    Vc_INTRINSIC Vc_PURE int shiftMask() const { return data.shiftMask(); }

    Vc_INTRINSIC Vc_PURE int toInt() const { return data.toInt(); }

private:
    friend reference;
    static Vc_INTRINSIC value_type get(const storage_type &k, int i) noexcept
    {
        return k[i];
    }
    template <typename U>
    static Vc_INTRINSIC void set(storage_type &k, int i, U &&v) noexcept(
        noexcept(std::declval<storage_type &>()[0] = std::declval<U>()))
    {
        k[i] = std::forward<U>(v);
    }

public:
    /**
     * \note the returned object models the concept of a reference and
     * as such it can exist longer than the data it is referencing.
     * \note to avoid lifetime issues, we strongly advice not to store
     * any reference objects.
     */
    Vc_INTRINSIC Vc_PURE reference operator[](size_t index) noexcept
    {
        return {data, int(index)};
    }
    Vc_INTRINSIC Vc_PURE value_type operator[](size_t index) const noexcept
    {
        return data[index];
    }

    Vc_INTRINSIC Vc_PURE int count() const { return data.count(); }

    /**
     * Returns the index of the first one in the mask.
     *
     * The return value is undefined if the mask is empty.
     */
    Vc_INTRINSIC Vc_PURE int firstOne() const { return data.firstOne(); }

    template <typename G>
    static Vc_INTRINSIC fixed_size_simd_mask<T, N> generate(const G &gen)
    {
        return {private_init, mask_type::generate(gen)};
    }

    Vc_INTRINSIC Vc_PURE fixed_size_simd_mask<T, N> shifted(int amount) const
    {
        return {private_init, data.shifted(amount)};
    }

    /// \internal execute specified Operation
    template <typename Op, typename... Args>
    static Vc_INTRINSIC fixed_size_simd_mask<T, N> fromOperation(Op op, Args &&... args)
    {
        fixed_size_simd_mask<T, N> r;
        Common::unpackArgumentsAuto(op, r.data, std::forward<Args>(args)...);
        return r;
    }

    /// \internal
    Vc_INTRINSIC SimdMaskArray(private_init_t, mask_type &&x) : data(std::move(x)) {}

private:
    // The alignas attribute attached to the class declaration above is ignored by ICC
    // 17.0.0 (at least). So just move the alignas attribute down here where it works for
    // all compilers.
    alignas(static_cast<std::size_t>(
        Common::BoundedAlignment<Common::NextPowerOfTwo<N>::value * sizeof(VectorType_) /
                                 VectorType_::size()>::value)) storage_type data;
};

template <typename T, std::size_t N, typename VectorType> constexpr std::size_t SimdMaskArray<T, N, VectorType, N>::Size;
template <typename T, std::size_t N, typename VectorType>
constexpr std::size_t SimdMaskArray<T, N, VectorType, N>::MemoryAlignment;

// generic SimdMaskArray {{{1
/**
 * Data-parallel mask type with user-defined number of boolean elements.
 *
 * \tparam T The value type of the corresponding SimdArray. Depending on the target
 *           platform this type determines a different bit representation to work most
 *           efficient with SimdArray types instantiated for \p T.
 *
 * \tparam N The number of boolean elements to store and process concurrently. You can
 *           choose an arbitrary number, though not every number is a good idea.
 *           Generally, a power of two value or the sum of two power of two values might
 *           work efficiently, though this depends a lot on the target system.
 *
 * \tparam V Don't change the default value unless you really know what you are doing.
 *           This type is set to the underlying native Vc::Vector type used in the
 *           implementation of the type.
 *           Having it as part of the type name guards against some cases of ODR
 *           violations (i.e. linking incompatible translation units / libraries).
 *
 * \tparam Wt Don't ever change the default value.
 *           This parameter is an unfortunate implementation detail shining through.
 *
 * \headerfile simdmaskarray.h <Vc/SimdArray>
 */
template <typename T, size_t N, typename V, size_t Wt>
class SimdMaskArray
{
    static constexpr std::size_t N0 = Common::left_size<N>();

    using Split = Common::Split<N0>;

public:
    using storage_type0 = fixed_size_simd_mask<T, N0>;
    using storage_type1 = fixed_size_simd_mask<T, N - N0>;
    static_assert(storage_type0::size() == N0, "");

    using vector_type = fixed_size_simd<T, N>;

    friend storage_type0 &internal_data0(SimdMaskArray &m) { return m.data0; }
    friend storage_type1 &internal_data1(SimdMaskArray &m) { return m.data1; }
    friend const storage_type0 &internal_data0(const SimdMaskArray &m) { return m.data0; }
    friend const storage_type1 &internal_data1(const SimdMaskArray &m) { return m.data1; }

    using mask_type = SimdMaskArray;

    ///\copydoc Mask::size()
    static constexpr std::size_t size() { return N; }
    ///\copydoc Mask::Size
    static constexpr std::size_t Size = size();
    ///\copydoc Mask::MemoryAlignment
    static constexpr std::size_t MemoryAlignment =
        storage_type0::MemoryAlignment > storage_type1::MemoryAlignment
            ? storage_type0::MemoryAlignment
            : storage_type1::MemoryAlignment;
    static_assert(Size == vector_type::Size, "size mismatch");

    ///\internal
    using vectorentry_type = typename storage_type0::VectorEntryType;

    ///\copydoc Mask::value_type
    using value_type = typename storage_type0::EntryType;
    ///\copydoc Mask::Mask
    using MaskType = mask_type;
    ///\copydoc Mask::VectorEntryType
    using VectorEntryType = vectorentry_type;
    ///\copydoc Mask::EntryType
    using EntryType = value_type;
    ///\copydoc Mask::EntryReference
    using EntryReference = Vc::Detail::ElementReference<SimdMaskArray>;
    using reference = EntryReference;
    /// An alias for the corresponding SimdArray type.
    using Vector = fixed_size_simd<T, N>;

    Vc_FREE_STORE_OPERATORS_ALIGNED(alignof(mask_type));

    // zero init
    ///\copydoc Mask::Mask()
    SimdMaskArray() = default;

    // default copy ctor/operator
    SimdMaskArray(const SimdMaskArray &) = default;
    SimdMaskArray(SimdMaskArray &&) = default;
    SimdMaskArray &operator=(const SimdMaskArray &) = default;
    SimdMaskArray &operator=(SimdMaskArray &&) = default;

    // implicit conversion from SimdMaskArray with same N
    template <typename U, typename W>
    Vc_INTRINSIC SimdMaskArray(const SimdMaskArray<U, N, W> &rhs)
        : data0(Split::lo(rhs)), data1(Split::hi(rhs))
    {
    }

    // conversion from any Segment object (could be SimdMaskArray or Mask<T>)
    template <typename M, std::size_t Pieces, std::size_t Index>
    Vc_INTRINSIC SimdMaskArray(
        Common::Segment<M, Pieces, Index> &&rhs,
        enable_if<Traits::simd_vector_size<M>::value == Size * Pieces> = nullarg)
        : data0(Split::lo(rhs)), data1(Split::hi(rhs))
    {
    }

    // conversion from Mask<T>
    template <class M, class = enable_if<(Traits::is_simd_mask<M>::value &&
                                          !Traits::isSimdMaskArray<M>::value &&
                                          Traits::simd_vector_size<M>::value == Size)>>
    Vc_INTRINSIC SimdMaskArray(M k) : data0(Split::lo(k)), data1(Split::hi(k))
    {
    }

    // implicit conversion to Mask<U, AnyAbi> for if Mask<U, AnyAbi>::size() == N
    template <class U, class A,
              class = enable_if<Vc::Mask<U, A>::Size == N &&
                                !detail::is_fixed_size_abi<A>::value>>
    operator Vc::Mask<U, A>() const
    {
        return simd_cast<Vc::Mask<U, A>>(data0, data1);
    }
    Vc_INTRINSIC operator fixed_size_simd_mask<T, N> &()
    {
        return static_cast<fixed_size_simd_mask<T, N> &>(*this);
    }
    Vc_INTRINSIC operator const fixed_size_simd_mask<T, N> &() const
    {
        return static_cast<const fixed_size_simd_mask<T, N> &>(*this);
    }

    ///\copybrief Mask::Mask(VectorSpecialInitializerOne)
    Vc_INTRINSIC explicit SimdMaskArray(VectorSpecialInitializerOne one)
        : data0(one), data1(one)
    {
    }
    ///\copybrief Mask::Mask(VectorSpecialInitializerZero)
    Vc_INTRINSIC explicit SimdMaskArray(VectorSpecialInitializerZero zero)
        : data0(zero), data1(zero)
    {
    }
    ///\copydoc Mask::Mask(bool)
    Vc_INTRINSIC explicit SimdMaskArray(bool b) : data0(b), data1(b) {}

    ///\copydoc Mask::Zero()
    Vc_INTRINSIC static fixed_size_simd_mask<T, N> Zero()
    {
        return {storage_type0::Zero(), storage_type1::Zero()};
    }
    ///\copydoc Mask::One()
    Vc_INTRINSIC static fixed_size_simd_mask<T, N> One()
    {
        return {storage_type0::One(), storage_type1::One()};
    }

    ///\name Loads & Stores
    ///@{

    /**
     * Load N boolean values from the consecutive addresses starting at \p mem.
     *
     * \param mem A pointer to an array of booleans.
     * \param f A combination of flags to modify specific behavior of the load.
     */
    template <typename Flags = DefaultLoadTag>
    Vc_INTRINSIC explicit SimdMaskArray(const bool *mem, Flags f = Flags())
        : data0(mem, f), data1(mem + storage_type0::size(), f)
    {
    }

    /**
     * Load N boolean values from the consecutive addresses starting at \p mem.
     *
     * \param mem A pointer to an array of booleans.
     */
    Vc_INTRINSIC void load(const bool *mem)
    {
        data0.load(mem);
        data1.load(mem + storage_type0::size());
    }

    /**
     * Load N boolean values from the consecutive addresses starting at \p mem.
     *
     * \param mem A pointer to an array of booleans.
     * \param f A combination of flags to modify specific behavior of the load.
     */
    template <typename Flags> Vc_INTRINSIC void load(const bool *mem, Flags f)
    {
        data0.load(mem, f);
        data1.load(mem + storage_type0::size(), f);
    }

    /**
     * Store N boolean values to the consecutive addresses starting at \p mem.
     *
     * \param mem A pointer to an array of booleans.
     */
    Vc_INTRINSIC void store(bool *mem) const
    {
        data0.store(mem);
        data1.store(mem + storage_type0::size());
    }

    /**
     * Store N boolean values to the consecutive addresses starting at \p mem.
     *
     * \param mem A pointer to an array of booleans.
     * \param f A combination of flags to modify specific behavior of the load.
     */
    template <typename Flags> Vc_INTRINSIC void store(bool *mem, Flags f) const
    {
        data0.store(mem, f);
        data1.store(mem + storage_type0::size(), f);
    }
    ///@}

    ///\copydoc Mask::operator==
    Vc_INTRINSIC Vc_PURE bool operator==(const SimdMaskArray &mask) const
    {
        return data0 == mask.data0 && data1 == mask.data1;
    }
    ///\copydoc Mask::operator!=
    Vc_INTRINSIC Vc_PURE bool operator!=(const SimdMaskArray &mask) const
    {
        return data0 != mask.data0 || data1 != mask.data1;
    }

    ///\copybrief Mask::operator!
    Vc_INTRINSIC Vc_PURE fixed_size_simd_mask<T, N> operator!() const
    {
        return {!data0, !data1};
    }

    ///\copybrief Mask::operator&=
    Vc_INTRINSIC SimdMaskArray &operator&=(const SimdMaskArray &rhs)
    {
        data0 &= rhs.data0;
        data1 &= rhs.data1;
        return *this;
    }
    ///\copybrief Mask::operator|=
    Vc_INTRINSIC SimdMaskArray &operator|=(const SimdMaskArray &rhs)
    {
        data0 |= rhs.data0;
        data1 |= rhs.data1;
        return *this;
    }
    ///\copybrief Mask::operator^=
    Vc_INTRINSIC SimdMaskArray &operator^=(const SimdMaskArray &rhs)
    {
        data0 ^= rhs.data0;
        data1 ^= rhs.data1;
        return *this;
    }

    ///\copybrief Mask::operator&
    Vc_INTRINSIC Vc_PURE fixed_size_simd_mask<T, N> operator&(
        const SimdMaskArray &rhs) const
    {
        return {data0 & rhs.data0, data1 & rhs.data1};
    }
    ///\copybrief Mask::operator|
    Vc_INTRINSIC Vc_PURE fixed_size_simd_mask<T, N> operator|(
        const SimdMaskArray &rhs) const
    {
        return {data0 | rhs.data0, data1 | rhs.data1};
    }
    ///\copybrief Mask::operator^
    Vc_INTRINSIC Vc_PURE fixed_size_simd_mask<T, N> operator^(
        const SimdMaskArray &rhs) const
    {
        return {data0 ^ rhs.data0, data1 ^ rhs.data1};
    }

    ///\copybrief Mask::operator&&
    Vc_INTRINSIC Vc_PURE fixed_size_simd_mask<T, N> operator&&(
        const SimdMaskArray &rhs) const
    {
        return {data0 && rhs.data0, data1 && rhs.data1};
    }
    ///\copybrief Mask::operator||
    Vc_INTRINSIC Vc_PURE fixed_size_simd_mask<T, N> operator||(
        const SimdMaskArray &rhs) const
    {
        return {data0 || rhs.data0, data1 || rhs.data1};
    }

    ///\copybrief Mask::isFull
    Vc_INTRINSIC Vc_PURE bool isFull() const { return data0.isFull() && data1.isFull(); }
    ///\copybrief Mask::isNotEmpty
    Vc_INTRINSIC Vc_PURE bool isNotEmpty() const { return data0.isNotEmpty() || data1.isNotEmpty(); }
    ///\copybrief Mask::isEmpty
    Vc_INTRINSIC Vc_PURE bool isEmpty() const { return data0.isEmpty() && data1.isEmpty(); }
    ///\copybrief Mask::isMix
    Vc_INTRINSIC Vc_PURE bool isMix() const { return !isFull() && !isEmpty(); }

    ///\copydoc Mask::toInt
    Vc_INTRINSIC Vc_PURE int toInt() const
    {
        return data0.toInt() | (data1.toInt() << data0.size());
    }

private:
    friend reference;
    static Vc_INTRINSIC value_type get(const SimdMaskArray &o, int i) noexcept
    {
        if (i < int(o.data0.size())) {
            return o.data0[i];
        } else {
            return o.data1[i - o.data0.size()];
        }
    }
    template <typename U>
    static Vc_INTRINSIC void set(SimdMaskArray &o, int i, U &&v) noexcept(
        noexcept(std::declval<storage_type0 &>()[0] = std::declval<U>()) &&
        noexcept(std::declval<storage_type1 &>()[0] = std::declval<U>()))
    {
        if (i < int(o.data0.size())) {
            o.data0[i] = std::forward<U>(v);
        } else {
            o.data1[i - o.data0.size()] = std::forward<U>(v);
        }
    }

public:
    /**
     * Return a smart reference to the boolean element at index \p index.
     *
     * \param index The element index to be accessed.
     *
     * \returns A temporary smart reference object which acts as much as an lvalue
     * reference as possible.
     */
    Vc_INTRINSIC Vc_PURE reference operator[](size_t index) noexcept
    {
        return {*this, int(index)};
    }
    /**
     * Return a copy of the boolean element at index \p index.
     *
     * \param index The element index to be accessed.
     *
     * \returns A temporary boolean object with the value of the element at index \p
     * index.
     */
    Vc_INTRINSIC Vc_PURE value_type operator[](size_t index) const noexcept
    {
        return get(*this, index);
    }

    ///\copybrief Mask::count
    Vc_INTRINSIC Vc_PURE int count() const { return data0.count() + data1.count(); }

    ///\copydoc Mask::firstOne
    Vc_INTRINSIC Vc_PURE int firstOne() const {
        if (data0.isEmpty()) {
            return data1.firstOne() + storage_type0::size();
        }
        return data0.firstOne();
    }

    ///\copybrief Mask::generate
    template <typename G>
    static Vc_INTRINSIC fixed_size_simd_mask<T, N> generate(const G &gen)
    {
        return {storage_type0::generate(gen),
                storage_type1::generate([&](std::size_t i) { return gen(i + N0); })};
    }

    ///\copybrief Mask::shifted
    inline Vc_PURE fixed_size_simd_mask<T, N> shifted(int amount) const
    {
        if (Vc_IS_UNLIKELY(amount == 0)) {
            return *this;
        }
        return generate([&](unsigned i) {
            // modulo arithmetic of unsigned makes the check for j >= 0 unnecessary
            const unsigned j = i + amount;
            return j < size() ? get(*this, j) : false;
        });
    }

    /// \internal execute specified Operation
    template <typename Op, typename... Args>
    static Vc_INTRINSIC fixed_size_simd_mask<T, N> fromOperation(Op op, Args &&... args)
    {
        fixed_size_simd_mask<T, N> r = {
            storage_type0::fromOperation(op, Split::lo(args)...),  // no forward here - it
                                                                   // could move and thus
                                                                   // break the next line
            storage_type1::fromOperation(op, Split::hi(std::forward<Args>(args))...)};
        return r;
    }

    /// \internal
    Vc_INTRINSIC SimdMaskArray(storage_type0 &&x, storage_type1 &&y)
        : data0(std::move(x)), data1(std::move(y))
    {
    }

private:
    // The alignas attribute attached to the class declaration above is ignored by ICC
    // 17.0.0 (at least). So just move the alignas attribute down here where it works for
    // all compilers.
    alignas(static_cast<std::size_t>(
        Common::BoundedAlignment<Common::NextPowerOfTwo<N>::value * sizeof(V) /
                                 V::size()>::value)) storage_type0 data0;
    storage_type1 data1;
};
template <typename T, std::size_t N, typename V, std::size_t M>
constexpr std::size_t SimdMaskArray<T, N, V, M>::Size;
template <typename T, std::size_t N, typename V, std::size_t M>
constexpr std::size_t SimdMaskArray<T, N, V, M>::MemoryAlignment;

///}}}1
/// @}

}  // namespace Vc

// XXX: this include should be in <Vc/vector.h>. But at least clang 3.4 then fails to compile the
// code. Not sure yet what is going on, but it looks a lot like a bug in clang.
#include "simd_cast_caller.tcc"

#endif // VC_COMMON_SIMDMASKARRAY_H_

// vim: foldmethod=marker
