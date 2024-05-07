/*  This file is part of the Vc library. {{{
Copyright © 2015 Matthias Kretz <kretz@kde.org>

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

#ifndef VC_COMMON_MASK_H_
#define VC_COMMON_MASK_H_

#include "macros.h"

namespace Vc_VERSIONED_NAMESPACE
{
/**
 * \class Mask mask.h <Vc/vector.h>
 * \ingroup Masks
 *
 * The main SIMD mask class.
 */
template <typename T, typename Abi = VectorAbi::Best<T>> class Mask
{
public:
    /**
     * Returns the number of boolean components (\VSize{T}) in a mask of this type.
     *
     * The size of the mask. I.e. the number of boolean entries in the mask. Do not
     * make any assumptions about the size of masks.
     *
     * In addition, you can easily use if clauses that compare sizes. The compiler can
     * statically evaluate and fully optimize dead code away (very much like \#ifdef, but
     * with syntax checking).
     *
     * \returns The number of components (i.e. \VSize{T}) objects of this mask type store
     * and manipulate.
     */
    static constexpr size_t size() { return VectorTraits<T, Abi>::size(); }
    ///\copydoc size
    ///\deprecated Use Vc::Mask::size instead.
    static constexpr size_t Size = VectorTraits<T, Abi>::size();

    /**
     * Specifies the alignment requirement for aligned load and store calls for objects of
     * this mask type.
     */
    static constexpr size_t MemoryAlignment = VectorTraits<T, Abi>::maskMemoryAlignment();

    /// The ABI tag type of the current template instantiation.
    using abi = Abi;

    /**
     * The \c EntryType of masks is always \c bool, independent of \c T.
     */
    using EntryType = bool;
    /// \copydoc EntryType
    using value_type = EntryType;

    /// The reference wrapper type used for accessing individual mask components.
    using EntryReference = typename VectorTraits<T, Abi>::EntryReference;
    /// \copydoc EntryReference
    using value_reference = EntryReference;

    /**
     * The \c VectorEntryType, in contrast to \c EntryType, reveals information about the SIMD
     * implementation.
     * This type is useful for the \c sizeof operator in generic functions.
     */
    using VectorEntryType = typename VectorTraits<T, Abi>::VectorEntryType;

    /**\internal
     * The \c VectorType reveals the implementation-specific internal type used for the SIMD type.
     */
    using VectorType = typename VectorTraits<T, Abi>::VectorType;
    /**\internal
     * \copydoc VectorType
     */
    using vector_type = VectorType;

    /*
     * The associated Vector<T> type.
     */
    //using Vector = Vector<T, Abi>;

    /// \name Generators
    ///@{
    /**
     * Creates a new mask object initialized to zero/\c false.
     *
     * \returns A mask object with zero-initialized components.
     */
    Vc_INTRINSIC static Mask Zero();

    /**
     * Creates a mask object initialized to one/\c true.
     *
     * \returns A mask object with components initialized to \c true.
     */
    Vc_INTRINSIC static Mask One();

    /// Generate a mask object from booleans returned from the function \p gen.
    template <typename G> static Vc_INTRINSIC Mask generate(G &&gen);
    ///@}

    /// \name Compile-Time Constant Initialization
    ///@{
    /**
     * Construct a zero-initialized vector object.
     *
     * This constructor follows the behavior of the underlying \c bool type in that the
     * expression `bool()` zero-initializes the object (to \c false). On the other hand
     * the variable \c x in `bool x;` is uninitialized.
     * Since, for class types, both expressions call the default constructor `Mask<T> x`
     * must zero-initialize \c x as well.
     */
    Vc_INTRINSIC Mask() = default;

    /// Zero-initialize the new mask object (\c false).
    /// \see Vc::Zero, Zero()
    Vc_INTRINSIC explicit Mask(VectorSpecialInitializerZero);

    /// Initialize the new mask object to one (\c true).
    /// \see Vc::One, One()
    Vc_INTRINSIC explicit Mask(VectorSpecialInitializerOne);
    ///@}

    /// \name Conversion/Broadcast Constructors
    ///@{
    /**
     * Broadcast constructor.
     *
     * Set all components of the new mask object to \p b.
     *
     * \param b Determines the initial state of the mask.
     */
    Vc_INTRINSIC explicit Mask(bool b);

    /**
     * Implicit conversion from a compatible (equal \VSize{T} on every platform) mask
     * object.
     *
     * \param otherMask The mask to be converted.
     */
    template <typename U>
    Vc_INTRINSIC Mask(U &&otherMask,
                      Common::enable_if_mask_converts_implicitly<Mask, T, U> = nullarg);

#if Vc_IS_VERSION_1
    /**
     * Explicit conversion (static_cast) from a mask object that potentially has a
     * different \VSize{T}.
     *
     * \param otherMask The mask to be converted.
     *
     * \internal This is implemented via simd_cast in scalar/simd_cast_caller.h
     */
    template <typename U>
    Vc_DEPRECATED(
        "use simd_cast instead of explicit type casting to convert between mask types")
        Vc_INTRINSIC_L
        explicit Mask(U &&otherMask, Common::enable_if_mask_converts_explicitly<T, U> =
                                         nullarg) Vc_INTRINSIC_R;
    ///@}
#endif

    /**
     * \name Loads & Stores
     */
    ///@{
    /**
     * Load constructor from an array of \c bool.
     *
     * This constructor implements an explicit conversion from an array of booleans to a
     * mask object. It corresponds to a Vector load constructor.
     *
     * \param mem A pointer to the start of the array of booleans.
     * \see Mask(const bool *, Flags), load(const bool *)
     */
    Vc_ALWAYS_INLINE explicit Mask(const bool *mem);
    /**
     * Overload of the above with a load/store flag argument.
     *
     * \param mem A pointer to the start of the array of booleans.
     * \param flags Choose a combination of flags such as Vc::Aligned, Vc::Streaming,
     * Vc::Unaligned, Vc::PrefetchDefault, ...
     * \see load(const bool *, Flags)
     */
    template <typename Flags> Vc_ALWAYS_INLINE explicit Mask(const bool *mem, Flags flags);

    /**
     * Load the components of the mask from an array of \c bool.
     *
     * \param mem A pointer to the start of the array of booleans.
     * \see load(const bool *, Flags), Mask(const bool *)
     */
    Vc_ALWAYS_INLINE void load(const bool *mem);
    /**
     * Overload of the above with a load/store flag argument.
     *
     * \param mem A pointer to the start of the array of booleans.
     * \param flags Choose a combination of flags such as Vc::Aligned, Vc::Streaming,
     * Vc::Unaligned, Vc::PrefetchDefault, ...
     * \see Mask(const bool *, Flags)
     */
    template <typename Flags> Vc_ALWAYS_INLINE void load(const bool *mem, Flags flags);

    /**
     * Store the values of the mask to an array of \c bool.
     *
     * \param mem A pointer to the start of the array of booleans.
     * \see store(bool *, Flags)
     */
    Vc_ALWAYS_INLINE void store(bool *mem) const;
    /**
     * Overload of the above with a load/store flag argument.
     *
     * \param mem A pointer to the start of the array of booleans.
     * \param flags Choose a combination of flags such as Vc::Aligned, Vc::Streaming,
     * Vc::Unaligned, Vc::PrefetchDefault, ...
     */
    template <typename Flags> Vc_ALWAYS_INLINE void store(bool *mem, Flags flags) const;
    ///@}

    /// \name Comparison Operators
    ///@{
    /**
     * Returns whether the two masks are equal in all components.
     *
     * \param mask The other mask to compare against.
     * \returns A scalar boolean value that says whether all components of the two masks
     * are equal.
     *
     * \note If you expected a behavior similar to the compare operator of Vc::Vector,
     * consider that the bitwise operators already implement such functionality. There is
     * little use, typically, in having `a == b` return the same as `a ^ b`. In general,
     * it is more useful to query `all_of(a ^ b)` which is the same as this equality
     * operator.
     */
    Vc_ALWAYS_INLINE bool operator==(const Mask &mask) const;

    /**
     * Returns whether the two masks are different in at least one component.
     *
     * \param mask The other mask to compare against.
     * \returns A scalar boolean value that says whether at least one component of the two masks is different.
     *
     * \note `(a == b) == !(a != b)` holds
     * \see Mask::operator==(const Mask &)
     */
    Vc_ALWAYS_INLINE bool operator!=(const Mask &mask) const;
    ///@}

    /**
     * \name Logical and Binary Operators
     *
     * \brief Component-wise logical/binary operations on mask objects.
     *
     * The effect of logical and binary \c AND and \c OR is equivalent for mask types (as
     * it is for \c bool).
     */
    ///@{

    /// Returns the component-wise application of a logical \c AND to \p mask.
    Vc_ALWAYS_INLINE Mask operator&&(const Mask &mask) const;
    /// Returns the component-wise application of a binary \c AND to \p mask.
    Vc_ALWAYS_INLINE Mask operator&(const Mask &mask) const;
    /// Returns the component-wise application of a logical \c OR to \p mask.
    Vc_ALWAYS_INLINE Mask operator||(const Mask &mask) const;
    /// Returns the component-wise application of a binary \c OR to \p mask.
    Vc_ALWAYS_INLINE Mask operator|(const Mask &mask) const;
    /// Returns the component-wise application of a binary \c XOR to \p mask.
    Vc_ALWAYS_INLINE Mask operator^(const Mask &mask) const;
    /// Returns a mask with inverted components.
    Vc_ALWAYS_INLINE Mask operator!() const;

    /// Modifies the mask using an \c AND operation with \p mask.
    Vc_ALWAYS_INLINE Mask &operator&=(const Mask &mask);
    /// Modifies the mask using an \c OR operation with \p mask.
    Vc_ALWAYS_INLINE Mask &operator|=(const Mask &mask);
    /// Modifies the mask using an \c XOR operation with \p mask.
    Vc_ALWAYS_INLINE Mask &operator^=(const Mask &mask);
    ///@}

    /**
     * \name Reductions
     *
     * \see any_of, all_of, none_of, some_of
     */
    ///@{

    /// Returns a logical \c AND of all components.
    Vc_ALWAYS_INLINE bool isFull() const;
    /// Returns a logical \c OR of all components.
    Vc_ALWAYS_INLINE bool isNotEmpty() const;
    /// Returns \c true if components are \c false, \c false otherwise.
    Vc_ALWAYS_INLINE bool isEmpty() const;
    /// Returns `!isFull() && !isEmpty()`.
    Vc_ALWAYS_INLINE bool isMix() const;
    ///@}

    /**\internal
     * \name Internal Data Access
     */
    ///@{
    Vc_ALWAYS_INLINE bool data() const;
    Vc_ALWAYS_INLINE bool dataI() const;
    Vc_ALWAYS_INLINE bool dataD() const;
    ///@}

    /// \name Scalar Subscript Operators
    ///@{
    /**
     * Lvalue-reference-like access to mask entries.
     *
     * \param index Determines the boolean to be accessed.
     * \return a temporary proxy object referencing the \p index th entry of the mask.
     *
     * \warning This operator does not return an lvalue reference (to \c bool), but rather
     * a temporary (rvalue) object that mimics an lvalue reference (as much as is possible
     * with C++11/14).
     */
    Vc_ALWAYS_INLINE EntryReference operator[](size_t index);

    /**
     * Read-only access to mask entries.
     *
     * \param index Determines the boolean to be accessed.
     * \return The \p index th entry of the mask as a \c bool (rvalue).
     *
     * \warning This operator does not return an lvalue reference (to `const bool`), but
     * rather a temporary (rvalue) \c bool.
     */
    Vc_ALWAYS_INLINE EntryType operator[](size_t index) const;
    ///@}

    /// Returns how many components of the mask are \c true.
    Vc_ALWAYS_INLINE int count() const;

    /**
     * Returns the index of the first one in the mask.
     *
     * \returns the index of the first component that is \c true.
     *
     * \warning The return value is undefined if the mask is empty.
     *
     * Thus, unless `none_of(mask)`, `mask[mask.firstOne()] == true` holds and `mask[i] ==
     * false` for all `i < mask.firstOne()`.
     */
    Vc_ALWAYS_INLINE int firstOne() const;

    /**
     * Convert the boolean components of the mask into bits of an integer.
     *
     * \return An \c int where each bit corresponds to the boolean value in the mask.
     *
     * For example, the mask `[true, false, false, true]` results in a `9` (in binary: `1001`).
     */
    Vc_ALWAYS_INLINE int toInt() const;

    /// Returns a mask with components shifted by \p amount places.
    Vc_INTRINSIC Vc_PURE Mask shifted(int amount) const;

    Vc_FREE_STORE_OPERATORS_ALIGNED(alignof(Mask));

private:
    VectorType d;
};

/**
 * \ingroup Utilities
 *
 * \name Boolean Reductions
 */
//@{
/** \ingroup Utilities
 *  Returns whether all entries in the mask \p m are \c true.
 */
template<typename Mask> constexpr bool all_of(const Mask &m) { return m.isFull(); }
/** \ingroup Utilities
 *  Returns \p b
 */
constexpr bool all_of(bool b) { return b; }

/** \ingroup Utilities
 *  Returns whether at least one entry in the mask \p m is \c true.
 */
template<typename Mask> constexpr bool any_of(const Mask &m) { return m.isNotEmpty(); }
/** \ingroup Utilities
 *  Returns \p b
 */
constexpr bool any_of(bool b) { return b; }

/** \ingroup Utilities
 *  Returns whether all entries in the mask \p m are \c false.
 */
template<typename Mask> constexpr bool none_of(const Mask &m) { return m.isEmpty(); }
/** \ingroup Utilities
 *  Returns \p !b
 */
constexpr bool none_of(bool b) { return !b; }

/** \ingroup Utilities
 *  Returns whether at least one entry in \p m is \c true and at least one entry in \p m is \c
 *  false.
 */
template<typename Mask> constexpr bool some_of(const Mask &m) { return m.isMix(); }
/** \ingroup Utilities
 *  Returns \c false
 */
constexpr bool some_of(bool) { return false; }
//@}
}  // namespace Vc

#endif  // VC_COMMON_MASK_H_

// vim: foldmethod=marker
