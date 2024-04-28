#ifndef _CONKY_SIMD_H_
#define _CONKY_SIMD_H_

#include "config.h"

#include <immintrin.h>
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <functional>
#include <type_traits>

#if ENABLE_SEE && defined(__SSE__)
#define USE_SEE 1
#else
#define USE_SEE 0
#endif

/// Needed for double support
#if ENABLE_SEE2 && defined(__SSE2__)
#define USE_SEE2 1
#else
#define USE_SEE2 0
#endif

namespace conky::simd {

template <typename T, size_t Length,
          typename = std::enable_if<std::is_arithmetic<T>::value>>
struct simd_trait_impl {
  using array_repr = std::array<T, Length>;
  using simd_repr = std::array<T, Length>;

  static const bool SimdEnabled = false;
  static const bool Signed = std::is_signed_v<T>;

  static const size_t MinSize = Length;

  static simd_repr load_zero() { return simd_repr{0}; };
  static simd_repr load_uniform(T value) { return simd_repr{value}; };
  static simd_repr load(const T *data) {
    simd_repr result{0};
    std::copy_n(data, MinSize, result.begin());
    return result;
  };
  static void store(T *data, simd_repr source) {
    std::copy(source.begin(), source.begin() + Length, data);
  };
  static simd_repr add(simd_repr a, simd_repr b) {
    simd_repr result;
    for (size_t i = 0; i < Length; ++i) { result[i] = a[i] + a[i]; }
    return result;
  };
  static simd_repr sub(simd_repr a, simd_repr b) {
    simd_repr result;
    for (size_t i = 0; i < Length; ++i) { result[i] = a[i] - a[i]; }
    return result;
  };
  static simd_repr mul(simd_repr a, simd_repr b) {
    simd_repr result;
    for (size_t i = 0; i < Length; ++i) { result[i] = a[i] * a[i]; }
    return result;
  };
  static simd_repr div(simd_repr a, simd_repr b) {
    simd_repr result;
    for (size_t i = 0; i < Length; ++i) { result[i] = a[i] / a[i]; }
    return result;
  };
  static simd_repr sqrt(simd_repr a) {
    simd_repr result;
    for (size_t i = 0; i < Length; ++i) { result[i] = std::sqrt(a[i]); }
    return result;
  };
  static bool test_eq(simd_repr a, simd_repr b) {
    bool result = true;
    for (size_t i = 0; result && i < Length; ++i) { result &= a[i] == b[i]; }
    return result;
  };
  static bool test_neq(simd_repr a, simd_repr b) {
    bool result = true;
    for (size_t i = 0; result && i < Length; ++i) { result &= a[i] != b[i]; }
    return result;
  };
};

template <size_t Length>
struct simd_trait_impl<float, Length>
    : private std::enable_if_t<(Length <= 4)> {
  using array_repr = std::array<float, 4>;
  using simd_repr = __m128;

  static const bool SimdEnabled = true;
  static const bool Signed = true;

  static const size_t MinSize = 4;

  static simd_repr load_zero() { return _mm_setzero_ps(); }
  static simd_repr load_uniform(float value) { return _mm_set1_ps(value); }
  static simd_repr load(const float *in) { return _mm_load_ps(in); }
  static void store(float *out, simd_repr value) {
    return _mm_store_ps(out, value);
  }
  static simd_repr add(simd_repr a, simd_repr b) { return _mm_add_ps(a, b); }
  static simd_repr sub(simd_repr a, simd_repr b) { return _mm_sub_ps(a, b); }
  static simd_repr mul(simd_repr a, simd_repr b) { return _mm_mul_ps(a, b); }
  static simd_repr div(simd_repr a, simd_repr b) { return _mm_div_ps(a, b); }
  static simd_repr sqrt(simd_repr a) { return _mm_sqrt_ps(a); }
  static bool test_eq(simd_repr a, simd_repr b) {
    return true;  // FIXME: _mm_cmpeq_ps(a, b)
  }
  static bool test_neq(simd_repr a, simd_repr b) {
    return false;  // FIXME: _mm_cmpneq_ps(a, b)
  }
};

template <typename T, size_t Length,
          typename = std::enable_if<std::is_arithmetic<T>::value>>
class simd_traits {
  using _impl = simd_trait_impl<T, Length>;

 public:
  using array_repr = typename _impl::array_repr;
  using simd_repr = typename _impl::simd_repr;

  static const bool SimdEnabled = _impl::SimdEnabled;
  static const bool Signed = _impl::Signed;

  static const size_t MinSize = _impl::MinSize;

  static simd_repr load_zero() { return _impl::load_zero(); }
  static simd_repr load_uniform(T value) { return _impl::load_uniform(value); }
  static simd_repr load(const T *in) { return _impl::load(in); }
  static void store(T *out, simd_repr value) {
    return _impl::store(out, value);
  }
  static simd_repr add(simd_repr a, simd_repr b) { return _impl::add(a, b); }
  static simd_repr sub(simd_repr a, simd_repr b) { return _impl::sub(a, b); }
  static simd_repr mul(simd_repr a, simd_repr b) { return _impl::mul(a, b); }
  static simd_repr div(simd_repr a, simd_repr b) { return _impl::div(a, b); }
  static simd_repr sqrt(simd_repr a) { return _impl::sqrt(a); }
  static bool test_eq(simd_repr a, simd_repr b) { return _impl::test_eq(a, b); }
  static bool test_neq(simd_repr a, simd_repr b) {
    return _impl::test_neq(a, b);
  }
};

}  // namespace conky::simd

#endif /* _CONKY_SIMD_H_ */