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

#ifndef VC_COMMON_VECTOR_H_
#define VC_COMMON_VECTOR_H_

#include <ratio>
#include "elementreference.h"
#include "types.h"
#include "vectorabi.h"
#include "vectortraits.h"
#include "simdarrayfwd.h"
#include "loadstoreflags.h"
#include "writemaskedvector.h"
#include "detail.h"

namespace Vc_VERSIONED_NAMESPACE
{
/**
 * \ingroup Math
 * Copies the sign(s) of \p sign to the value(s) in \p magnitude and returns the resulting
 * vector.
 *
 * \param magnitude This vector's magnitude will be used in the return vector.
 * \param sign This vector's sign bit will be used in the return vector.
 *
 * \return a value where the sign of the value equals the sign of \p sign. I.e.
 * `sign(copysign(v, r)) == sign(r)`.
 */
template <typename T, typename Abi,
          typename = enable_if<std::is_floating_point<T>::value &&
                               !detail::is_fixed_size_abi<Abi>::value>>
inline Vector<T, Abi> copysign(Vector<T, Abi> magnitude, Vector<T, Abi> sign);

/**
 * \ingroup Math
 * Extracts the exponent of each floating-point vector component.
 *
 * \param x The vector of values to check for the sign.
 * \return the exponent to base 2.
 *
 * This function provides efficient access to the exponent of the floating point number. The
 * returned value is a fast approximation to the logarithm of base 2. The absolute error of that
 * approximation is between [0, 1[.
 *
 * Examples:
\verbatim
 value | exponent | log2
=======|==========|=======
   1.0 |        0 | 0
   2.0 |        1 | 1
   3.0 |        1 | 1.585
   3.9 |        1 | 1.963
   4.0 |        2 | 2
   4.1 |        2 | 2.036
\endverbatim
 *
 * \warning This function assumes a positive value (non-zero). If the value is negative the sign bit will
 * modify the returned value. An input value of zero will return the bias of the floating-point
 * representation. If you compile with Vc runtime checks, the function will assert
 * values greater than or equal to zero.
 *
 * You may use abs to apply this function to negative values:
 * \code
 * exponent(abs(v))
 * \endcode
 */
template <typename T, typename Abi,
          typename = enable_if<std::is_floating_point<T>::value &&
                               !detail::is_fixed_size_abi<Abi>::value>>
inline Vector<T, Abi> exponent(Vector<T, Abi> x);

/**
 * \ingroup Math
 * Returns for each vector component whether it stores a negative value.
 *
 * \param x The vector of values to check for the sign.
 * \returns a mask which is \c true only in those components that are negative in \p x.
 */
template <typename T, typename Abi>
Vc_INTRINSIC Vc_CONST typename Vector<T, detail::not_fixed_size_abi<Abi>>::MaskType
isnegative(Vector<T, Abi> x)
{
    return x < Vector<T, Abi>::Zero();
}

/**
 * \class Vector types.h <Vc/vector.h>
 * \ingroup Vectors
 *
 * The main vector class for expressing data parallelism.
 *
 * are specializations of this class.
 * For most cases there are no API differences for the specializations.
 * Make use of Vector<T> for generic programming, otherwise you might prefer to use
 * the \p *_v aliases.
 *
 * \see Vc::float_v, Vc::double_v, Vc::int_v, Vc::uint_v, Vc::short_v, Vc::ushort_v
 * \see Mask
 */
template<typename T, typename Abi = VectorAbi::Best<T>> class Vector
{
public:
    /**
     * Returns the number of scalar components (\VSize{T}) in a vector of this type.
     *
     * The size of the vector. I.e. the number of scalar entries in the vector. Do not
     * make any assumptions about the size of vectors. If you need vectors of \c float and
     * \c int types use Vector::IndexType or SimdArray.
     *
     * You can easily use if clauses to compare Vector sizes. The compiler can
     * statically evaluate and fully optimize dead code away (very much like \#ifdef, but
     * with syntax checking).
     *
     * \returns The number of components (i.e. \VSize{T}) objects of this vector type
     * store and manipulate.
     */
    static constexpr size_t size() { return VectorTraits<T, Abi>::size(); }

    /**
     * Specifies the alignment requirement for aligned load and store calls for objects of
     * this vector type.
     */
    static constexpr size_t MemoryAlignment = VectorTraits<T, Abi>::memoryAlignment();

    /// The ABI tag type of the current template instantiation.
    using abi = Abi;

    /// The type of the entries in the vector.
    using EntryType = typename VectorTraits<T, Abi>::EntryType;
    /// \copydoc EntryType
    using value_type = EntryType;

    using VectorEntryType = typename VectorTraits<T, Abi>::VectorEntryType;
    /**\internal
     * This type reveals the implementation-specific type used for the data member.
     */
    using VectorType = typename VectorTraits<T, Abi>::VectorType;
    /**\internal
     * \copydoc VectorType
     */
    using vector_type = VectorType;

    /// The type of the mask used for masked operations and returned from comparisons.
    using MaskType = Vc::Mask<T, Abi>;
    /// \copydoc MaskType
    using mask_type = MaskType;

    using MaskArgument = MaskType;
    using VectorArgument = Vector;

    /// The type of the vector used for indexes in gather and scatter operations.
    using IndexType = Vc::fixed_size_simd<int, VectorTraits<T, Abi>::size()>;
    /// \copydoc IndexType
    using index_type = IndexType;

    using reference = Detail::ElementReference<Vector>;

    /// \name Generators
    ///@{
    /**
     * Returns a vector with the entries initialized to zero.
     */
    static inline Vector Zero();

    /**
     * Returns a vector with the entries initialized to one.
     */
    static inline Vector One();

    /**
     * Returns a vector with the entries initialized to 0, 1, 2, 3, 4, 5, ...
     */
    static inline Vector IndexesFromZero();

    /**
     * Returns a vector with pseudo-random entries.
     *
     * Currently the state of the random number generator cannot be modified and starts
     * off with the same state. Thus you will get the same sequence of numbers for the
     * same sequence of calls.
     *
     * \return a new random vector. Floating-point values will be in the 0-1 range.
     * Integers will use the full range the integer representation allows.
     *
     * \note This function may use a very small amount of state and thus will be a weak
     * random number generator.
     */
    static inline Vector Random();

    /// Generate a vector object from return values of \p gen (static variant of \ref fill).
    template <typename G> static inline Vector generate(G gen);
    ///@}

    /// \name Compile-Time Constant Initialization
    ///@{
    /**
     * Construct a zero-initialized vector object.
     *
     * This constructor follows the behavior of the underlying arithmetic type \p T in
     * that the expression `T()` zero-initializes the object. On the other hand the
     * variable \c x in `T x;` is uninitialized.
     * Since, for class types, both expressions call the default constructor `Vector<T> x`
     * must zero-initialize \c x as well.
     */
    inline Vector() = default;

    /**
     * Construct a vector with the entries initialized to zero.
     *
     * \see Vc::Zero, Zero()
     */
    explicit inline Vector(VectorSpecialInitializerZero);

    /**
     * Construct a vector with the entries initialized to one.
     *
     * \see Vc::One, One()
     */
    explicit inline Vector(VectorSpecialInitializerOne);

    /**
     * Construct a vector with the entries initialized to 0, 1, 2, 3, 4, 5, ...
     *
     * \see Vc::IndexesFromZero, IndexesFromZero()
     */
    explicit inline Vector(VectorSpecialInitializerIndexesFromZero);
    ///@}

    /// \name Conversion/Broadcast Constructors
    ///@{
    /**
     * Implict conversion from compatible Vector<U, Abi> types.
     */
    template <typename U>
    inline Vector(Vector<U, abi> x,
                  enable_if<Traits::is_implicit_cast_allowed<U, T>::value> = nullarg);

#if Vc_IS_VERSION_1
    /**
     * Explicit conversion (i.e. `static_cast`) from the remaining Vector<U, Abi> types.
     *
     * \param x A vector object to use for initialization of the new vector object. If \p
     *          x contains more entries than the new object the high components will be
     *          ignored. If \p x contains fewer entries than the new object the high
     *          components of the new object will be zero-initialized. Type conversion is
     *          done according to the standard conversion rules for the underlying
     *          fundamental arithmetic types.
     */
    template <typename U>
    Vc_DEPRECATED("use simd_cast instead of explicit type casting to convert between "
                  "vector types") inline explicit Vector(
        Vector<U, abi> x,
        enable_if<!Traits::is_implicit_cast_allowed<U, T>::value> = nullarg);
#endif

    /**
     * Broadcast Constructor.
     *
     * Constructs a vector with all entries of the vector filled with the given value.
     *
     * \param a The scalar value to broadcast to all entries of the constructed vector.
     */
    inline Vector(EntryType a);
    template <typename U>
    inline Vector(U a, enable_if<std::is_same<U, int>::value &&
                                 !std::is_same<U, EntryType>::value> = nullarg);
    inline explicit Vector(reference a);
    ///@}

    /**
     * \name Loads & Stores
     */
    ///@{
#include "../common/loadinterface.h"
#include "../common/storeinterface.h"
    ///@}

    /**
     * Set all entries to zero.
     */
    inline void setZero();

    /**
     * Set all entries to zero where the mask is set.
     *
     * A 4-vector with a mask of `[0111]` therefore would set the last three entries to 0.
     *
     * \param mask Selects the entries to be set to zero.
     */
    inline void setZero(MaskType mask);

    /**
     * Set all entries to zero where the mask is not set.
     *
     * A 4-vector with a mask of `[0111]` therefore would set only the first entry to 0.
     *
     * \param mask Selects the entries to not be set to zero.
     */
    inline void setZeroInverted(MaskType mask);

    /**
     * Set all entries to the bit representation of a QNaN.
     */
    inline void setQnan();

    /**
     * Set all entries to the bit representation of a QNaN where the mask is set.
     *
     * \param mask Selects the entries to be set to QNaN.
     */
    inline void setQnan(MaskType mask);

#define Vc_CURRENT_CLASS_NAME Vector
#include "../common/gatherinterface.h"
#include "../common/scatterinterface.h"
#undef Vc_CURRENT_CLASS_NAME

    /// \name Scalar Subscript Operators
    ///@{
    /**
     * This operator can be used to modify scalar entries of the vector.
     *
     * \param index A value between 0 and Size. This value is not checked internally so
     *              you must make/be sure it is in range.
     *
     * \return a reference to the vector entry at the given \p index.
     *
     * \warning The use of this function may result in suboptimal performance. Please
     *          check whether you can find a more vector-friendly way to do what you
     *          intended.
     * \note the returned object models the concept of a reference and
     * as such it can exist longer than the data it is referencing.
     * \note to avoid lifetime issues, we strongly advice not to store
     * any reference objects.
     */
    inline reference operator[](size_t index) noexcept;
    /**
     * This operator can be used to read scalar entries of the vector.
     *
     * \param index A value between 0 and Size. This value is not checked internally so
     *              you must make/be sure it is in range.
     *
     * \return a copy of the vector entry at the given \p index.
     */
    inline EntryType operator[](size_t index) const noexcept;
    ///@}

    /// \name Unary Operators
    ///@{
    /**
     * Determine where the vector is null.
     *
     * \returns a mask which denotes the zero entries of this vector object.
     */
    inline MaskType operator!() const;

    /**
     * Inverts all bits.
     *
     * \returns a new vector which has all bits inverted. I.e. `v & ~v == 0`.
     *
     * \note This operator is only defined for integral types \p T.
     */
    inline Vector operator~() const;

    /// Returns a new vector object with all entries negated.
    inline Vector operator-() const;
    /// Returns a copy of the vector object.
    inline Vector operator+() const;
    ///@}

    /**
     * \name Increment and Decrement Operators
     * The increment and decrement operators apply the increment/decrement operation per
     * component.
     *
     * The semantics are equal to the semantics of the fundamental arithmetics type \p T.
     *
     * \note Over-/Underflow of signed integral types is undefined behavior and may
     * actually break your code.
     */
    ///@{
    inline Vector &operator++();  // prefix
    inline Vector operator++(int);  // postfix
    inline Vector &operator--();  // prefix
    inline Vector operator--(int);  // postfix
    ///@}

#define Vc_OP(symbol)                                                                    \
    inline Vc_PURE Vector operator symbol(const Vector &x) const;
    /**
     * \name Arithmetic Operations
     *
     * The arithmetic operations are implemented as component-wise
     * application of the operator on the two vector objects.
     *
     * Example:
     * \code
     * void foo(float_v a, float_v b) {
     *   const float_v product    = a * b;
     *   const float_v difference = a - b;
     *   a += b;
     *   auto quotient = a / b;
     *   auto modulo = static_cast<int_v>(a) % static_cast<int_v>(b);
     * }
     * \endcode
     *
     * \param x The vector to add, subtract, multiply, or divide by.
     * \returns A vector object of the same type with the components filled according to a
     *          component-wise application of the operator.
     *
     * \note If a signed integral vector operation overflows the result is undefined.
     * (which is in agreement to the behavior of the fundamental signed integral types in
     * C++)
     */
    ///@{
    Vc_ALL_ARITHMETICS(Vc_OP);
    ///@}

    /**
     * \name Binary Operations
     *
     * The binary operations are implemented as component-wise
     * application of the operator on the two vector objects.
     *
     * Example:
     * \code
     * void foo(int_v a, int_v b) {
     *   const int_v combined_bits = a | b;
     *   const int_v masked_bits = a & b;
     *   a ^= b;  // flipped bits
     * }
     * \endcode
     *
     * \returns A vector object of the same type with the components filled according to a
     *          component-wise application of the operator.
     */
    ///@{
    Vc_ALL_BINARY(Vc_OP);
    ///@}

    /**
     * \name Shift Operations
     *
     * The shift operations are implemented as component-wise
     * application of the operator on the two vector objects.
     *
     * Example:
     * \code
     * void foo(int_v a, int_v b) {
     *   const int_v right = a >> b;
     *   a <<= b;
     * }
     * \endcode
     *
     * \returns A vector object of the same type with the components filled according to a
     *          component-wise application of the operator.
     */
    ///@{
    Vc_ALL_SHIFTS(Vc_OP);
    ///@}
#undef Vc_OP

    /**
     * \name Comparisons
     *
     * All comparison operators return a mask object.
     *
     * Example:
     * \code
     * void foo(const float_v &a, const float_v &b) {
     *   const float_m mask = a < b;
     *   ...
     * }
     * \endcode
     *
     * \param x The vector to compare against.
     * \returns A mask object. Its components contain the boolean results of the
     *          component-wise compare operation.
     */
    ///@{
#define Vc_CMP_OP(symbol) inline Vc_PURE MaskType operator symbol(const Vector &x) const;
    Vc_ALL_COMPARES(Vc_CMP_OP);
#undef Vc_CMP_OP
    ///@}

    /**
     * Writemask the vector before an assignment.
     *
     * \param mask The writemask to be used.
     *
     * \return an object that can be used for any kind of masked assignment.
     *
     * The returned object is only to be used for assignments and should not be assigned
     * to a variable.
     *
     * Examples:
     * \code
     * float_v v = float_v::Zero();         // v  = [0, 0, 0, 0]
     * int_v v2 = int_v::IndexesFromZero(); // v2 = [0, 1, 2, 3]
     * v(v2 < 2) = 1.f;                     // v  = [1, 1, 0, 0]
     * v(v2 < 3) += 1.f;                    // v  = [2, 2, 1, 0]
     * ++v2(v < 1.f);                       // v2 = [0, 1, 2, 4]
     * \endcode
     */
    inline Common::WriteMaskedVector<Vector, MaskType> operator()(MaskType mask);

    /**
     * \name Horizontal Reduction Operations
     *
     * Horizontal operations can be used to reduce the values of a vector to a scalar
     * value.
     *
     * Example:
     * \code
     * void foo(const float_v &v) {
     *   float min = v.min(); // smallest value in v
     *   float sum = v.sum(); // sum of all values in v
     * }
     * \endcode
     */
    ///@{

    /// Returns the smallest entry in the vector.
    inline EntryType min() const;
    /// Returns the largest entry in the vector.
    inline EntryType max() const;
    /// Returns the product of all entries in the vector.
    inline EntryType product() const;
    /// Returns the sum of all entries in the vector.
    inline EntryType sum() const;
    /// Returns a vector containing the sum of all entries with smaller index.
    inline Vector partialSum() const;
    /// Returns the smallest entry of the vector components selected by \p mask.
    inline EntryType min(MaskType mask) const;
    /// Returns the largest entry of the vector components selected by \p mask.
    inline EntryType max(MaskType mask) const;
    /// Returns the product of the vector components selected by \p mask.
    inline EntryType product(MaskType mask) const;
    /// Returns the sum of the vector components selected by \p mask.
    inline EntryType sum(MaskType mask) const;
    ///@}

    /**
     * \name Shift and Rotate
     *
     * These functions allow to shift or rotate the entries in a vector.
     *
     * All functions with an \p amount parameter support positive and negative numbers for
     * the shift/rotate value.
     *
     * Example:
     * \code
     * using namespace Vc;
     * int_v foo = int_v::IndexesFromZero() + 1; // e.g. [1, 2, 3, 4] with SSE
     * int_v x;
     * x = foo.shifted( 1); // [2, 3, 4, 0]
     * x = foo.shifted( 2); // [3, 4, 0, 0]
     * x = foo.shifted( 3); // [4, 0, 0, 0]
     * x = foo.shifted( 4); // [0, 0, 0, 0]
     * x = foo.shifted(-1); // [0, 1, 2, 3]
     * x = foo.shifted(-2); // [0, 0, 1, 2]
     * x = foo.shifted(-3); // [0, 0, 0, 1]
     * x = foo.shifted(-4); // [0, 0, 0, 0]
     *
     * x = foo.rotated( 1); // [2, 3, 4, 1]
     * x = foo.rotated( 2); // [3, 4, 1, 2]
     * x = foo.rotated( 3); // [4, 1, 2, 3]
     * x = foo.rotated( 4); // [1, 2, 3, 4]
     * x = foo.rotated(-1); // [4, 1, 2, 3]
     * x = foo.rotated(-2); // [3, 4, 1, 2]
     * x = foo.rotated(-3); // [2, 3, 4, 1]
     * x = foo.rotated(-4); // [1, 2, 3, 4]
     * \endcode
     *
     * These functions are slightly related to the above swizzles. In any case, they are
     * often useful for communication between SIMD lanes or binary decoding operations.
     *
     * \warning Use of these functions leads to less portable code. Consider the scalar
     * implementation where every vector has only one entry. The shift and rotate
     * functions have no useful task to fulfil there and you will almost certainly not get
     * any useful results. It is recommended to add a static_assert for the assumed
     * minimum vector size.
     */
    ///@{

    /// Shift vector entries to the left by \p amount; shifting in zeros.
    inline Vector shifted(int amount) const;
    /**
     * Shift vector entries to the left by \p amount; shifting in values from shiftIn
     * (instead of zeros).
     *
     * This function can be used to create vectors from unaligned memory locations.
     *
     * Example:
     * \code
     * Vc::Memory<int_v, 256> mem;
     * for (int i = 0; i < 256; ++i) { mem[i] = i + 1; }
     * int_v a = mem.vectorAt(0);
     * int_v b = mem.vectorAt(int_v::Size);
     * int_v x = a.shifted(1, b);
     * // now x == mem.vectorAt(1, Vc::Unaligned)
     * \endcode
     *
     * \param amount  The number of entries to shift by. \p amount must be between \c
     *                -Size and \c Size, otherwise the result is undefined.
     * \param shiftIn The vector of values to shift in.
     * \return        A new vector with values from \p this and \p shiftIn concatenated
     *                and then shifted by \p amount.
     */
    inline Vector shifted(int amount, Vector shiftIn) const;
    /// Rotate vector entries to the left by \p amount.
    inline Vector rotated(int amount) const;
    /// Returns a vector with all components reversed.
    inline Vector reversed() const;
    ///@}

    /**
     * Return a sorted copy of the vector.
     *
     * \returns a sorted vector. The returned values are in ascending order:
       \verbatim
       v[0] <= v[1] <= v[2] <= v[3] ...
       \endverbatim
     *
     * \note If the vector contains NaNs the result is undefined.
     *
     * Example:
     * \code
     * int_v v = int_v::Random();
     * int_v s = v.sorted();
     * std::cout << v << '\n' << s << '\n';
     * \endcode
     *
     * With SSE the output would be:
     *
       \verbatim
       [1513634383, -963914658, 1763536262, -1285037745]
       [-1285037745, -963914658, 1513634383, 1763536262]
       \endverbatim
     *
     * With the Scalar implementation:
       \verbatim
       [1513634383]
       [1513634383]
       \endverbatim
     */
    inline Vector sorted() const;

    /*!
     * \name Apply/Call/Fill Functions
     *
     * There are still many situations where the code needs to switch from SIMD operations
     * to scalar execution. In this case you can, of course rely on operator[]. But there
     * are also a number of functions that can help with common patterns.
     *
     * The apply functions expect a function that returns a scalar value, i.e. a function
     * of the form "T f(T)".  The call functions do not return a value and thus the
     * function passed does not need a return value. The fill functions are used to
     * serially set the entries of the vector from the return values of a function.
     *
     * Example:
     * \code
     * void foo(float_v v) {
     *   float_v logarithm = v.apply(std::log);
     *   float_v exponential = v.apply(std::exp);
     * }
     * \endcode
     *
     * Of course, you can also use lambdas here:
     * \code
     *   float_v power = v.apply([](float f) { return std::pow(f, 0.6f); })
     * \endcode
     *
     * \param f A functor: this can either be a function or an object that implements
     * operator().
     */
    ///@{

    /// Call \p f sequentially, starting with the minimum up to the maximum value.
    template <typename F> void callWithValuesSorted(F &&f);
    /// Call \p f with the scalar entries of the vector.
    template <typename F> inline void call(F &&f) const;
    /// As above, but skip the entries where \p mask is not set.
    template <typename F> inline void call(F &&f, MaskType mask) const;

    /// Call \p f on every entry of the vector and return the results as a new vector.
    template <typename F> inline Vector apply(F &&f) const;
    /// As above, but skip the entries where \p mask is not set.
    template <typename F> inline Vector apply(F &&f, MaskType mask) const;

    /// Fill the vector with the values [f(0), f(1), f(2), ...].
    template <typename IndexT> inline void fill(EntryType(&f)(IndexT));
    /// Fill the vector with the values [f(), f(), f(), ...].
    inline void fill(EntryType(&f)());
    ///@}

    /**\internal
     * Interleaves this vector and \p x and returns the resulting low vector.
     * Used to implement Vc::interleave.
     */
    inline Vector interleaveLow(Vector x) const;
    /**\internal
     * Interleaves this vector and \p x and returns the resulting low vector.
     * Used to implement Vc::interleave.
     */
    inline Vector interleaveHigh(Vector x) const;

    /**\internal
     * Assigns the components of \p v where \p m is \c true.
     */
    inline void assign(const Vector &v, const MaskType &m);

    /**
     * \internal
     * \name Internal Data Access
     * Returns a (const) reference the internal data member, storing the vector data.
     */
    ///@{
    inline VectorType &data();
    inline const VectorType &data() const;
    ///@}

    /// \name Deprecated Members
    ///@{

    /**
     * Returns the exponents of the floating-point values in the vector.
     *
     * \return A new vector object of the same type containing the exponents.
     *
     * \deprecated use Vc::exponent instead.
     */
    Vc_DEPRECATED("use exponent(x) instead") inline Vector exponent() const;

    /**
     * Returns whether a value is negative.
     *
     * \return A new mask object indicating the sign of each vector element.
     *
     * \deprecated use Vc::isnegative instead.
     */
    Vc_DEPRECATED("use isnegative(x) instead") inline MaskType isNegative() const;

    ///\copydoc size
    ///\deprecated Use Vc::Vector::size instead.
    static constexpr size_t Size = VectorTraits<T, Abi>::size();

    /**
     * Casts the current object to \p V2.
     *
     * \returns a converted object of type \p Vc.
     *
     * \deprecated Use Vc::simd_cast instead.
     */
    template <typename V2> inline V2 staticCast() const;

    /**
     * reinterpret_cast the vector components to construct a vector of type \p V2.
     *
     * \returns An object of type \p V2 with the smae bit-representation.
     *
     * \deprecated use Vc::reinterpret_components_cast instead.
     */
    template <typename V2>
    Vc_DEPRECATED("use reinterpret_components_cast instead") inline V2
        reinterpretCast() const;

    /**
     * Copies the signs of the components of \p reference to the components of the current
     * vector, returning the result.
     *
     * \param reference A vector object that determines the sign of the the result.
     * \returns A new vector with sign taken from \p reference and absolute value taken
     * from the current vector object.
     *
     * \deprecated Use Vc::copysign instead.
     */
    Vc_DEPRECATED("use copysign(x, y) instead") inline Vector
        copySign(Vector reference) const;
    ///@}

    Vc_FREE_STORE_OPERATORS_ALIGNED(alignof(Vector));

private:
    VectorType d;
};

/**
 * \ingroup Utilities
 * Constructs a new Vector object of type \p V from the Vector \p x, reinterpreting the
 * bits of \p x for the new type \p V.
 *
 * This function is only applicable if:
 * - the \c sizeof of the input and output types is equal
 * - the Vector::size() of the input and output types is equal
 * - the \c VectorEntryTypes of input and output have equal \c sizeof
 *
 * \tparam V The requested type to change \p x into.
 * \param x The Vector to reinterpret as an object of type \p V.
 * \returns A new object (rvalue) of type \p V.
 *
 * \warning This cast is non-portable since the applicability (see above) may change
 * depending on the default vector types of the target platform. The function is perfectly
 * safe to use with fully specified \p Abi, though.
 */
template <typename V, typename T, typename Abi>
Vc_ALWAYS_INLINE Vc_CONST enable_if<
    (V::size() == Vector<T, Abi>::size() &&
     sizeof(typename V::VectorEntryType) ==
         sizeof(typename Vector<T, Abi>::VectorEntryType) &&
     sizeof(V) == sizeof(Vector<T, Abi>) && alignof(V) <= alignof(Vector<T, Abi>)),
    V>
reinterpret_components_cast(const Vector<T, Abi> &x)
{
    return reinterpret_cast<const V &>(x);
}

#define Vc_OP(symbol)                                                                    \
    template <typename T, typename Abi>                                                  \
    inline Vector<T, Abi> &operator symbol##=(Vector<T, Abi> &,                          \
                                              const Vector<T, Abi> &x);
    //Vc_ALL_ARITHMETICS(Vc_OP);
    //Vc_ALL_BINARY(Vc_OP);
    //Vc_ALL_SHIFTS(Vc_OP);
#undef Vc_OP

}  // namespace Vc

#endif  // VC_COMMON_VECTOR_H_

// vim: foldmethod=marker
