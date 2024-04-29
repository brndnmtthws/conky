/*
 * simd.h: traits for SIMD support
 *
 * Copyright 2023 Tin Švagelj <tin.svagelj@live.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the “Software”), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef _CONKY_SIMD_H_
#define _CONKY_SIMD_H_

#include "config.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <tuple>
#include <type_traits>
#include <variant>

#if ENABLE_SSE && __SSE__
#define USE_SSE 1
#endif

/// Needed for double support
#if ENABLE_SSE2 && __SSE2__
#define USE_SSE2 1
#endif

#ifdef USE_SSE
#include <immintrin.h>
#endif

namespace conky::simd {

/// 32-aligned std::array wrapper
template <typename T, size_t Length>
struct aligned_array {
  alignas(32) std::array<T, Length> value;

  constexpr aligned_array(const aligned_array &) = default;
  constexpr aligned_array(aligned_array &&) = default;

  constexpr aligned_array &operator=(const aligned_array &) = default;
  constexpr aligned_array &operator=(aligned_array &&) = default;
  constexpr std::array<T, Length> &operator=(
      const std::array<T, Length> &value) {
    value = value;
    return *this;
  }
  constexpr std::array<T, Length> &operator=(std::array<T, Length> &&value) {
    value = std::move(value);
    return *this;
  }

  constexpr std::array<T, Length>() { return this->value; };

  constexpr std::array<T, Length> &operator->() { return value; }
  constexpr const std::array<T, Length> &operator->() const { return value; }

  constexpr std::array<T, Length> &operator*() { return value; }
  constexpr const std::array<T, Length> &operator*() const { return value; }
};

/// Base type variables for SIMD operations.
template <typename T, size_t Length>
struct _base_types {
  using component = T;
  using array_repr = std::array<T, Length>;
  using array_repr_aligned = aligned_array<T, sizeof(array_repr) / sizeof(T)>;
  using simd_repr = array_repr_aligned;
};

/// Load and store operations
// template <typename T, size_t Length>
// struct _load_store : public _base_types<T, Length> {
//   static inline simd_repr load_zero() { return array_repr_aligned{0}; }
//   static inline simd_repr load_uniform(T value) {
//     return array_repr_aligned{value};
//   }
//   static inline simd_repr loadu(const T *data) {
//     array_repr_aligned result{0};
//     std::copy_n(data, sizeof(array_repr_aligned) / sizeof(T),
//     result.begin()); return result;
//   }
//   static inline simd_repr load(const T *data) {
//     array_repr_aligned result{0};
//     std::copy_n(data, sizeof(array_repr_aligned) / sizeof(T),
//     result.begin()); return result;
//   }
//   static inline simd_repr load(const array_repr_aligned &value) {
//     return load(value.data());
//   }
//   static inline void store(T *data, simd_repr source) {
//     std::copy(source.begin(), source.begin() + Length, data);
//   }

//   static inline array_repr to_array(simd_repr source) {
//     return reinterpret_cast<array_repr>(source);
//   }

//   static inline simd_repr from_array(const array_repr_aligned source) {
//     if constexpr (std::is_same_v(array_repr_aligned, simd_repr)) {
//       return source;
//     } else {
//       return load(source.data());
//     }
//   }
// };

enum class simd_op {
  // constructors
  LOAD_ZERO,
  LOAD_UNIFORM,
  LOADU,
  LOAD,
  STORE,
  TO_ARRAY,
  FROM_ARRAY,

  // binary operators
  ADD,
  SUB,
  MUL,
  DIV,
  EQ,
  NEQ,

  // unary operators
  SQRT,
  ABS,
  NEG,
};

/// Describes what an operation application consumes and returns for a given
/// `Base`, i.e. mapping requirements and result.
template <simd_op Op, typename Base>
struct _simd_op_info {
  using args;
  using return_ty;
};

template <typename Base>
struct _simd_op_info<simd_op::LOAD_ZERO, Base> {
  using args = std::tuple<>;
  using return_ty = Base::simd_repr;
};

template <typename Base>
struct _simd_op_info<simd_op::LOAD_UNIFORM, Base> {
  using args = std::tuple<Base::component>;
  using return_ty = Base::simd_repr;
};

template <typename Base>
struct _simd_op_info<simd_op::LOADU, Base> {
  using args = std::tuple<const Base::component *>;
  using return_ty = Base::simd_repr;
};

template <typename Base>
struct _simd_op_info<simd_op::LOAD, Base> {
  using args = std::tuple<const Base::component *>;
  using return_ty = Base::simd_repr;
};

template <typename Base>
struct _simd_op_info<simd_op::STORE, Base> {
  using args = std::tuple<Base::component *, Base::simd_repr>;
  using return_ty = void;
};

template <typename Base>
struct _simd_op_info<simd_op::TO_ARRAY, Base> {
  using args = std::tuple<Base::simd_repr>;
  using return_ty = Base::array_repr;
};

template <typename Base>
struct _simd_op_info<simd_op::FROM_ARRAY, Base> {
  using args = std::tuple<Base::array_repr>;
  using return_ty = Base::simd_repr;
};

#define DECL_BINARY_OP_INFO(Op, Result)                        \
  template <typename Base>                                     \
  struct _simd_op_info<simd_op::Op, Base> {                    \
    using args = std::tuple<Base::simd_repr, Base::simd_repr>; \
    using return_ty = Result;                                  \
  };

#define DECL_MAPPING_OP_INFO(Op)              \
  template <typename Base>                    \
  struct _simd_op_info<simd_op::Op, Base> {   \
    using args = std::tuple<Base::simd_repr>; \
    using return_ty = Base::simd_repr;        \
  };

DECL_BINARY_OP_INFO(ADD, Base::simd_repr)
DECL_BINARY_OP_INFO(SUB, Base::simd_repr)
DECL_BINARY_OP_INFO(MUL, Base::simd_repr)
DECL_BINARY_OP_INFO(DIV, Base::simd_repr)
DECL_BINARY_OP_INFO(EQ, bool)
DECL_BINARY_OP_INFO(NEQ, bool)

DECL_MAPPING_OP_INFO(SQRT)
DECL_MAPPING_OP_INFO(ABS)
DECL_MAPPING_OP_INFO(NEG)

#undef DECL_BINARY_OP_INFO
#undef DECL_MAPPING_OP_INFO

/// Applies `Op` operator on a SIMD type of `T`x`Length` packed values.
template <typename T, size_t Length, simd_op Op>
static inline typename _simd_op_info<Op, _base_types<T, Length>>::return_ty
apply_operator(
    typename _simd_op_info<Op, _base_types<T, Length>>::args &&args) {
  // missing specialization should always panic so we can be sure at least
  // fallback implementation with arrays is defined
  CRIT_ERR("not implemented");
}

// Now we define base implementations that assume they're recieving simd values,
// turn them into arrays and then perform operations on array elements. This
// allows partial specialization of behavior.

// Don't be confused by use of `simd_repr`, for base implementation it's equal
// to aligned std::array.

template <typename T, size_t Length>
static inline typename _base_types<T, Length>::simd_repr
apply_operator<T, Length, simd_op::LOADU>(
    std::tuple<const typename _base_types<T, Length>::component *> &&args) {
  auto ptr_first = std::get<0>(std::move(args));
  _base_types<T, Length>::simd_repr result;
  for (size_t i = 0; i < Length; ++i) { result[i] = ptr_first[i]; }
  return result;
}

template <typename T, size_t Length>
static inline typename _base_types<T, Length>::simd_repr
apply_operator<T, Length, simd_op::LOAD>(
    std::tuple<const typename _base_types<T, Length>::component *> &&args) {
  auto ptr_first = std::get<0>(std::move(args));
  _base_types<T, Length>::simd_repr result;
  for (size_t i = 0; i < Length; ++i) { result[i] = ptr_first[i]; }
  return result;
}

template <typename T, size_t Length>
static inline typename _base_types<T, Length>::simd_repr
apply_operator<T, Length, simd_op::LOAD_ZERO>(std::tuple<> &&args) {
  _base_types<T, Length>::array_repr_alligned value{0};
  return apply_operator<T, Length, simd_op::LOAD>(
      std::make_tuple(value.data()));
}

template <typename T, size_t Length>
static inline typename _base_types<T, Length>::simd_repr
apply_operator<T, Length, simd_op::LOAD_UNIFORM>(std::tuple<T> &&args) {
  _base_types<T, Length>::array_repr_alligned value{
      std::get<0>(std::move(args))};
  return apply_operator<T, Length, simd_op::LOAD>(
      std::make_tuple(value.data()));
}

template <typename T, size_t Length>
static inline void apply_operator<T, Length, simd_op::STORE>(
    std::tuple<typename _base_types<T, Length>::component *,
               typename _base_types<T, Length>::simd_repr> &&args) {
  auto prargs = std::move(args);
  auto target = std::get<0>(prargs);
  auto source = std::get<1>(prargs);
  _base_types<T, Length>::simd_repr result;
  for (size_t i = 0; i < Length; ++i) { result[i] = source[i]; }
  return result;
}

template <typename T, size_t Length>
static inline typename _base_types<T, Length>::array_repr
apply_operator<T, Length, simd_op::TO_ARRAY>(
    std::tuple<typename _base_types<T, Length>::simd_repr> &&args) {
  _base_types<T, Length>::simd_repr value = std::get<0>(std::move(args));
  _base_types<T, Length>::array_repr result;
  apply_operator<T, Length, simd_op::STORE>(
      std::make_tuple(result.data(), value));
  return result;
}

template <typename T, size_t Length>
static inline typename _base_types<T, Length>::simd_repr
apply_operator<T, Length, simd_op::FROM_ARRAY>(
    std::tuple<typename _base_types<T, Length>::array_repr> &&args) {
  _base_types<T, Length>::array_repr value = std::get<0>(std::move(args));
  return apply_operator<T, Length, simd_op::LOADU>(
      std::make_tuple(value.data()));
}

#define BINARY_OP_BASE_IMPL(Op, Symbol)                                     \
  template <typename T, size_t Length>                                      \
  class _simd_op_impl<T, Length, _simd_op::Op>                              \
      : public _load_store<T, Length> {                                     \
    using Args = std::tuple<simd_repr, simd_repr>;                          \
    using Return = simd_repr;                                               \
                                                                            \
   public:                                                                  \
    static inline Return operator()(const Args &args) {                     \
      auto a = to_array(std::get<0>(args));                                 \
      auto b = to_array(std::get<1>(args));                                 \
      array_repr result;                                                    \
      for (size_t i = 0; i < Length; ++i) { result[i] = a[i] Symbol b[i]; } \
      return from_array(result);                                            \
    }                                                                       \
  };

BINARY_OP_BASE_IMPL(ADD, +)
BINARY_OP_BASE_IMPL(SUB, -)
BINARY_OP_BASE_IMPL(MUL, *)
BINARY_OP_BASE_IMPL(DIV, /)

#undef BINARY_OP_BASE_IMPL

#define UNARY_OP_BASE_IMPL(Op, Apply)                                 \
  template <typename T, size_t Length>                                \
  class _simd_op_impl<T, Length, _simd_op::Op>                        \
      : public _load_store<T, Length> {                               \
    using Args = std::tuple<simd_repr>;                               \
    using Return = simd_repr;                                         \
                                                                      \
   public:                                                            \
    static inline Return operator()(const Args &args) {               \
      auto a = to_array(std::get<0>(args));                           \
      array_repr result;                                              \
      for (size_t i = 0; i < Length; ++i) { result[i] = Apply(a[i]) } \
      return from_array(result);                                      \
    }                                                                 \
  };

UNARY_OP_BASE_IMPL(SQRT, std::sqrt)
UNARY_OP_BASE_IMPL(ABS, std::abs)
template <typename T>
inline T neg(T &&value) {
  return -value;
}
UNARY_OP_BASE_IMPL(NEG, neg)

#undef UNARY_OP_BASE_IMPL

template <typename T, size_t Length>
class _simd_op_impl<T, Length, simd_op::EQ> : public _load_store<T, Length> {
  using Args = std::tuple<simd_repr, simd_repr>;
  using Return = bool;

 public:
  static inline Return operator()(const Args &args) {
    auto a = to_array(std::get<0>(args));
    auto b = to_array(std::get<1>(args));
    bool result = true;
    for (size_t i = 0; result && i < Length; ++i) { result &= a[i] == b[i]; }
    return result;
  }
};
template <typename T, size_t Length>
class _simd_op_impl<T, Length, _simd_op::NEQ> : public _load_store<T, Length> {
  using Args = std::tuple<simd_repr, simd_repr>;
  using Return = bool;

 public:
  static inline typename Return operator()(const Args &args) {
    auto a = to_array(std::get<0>(args));
    auto b = to_array(std::get<1>(args));
    bool result = true;
    for (size_t i = 0; result && i < Length; ++i) { result &= a[i] != b[i]; }
    return result;
  }
};

// All the code from here on out is inherently `unsafe` because it relies on
// architecture specific intrinsics.

template <typename T, size_t Length>
struct _simd_impl : public _load_store<T, Length> {
  static const bool SimdEnabled = false;
};

#define IMPL_UN_INTRINSIC(T, MinSize, MaxSize, Op, Intrinsic)              \
  template <size_t Length>                                                 \
  class _simd_op_impl<T, Length, _simd_op::Op>                             \
      : public _load_store<T, Length>,                                     \
        private std::enable_if_t<(Length > MinSize && Length <= MaxSize)>, \
  {                                                                        \
    using Args = std::tuple<simd_repr, simd_repr>;                         \
    using Return = simd_repr;                                              \
                                                                           \
   public:                                                                 \
    static inline typename Return operator()(const Args &args) const {     \
      return Intrinsic(std::get<0>(args));                                 \
    }                                                                      \
  };

#define IMPL_BIN_INTRINSIC(T, MinSize, MaxSize, Op, Intrinsic)             \
  template <size_t Length>                                                 \
  class _simd_op_impl<T, Length, _simd_op::Op>                             \
      : public _load_store<T, Length>,                                     \
        private std::enable_if_t<(Length > MinSize && Length <= MaxSize)>, \
  {                                                                        \
    using Args = std::tuple<simd_repr, simd_repr>;                         \
    using Return = simd_repr;                                              \
                                                                           \
   public:                                                                 \
    static inline Return operator()(const Args &args) const {              \
      return Intrinsic(std::get<0>(args), std::get<1>(args));              \
    }                                                                      \
  };

#ifdef USE_SSE

#if false
template <size_t Length>
struct _base_types<float, Length> : private std::enable_if_t<(Length <= 4)> {
  using array_repr = std::array<float, 4>;
  using array_repr_aligned = aligned_array<T, sizeof(array_repr) / sizeof(T)>;
  using simd_repr = __m128;
};

template <size_t Length>
struct _load_store<float, Length> : public _base_types<T, Length>,
                                    private std::enable_if_t<(Length <= 4)> {
  static simd_repr load_zero() { return _mm_setzero_ps(); }
  static simd_repr load_uniform(float value) { return _mm_set1_ps(value); }
  static simd_repr loadu(const float *in) { return _mm_loadu_ps(in); }
  static void store(float *out, simd_repr value) {
    return _mm_store_ps(out, value);
  }
};

IMPL_BIN_INTRINSIC(float, 0, 4, ADD, _mm_add_ps)
IMPL_BIN_INTRINSIC(float, 0, 4, SUB, _mm_sub_ps)
IMPL_BIN_INTRINSIC(float, 0, 4, MUL, _mm_mul_ps)
IMPL_BIN_INTRINSIC(float, 0, 4, DIV, _mm_div_ps)
IMPL_UN_INTRINSIC(float, 0, 4, SQRT, _mm_sqrt_ps)


template <size_t Length>
struct _base_types<uint16_t, Length> : private std::enable_if_t<(Length <= 8)> {
  using array_repr = std::array<uint16_t, 8>;
  using array_repr_aligned = aligned_array<T, sizeof(array_repr) / sizeof(T)>;
  using simd_repr = __m128i;
};

template <size_t Length>
struct _load_store<uint16_t, Length> : public _base_types<T, Length>,
                                       private std::enable_if_t<(Length <= 8)> {
  static simd_repr load_uniform(uint16_t value) {
    return _mm_set1_epi16(value);
  }
  static simd_repr loadu(const uint16_t *in) { return _mm_loadu_epi16(in); }
  static void store(uint16_t *out, simd_repr value) {
    return _mm_store_epi16(out, value);
  }
};

IMPL_BIN_INTRINSIC(uint16_t, 0, 8, ADD, _mm_add_epi16)
IMPL_BIN_INTRINSIC(uint16_t, 0, 8, SUB, _mm_sub_epi16)

template <size_t Length>
struct _base_types<int32_t, Length> : private std::enable_if_t<(Length <= 4)> {
  using array_repr = std::array<int32_t, 4>;
  using array_repr_aligned = aligned_array<T, sizeof(array_repr) / sizeof(T)>;
  using simd_repr = __m128i;
};

template <size_t Length>
struct _load_store<int32_t, Length> : public _base_types<T, Length>,
                                      private std::enable_if_t<(Length <= 4)> {
  static simd_repr load_zero() { return _mm_setzero_epi32(); }
  static simd_repr load_uniform(int32_t value) { return _mm_set1_epi32(value); }
  static simd_repr loadu(const int32_t *in) { return _mm_loadu_epi32(in); }
  static void store(int32_t *out, simd_repr value) {
    return _mm_store_epi32(out, value);
  }
};

IMPL_BIN_INTRINSIC(int32_t, 0, 4, ADD, _mm_add_epi32)
IMPL_BIN_INTRINSIC(int32_t, 0, 4, SUB, _mm_sub_epi32)
IMPL_BIN_INTRINSIC(int32_t, 0, 4, MUL, _mm_mul_epi32)

#endif

#endif /* USE_SSE */

#if false
#ifdef USE_SSE2

template <size_t Length>
struct _base_types<double, Length> : private std::enable_if_t<(Length <= 2)> {
  using array_repr = std::array<double, 2>;
  using array_repr_aligned = aligned_array<T, sizeof(array_repr) / sizeof(T)>;
  using simd_repr = __m128d;
};

template <size_t Length>
struct _load_store<double, Length> : public _base_types<T, Length>,
                                     private std::enable_if_t<(Length <= 2)> {
  static simd_repr load_zero() { return _mm_setzero_pd(); }
  static simd_repr load_uniform(double value) { return _mm_set1_pd(value); }
  static simd_repr loadu(const double *in) { return _mm_loadu_pd(in); }
  static void store(double *out, simd_repr value) {
    return _mm_store_pd(out, value);
  }
};

IMPL_BIN_INTRINSIC(double, 0, 2, ADD, _mm_add_pd)
IMPL_BIN_INTRINSIC(double, 0, 2, SUB, _mm_sub_pd)
IMPL_BIN_INTRINSIC(double, 0, 2, MUL, _mm_mul_pd)
IMPL_BIN_INTRINSIC(double, 0, 2, DIV, _mm_div_pd)

IMPL_UN_INTRINSIC(double, 0, 2, SQRT, _mm_sqrt_pd)

template <size_t Length>
struct _simd_impl<double, Length> : private std::enable_if_t<(Length <= 2)> {
  static const bool SimdEnabled = true;
};

#endif /* USE_SSE2 */
#endif

#undef IMPL_BIN_INTRINSIC
#undef IMPL_UN_INTRINSIC

template <typename T, size_t Length,
          typename = std::enable_if<std::is_arithmetic<T>::value>>
class simd_traits {
  using _impl = _simd_impl<T, Length>;

 public:
  using array_repr = typename _impl::array_repr;
  using array_repr_aligned = typename _impl::array_repr_aligned;
  using simd_repr = typename _impl::simd_repr;

  static const bool SimdEnabled = _impl::SimdEnabled;
  static const bool Signed = std::is_signed_v<T>;

  static const size_t MinSize = sizeof(_impl::array_repr) / sizeof(T);

  static inline simd_repr load_zero() { return _impl::load_zero(); }
  static inline simd_repr load_uniform(T value) {
    return _impl::load_uniform(value);
  }
  static inline simd_repr loadu(const T *in) { return _impl::loadu(in); }
  static inline simd_repr load(const T *in) { return _impl::load(in); }
  static inline simd_repr load(const array_repr_aligned &in) {
    return _impl::load(in.data());
  }
  static inline void store(T *out, simd_repr value) {
    return _impl::store(out, value);
  }

  static inline array_repr to_array(simd_repr source) {
    return _impl::to_array(source);
  }

  static inline simd_repr from_array(const array_repr_aligned source) {
    return _impl::from_array(source);
  }

  static inline simd_repr add(simd_repr a, simd_repr b) {
    return _simd_op_impl<T, Length, simd_op::ADD>(std::make_tuple(a, b));
  }
  static inline simd_repr sub(simd_repr a, simd_repr b) {
    return _simd_op_impl<T, Length, simd_op::SUB>(std::make_tuple(a, b));
  }
  static inline simd_repr mul(simd_repr a, simd_repr b) {
    return _simd_op_impl<T, Length, simd_op::MUL>(std::make_tuple(a, b));
  }
  static inline simd_repr div(simd_repr a, simd_repr b) {
    return _simd_op_impl<T, Length, simd_op::DIV>(std::make_tuple(a, b));
  }
  static inline simd_repr sqrt(simd_repr a) {
    return _simd_op_impl<T, Length, simd_op::SQRT>(std::make_tuple(a));
  }
  static inline simd_repr abs(simd_repr a) {
    return _simd_op_impl<T, Length, simd_op::ABS>(std::make_tuple(a));
  }
  static inline bool test_eq(simd_repr a, simd_repr b) {
    return _simd_op_impl<T, Length, simd_op::EQ>(std::make_tuple(a, b));
  }
  static inline bool test_neq(simd_repr a, simd_repr b) {
    return _simd_op_impl<T, Length, simd_op::NEQ>(std::make_tuple(a, b));
  }
};

}  // namespace conky::simd

#endif /* _CONKY_SIMD_H_ */
