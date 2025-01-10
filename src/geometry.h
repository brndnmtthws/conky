#ifndef _CONKY_GEOMETRY_H_
#define _CONKY_GEOMETRY_H_

#include "macros.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <numeric>
#include <type_traits>

#include <Vc/Vc>

#ifdef BUILD_X11
#include <X11/Xlib.h>
#endif /* BUILD_X11 */

namespace conky {
namespace _priv_geom {
/// @brief Constructs an index assignable type from an array with different
/// value types using `static_cast`.
///
/// Panics at runtime if `O` can be indexed by every value in range `0` to
/// (excluding) `Length`.
///
/// @tparam Input source array element
/// @tparam R target element type
/// @tparam Output target container
/// @tparam Length array length
/// @param value array to pad/trim
/// @return casted array
template <typename Input, typename R, size_t Length, typename Output>
inline Output cast_to_assignable(const Input &value, Output &target) {
  if constexpr (std::is_same_v<Input, Output>) { return value; }
  for (size_t i = 0; i < Length; i++) {
    target[i] = static_cast<R>(value.at(i));
  }
  return target;
}

/// @brief Constructs a casted array from an array with different value types
/// using `static_cast`.
///
/// @tparam T source array element
/// @tparam R target array element
/// @tparam Length array length
/// @param value array to pad/trim
/// @return casted array
template <typename T, typename R, size_t Length>
inline std::array<R, Length> cast_array(const std::array<T, Length> &value) {
  static_assert(std::is_convertible_v<T, R>, "T is not convertible to R");
  if constexpr (std::is_same_v<T, R>) { return value; }
  std::array<R, Length> result;
  cast_to_assignable<std::array<T, Length>, R, Length, std::array<R, Length>>(
      value, result);
  return result;
}

/// @brief Takes an array and pads/trims it to the desired length by inserting
/// zeros or discarding unwanted elements
/// @tparam T array element
/// @tparam Source source array length
/// @tparam Target target array length
/// @param value array to pad/trim
/// @return padded/trimmed array
template <typename T, size_t Source, size_t Target>
inline std::array<T, Target> resize_array(std::array<T, Source> value) {
  if constexpr (Source == Target) { return value; }
  auto buff = std::array<T, Target>{0};
  std::copy_n(value.begin(), std::min(Source, Target), buff.begin());
  return buff;
}
}  // namespace _priv_geom

/// @brief A 2D vector representation.
///
/// Uses eigen under to hood to provide SIMD optimizations on appropriate
/// platforms if available. Doesn't expose any of the eigen functionality
/// directly to avoid pollution.
///
/// @tparam T numeric component type.
template <typename T, size_t Length>
struct vec {
  static_assert(std::is_arithmetic_v<T>, "T must be a number");
  static_assert(Length >= 2, "Length must be greater than 2");

  /// @brief Compile time component type information.
  using Component = T;

 private:
  /// Sealed type used to disable constructor selection.
  class _disabled {};

 protected:
  using Data = Vc::array<T, Length>;

  Data value;

 public:
  vec() : value(Data{0}) {}
  vec(const Data &value) : value(value) {}
  vec(std::array<T, Length> array) {
    _priv_geom::cast_to_assignable<std::array<T, Length>, T, Length,
                                   Vc::array<T, Length>>(array, this->value);
  }

  vec(const vec<T, Length> &other) : vec(other.value) {}

  vec(T x, T y) : vec<T, 2>(std::array<T, 2>{x, y}) {
    static_assert(Length == 2, "constructor only valid for vec2<T>");
  }
  vec(T x, T y, T z) : vec<T, 3>(std::array<T, 3>{x, y, z, 0}) {
    static_assert(Length == 3, "constructor only valid for vec3<T>");
  }
  vec(T x, T y, T z, T w) : vec<T, 4>(std::array<T, 4>{x, y, z, w}) {
    static_assert(Length == 4, "constructor only valid for vec4<T>");
  }

  vec(vec<T, Length> &&other) { this->value = other->value; }

  static inline vec<T, Length> uniform(T v) {
    std::array<T, Length> data;
    data.fill(v);
    return vec<T, Length>(data);
  }

  /// @brief Returns vec component at `index`.
  /// @param index component index.
  /// @return component at `index` of this vec.
  T at(size_t index) const { return static_cast<T>(this->value[index]); }

  /// @brief vec x component.
  /// @return x value of this vec.
  inline T x() const { return this->at(0); }
  /// @brief vec y component.
  /// @return y value of this vec.
  inline T y() const { return this->at(1); }
  /// @brief vec z component.
  /// @return z value of this vec.
  inline T z() const {
    static_assert(Length >= 3, "vector doesn't have a z component");
    return this->at(2);
  }
  /// @brief vec w component.
  /// @return w value of this vec.
  inline T w() const {
    static_assert(Length >= 4, "vector doesn't have a w component");
    return this->at(3);
  }

  void set(size_t index, T value) { this->value[index] = value; }

  inline void set_x(T new_value) { this->set(0, new_value); }
  inline void set_y(T new_value) { this->set(1, new_value); }
  inline void set_z(T new_value) {
    static_assert(Length >= 3, "vector doesn't have a z component");
    this->set(2, new_value);
  }
  inline void set_w(T new_value) {
    static_assert(Length >= 4, "vector doesn't have a w component");
    this->set(3, new_value);
  }

  vec<T, Length> &operator=(const vec<T, Length> &other) {
    this->value = other.value;
    return *this;
  }
  vec<T, Length> &operator=(vec<T, Length> &&other) {
    this->value = std::move(other.value);
    return *this;
  }
  template <typename O = T>
  vec<T, Length> &operator=(std::array<O, Length> other) {
    _priv_geom::cast_to_assignable(other, this->value);
    return *this;
  }

  inline T operator[](size_t index) { return this->at(index); }

  /// @brief Zero vector value.
  static vec<T, Length> Zero() { return vec<T, Length>::uniform(0); }
  /// @brief Unit vector value.
  static vec<T, Length> One() { return vec<T, Length>::uniform(1); }
  /// @brief X unit vector value.
  static vec<T, Length> UnitX() {
    Data buffer{0};
    buffer[0] = static_cast<T>(1);
    return vec(buffer);
  }
  /// @brief Y unit vector value.
  static vec<T, Length> UnitY() {
    Data buffer{0};
    buffer[1] = static_cast<T>(1);
    return vec(buffer);
  }
  /// @brief Z unit vector value.
  static vec<T, Length> UnitZ() {
    static_assert(Length >= 3, "vector doesn't have a z component");
    Data buffer{0};
    buffer[2] = static_cast<T>(1);
    return vec(buffer);
  }
  /// @brief W unit vector value.
  static vec<T, Length> UnitW() {
    static_assert(Length >= 4, "vector doesn't have a w component");
    Data buffer{0};
    buffer[3] = static_cast<T>(1);
    return vec(buffer);
  }

  // NOTE: All of the following loops will get unrolled by the compiler because
  // Length is a constant expression (unless Length is very large).

  inline vec<T, Length> operator+(vec<T, Length> other) const {
    Data result{this->value};
    for (size_t i = 0; i < Length; i++) { result[i] += other.value[i]; }
    return vec<T, Length>(result);
  }
  inline vec<T, Length> &operator+=(vec<T, Length> other) {
    for (size_t i = 0; i < Length; i++) { this->value[i] += other.value[i]; }
    return *this;
  }
  inline vec<T, Length> operator-(vec<T, Length> other) const {
    Data result{this->value};
    for (size_t i = 0; i < Length; i++) { result[i] -= other.value[i]; }
    return vec<T, Length>(result);
  }
  inline vec<T, Length> &operator-=(vec<T, Length> other) {
    for (size_t i = 0; i < Length; i++) { this->value[i] -= other.value[i]; }
    return *this;
  }
  inline vec<T, Length> operator*(T scalar) const {
    Data result{this->value};
    for (size_t i = 0; i < Length; i++) { result[i] *= scalar; }
    return vec<T, Length>(result);
  }
  inline vec<T, Length> &operator*=(T scalar) {
    for (size_t i = 0; i < Length; i++) { this->value[i] *= scalar; }
    return *this;
  }
  inline vec<T, Length> operator*(vec<T, Length> other) const {
    Data result{this->value};
    for (size_t i = 0; i < Length; i++) { result[i] *= other.value[i]; }
    return vec<T, Length>(result);
  }
  inline vec<T, Length> &operator*=(vec<T, Length> other) {
    for (size_t i = 0; i < Length; i++) { this->value[i] *= other.value[i]; }
    return *this;
  }
  inline vec<T, Length> operator/(T scalar) const {
    Data result{this->value};
    for (size_t i = 0; i < Length; i++) { result[i] /= scalar; }
    return vec<T, Length>(result);
  }
  inline vec<T, Length> &operator/=(T scalar) {
    for (size_t i = 0; i < Length; i++) { this->value[i] /= scalar; }
    return *this;
  }
  inline vec<T, Length> operator/(vec<T, Length> other) const {
    Data result{this->value};
    for (size_t i = 0; i < Length; i++) { result[i] /= other.value[i]; }
    return vec<T, Length>(result);
  }
  inline vec<T, Length> &operator/=(vec<T, Length> other) {
    for (size_t i = 0; i < Length; i++) { this->value[i] /= other.value[i]; }
    return *this;
  }

  inline bool operator==(vec<T, Length> other) const {
    for (size_t i = 0; i < Length; i++) {
      if (this->value[i] != other.value[i]) { return false; }
    }
    return true;
  }
  inline bool operator!=(vec<T, Length> other) const {
    return !(*this == other);
  }

  inline vec<T, Length> operator-() const {
    if constexpr (std::is_signed_v<T>) {
      return *this * Data{-1};
    } else {
      return Data{std::numeric_limits<T>::max()} - *this;
    }
  }
  inline vec<T, Length> abs() const {
    if constexpr (std::is_signed_v<T>) {
      Data result;
      std::transform(this->value.begin(), this->value.end(), result.begin(),
                     std::abs);
      return vec<T, Length>(result);
    } else {
      return *this;
    }
  }

  /// @brief Computes component-wise vector minimum
  /// @param other other vector
  /// @returns new vector with min values
  inline vec<T, Length> min(const vec<T, Length> &other) {
    Data result{this->value};
    for (size_t i = 0; i < Length; i++) {
      result[i] = std::min(result[i], other.value[i]);
    }
    return vec<T, Length>(result);
  }

  /// @brief Computes component-wise vector maximum
  /// @param other other vector
  /// @returns new vector with max values
  inline vec<T, Length> max(const vec<T, Length> &other) {
    Data result{this->value};
    for (size_t i = 0; i < Length; i++) {
      result[i] = std::max(result[i], other.value[i]);
    }
    return vec<T, Length>(result);
  }

  inline T distance_squared(vec<T, Length> other) const {
    vec<T, Length> buffer = other - *this;
    buffer *= buffer;
    return std::accumulate(buffer->value.begin(), buffer->value.end(), T{0});
  }
  inline T distance(vec<T, Length> &other) const {
    return std::sqrt(this->distance_squared(other));
  }
  inline T magnitude_squared() const {
    vec<T, Length> buffer = this->value * this->value;
    return std::accumulate(buffer->value.begin(), buffer->value.end(), T{0});
  }
  inline T magnitude() const { return std::sqrt(this->magnitude_squared()); }

  T surface() const {
    static_assert(Length == 2, "surface computable only for 2D vectors");
    return this->x() * this->y();
  }

  template <typename O>
  vec<O, Length> cast() const {
    Vc::array<O, Length> buffer;
    _priv_geom::cast_to_assignable<Vc::array<T, Length>, O, Length,
                                   Vc::array<O, Length>>(this->value, buffer);
    return vec<O, Length>(buffer);
  }

  std::array<T, Length> to_array() const {
    std::array<T, Length> result;
    for (size_t i = 0; i < Length; ++i) { result[i] = this->value[i]; }
    return result;
  }

  template <typename O>
  operator vec<O, Length>() const {
    return this->cast<O>();
  }
  operator std::array<T, Length>() const { return this->to_array(); }
};

template <typename T>
using vec2 = vec<T, 2>;
using vec2f = vec2<float>;
using vec2d = vec2<double>;
using vec2i = vec2<std::int32_t>;
template <typename T>
using vec3 = vec<T, 3>;
using vec3f = vec3<float>;
using vec3d = vec3<double>;
using vec3i = vec3<std::int32_t>;
template <typename T>
using vec4 = vec<T, 4>;
using vec4f = vec4<float>;
using vec4d = vec4<double>;
using vec4i = vec4<std::int32_t>;

enum class rect_kind {
  SIZED,
  ABSOLUTE,
};

/// @brief 2D rectangle representation using position and size vectors.
/// @tparam T component number type.
template <typename T = std::int32_t, rect_kind Kind = rect_kind::SIZED>
struct rect {
  static_assert(std::is_arithmetic_v<T>, "T must be a number");

  using Component = T;

 private:
  vec2<T> m_pos;
  vec2<T> m_other;

 public:
  rect() : m_pos(vec2<T>::Zero()), m_other(vec2<T>::Zero()) {}
  rect(vec2<T> pos, vec2<T> other) : m_pos(pos), m_other(other) {}

  /// @brief Rectangle x position.
  /// @return x position of this rectangle.
  inline T x() const { return this->m_pos.x(); }
  /// @brief Rectangle y position.
  /// @return y position of this rectangle.
  inline T y() const { return this->m_pos.y(); }

  inline vec2<T> pos() const { return this->m_pos; }
  inline vec2<T> size() const {
    if constexpr (Kind == rect_kind::SIZED) {
      return this->m_other;
    } else {
      return this->m_other - this->m_pos;
    }
  }
  inline vec2<T> end_pos() const {
    if constexpr (Kind == rect_kind::SIZED) {
      return this->m_pos + this->m_other;
    } else {
      return this->m_other;
    }
  }

  /// @brief Rectangle end x position.
  /// @return ending x position of this rectangle.
  inline T end_x() const { return this->end_pos().x(); }
  /// @brief Rectangle end y position.
  /// @return ending y position of this rectangle.
  inline T end_y() const { return this->end_pos().y(); }

  /// @brief Rectangle width.
  /// @return width of this rectangle.
  inline T width() const {
    return size().x();
  }

  /// @brief Rectangle height.
  /// @return height of this rectangle.
  inline T height() const {
    return size().y();
  }

  /// @brief Returns rectangle component at `index`.
  /// @param index component index.
  /// @return component at `index` of this rectangle.
  T at(size_t index) const {
    assert_print(index < static_cast<size_t>(4), "index out of bounds");
    switch (index) {
      case 0:
        return this->m_pos.x();
      case 1:
        return this->m_pos.y();
      case 2:
        return this->m_other.x();
      case 3:
        return this->m_other.y();
      default:
        UNREACHABLE();
    }
  }

  inline void set_pos(vec2<T> value) { this->m_pos = value; }
  inline void set_pos(T x, T y) { this->set_pos(vec2<T>(x, y)); }
  inline void set_size(vec2<T> value) {
    if constexpr (Kind == rect_kind::SIZED) {
      this->m_other = value;
    } else {
      this->m_other = this->m_pos + value;
    }
  }
  inline void set_size(T width, T height) {
    this->set_size(vec2<T>(width, height));
  }
  inline void set_end_pos(vec2<T> value) {
    if constexpr (Kind == rect_kind::SIZED) {
      this->m_other = value - this->m_pos;
    } else {
      this->m_other = value;
    }
  }
  inline void set_end_pos(T x, T y) { this->set_end_pos(vec2<T>(x, y)); }

  inline void set_x(T value) { this->m_pos.set_x(value); }
  inline void set_y(T value) { this->m_pos.set_y(value); }

  inline void set_width(T value) {
    if constexpr (Kind == rect_kind::SIZED) {
      this->m_other.set_x(value);
    } else {
      this->m_other.set_x(this->m_pos.get_x() + value);
    }
  }
  inline void set_height(T value) {
    if constexpr (Kind == rect_kind::SIZED) {
      this->m_other.set_y(value);
    } else {
      this->m_other.set_y(this->m_pos.get_y() + value);
    }
  }
  inline void set_end_x(T value) {
    if constexpr (Kind == rect_kind::SIZED) {
      this->m_other.set_x(value - this->m_pos.get_x());
    } else {
      this->m_other.set_x(value);
    }
  }
  inline void set_end_y(T value) {
    if constexpr (Kind == rect_kind::SIZED) {
      this->m_other.set_y(value - this->m_pos.get_y());
    } else {
      this->m_other.set_y(value);
    }
  }

  void set(size_t index, T value) {
    assert_print(index < static_cast<size_t>(4), "index out of bounds");
    switch (index) {
      case 0:
        return this->m_pos.set_x(value);
      case 1:
        return this->m_pos.set_y(value);
      case 2:
        return this->m_other.set_x(value);
      case 3:
        return this->m_other.set_y(value);
      default:
        UNREACHABLE();
    }
  }

  std::array<vec2<T>, 4> corners() const {
    return std::array<vec2<T>, 4>{
        this->m_pos,
        this->m_pos + vec2<T>(this->width(), 0),
        this->end_pos(),
        this->m_pos + vec2<T>(0, this->height()),
    };
  }

  template <typename O = T>
  bool contains(vec2<O> p) const {
    return p.x() >= this->x() && p.x() < this->x() + this->width() &&
           p.y() >= this->y() && p.y() < this->y() + this->height();
  }

  template <typename O = T>
  bool contains(rect<O> other) const {
    return contains(other.m_pos) &&
           contains(other.m_pos + vec2<O>(other.width(), 0)) &&
           contains(other.m_pos + other.m_other) &&
           contains(other.m_pos + vec2<O>(0, other.height()));
  }

 private:
  template <typename O = T>
  bool _intersects_partial(rect<O> other) const {
    return contains(other.m_pos) ||
           contains(other.m_pos + vec2<O>(other.width(), 0)) ||
           contains(other.m_pos + other.m_other) ||
           contains(other.m_pos + vec2<O>(0, other.height()));
  }

 public:
  template <typename O = T>
  bool intersects(rect<O> other) const {
    return this->_intersects_partial(m_other) ||
           other._intersects_partial(*this);
  }

  rect<T, Kind> &operator=(const rect<T, Kind> &other) {
    this->m_pos = other.m_pos;
    this->m_other = other.m_other;
    return *this;
  }
  rect<T, Kind> &operator=(rect<T, Kind> &&other) {
    this->m_pos = other.m_pos;
    this->m_other = other.m_other;
    return *this;
  }
  template <typename O = T>
  rect<T, Kind> &operator=(std::array<O, 4> other) {
    _priv_geom::cast_to_assignable(other.m_pos, this->m_pos);
    _priv_geom::cast_to_assignable(other.m_other, this->m_other);
    return *this;
  }

  inline T operator[](size_t index) {
    if (index < 2) {
      return this->m_pos[index];
    } else if (index < 4) {
      return this->m_other[index - 2];
    } else {
      CRIT_ERR("index out of bounds");
    }
  }

#ifdef BUILD_X11
  XRectangle to_xrectangle() const {
    return XRectangle{static_cast<short>(this->x()),
                      static_cast<short>(this->y()),
                      static_cast<unsigned short>(this->width()),
                      static_cast<unsigned short>(this->height())};
  }
#endif /* BUILD_X11 */

  rect<T, rect_kind::SIZED> to_sized() const {
    if constexpr (Kind == rect_kind::SIZED) {
      return *this;
    } else {
      return rect<T, rect_kind::SIZED>{this->m_pos,
                                       this->m_other - this->m_pos};
    }
  }

  rect<T, rect_kind::ABSOLUTE> to_absolute() const {
    if constexpr (Kind == rect_kind::ABSOLUTE) {
      return *this;
    } else {
      return rect<T, rect_kind::ABSOLUTE>{this->m_pos,
                                          this->m_pos + this->m_other};
    }
  }

  std::array<T, 4> to_array() const {
    return std::array<T, 4>{this->m_pos.x(), this->m_pos.y(), this->m_other.x(),
                            this->m_other.y()};
  }

  inline operator std::array<T, 4>() const { return this->to_array(); }
  inline explicit operator rect<T, rect_kind::SIZED>() const {
    return this->to_sized();
  }
  inline explicit operator rect<T, rect_kind::ABSOLUTE>() const {
    return this->to_absolute();
  }
};

template <typename T>
using sized_rect = rect<T>;

template <typename T>
using absolute_rect = rect<T, rect_kind::ABSOLUTE>;

}  // namespace conky

#endif /* _CONKY_GEOMETRY_H_ */
