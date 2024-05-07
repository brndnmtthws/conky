#ifndef _CONKY_GEOMETRY_H_
#define _CONKY_GEOMETRY_H_

#include "config.h"

#include "macros.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <numeric>
#include <type_traits>

#include <Vc/Vc>

#ifdef BUILD_X11
#include <X11/Xlib.h>
#endif /* BUILD_X11 */

namespace conky {
namespace _priv_geom {
/// @brief Member access wrapper for containers of values.
///
/// Emulates member access to abstract away the fact that components might not
/// be discrete `T` types.
///
/// This is implementation detail for `geometry.h` and not intended to be used
/// elsewhere.
template <typename Container, typename T>
struct _member_access {
  Container *value;
  size_t index;

  constexpr _member_access(Container *value, size_t index)
      : value(value), index(index) {}

  /// @brief Converts `_member_access` to `T`.
  inline T operator*() const { return this->value->at(this->index); }

  /// @brief Assignment handler for `new_value` to `T` at `index`.
  inline const _member_access &operator=(T new_value) const {
    this->value->set(this->index, new_value);
    return *this;
  }

#define ASSIGN_OP_IMPL(assign_op_name, application)                   \
  inline T assign_op_name(T other) const {                            \
    this->value->set(this->index,                                     \
                     this->value->at(this->index) application other); \
    return *this;                                                     \
  }
  ASSIGN_OP_IMPL(operator+=, +)
  ASSIGN_OP_IMPL(operator-=, -)
  ASSIGN_OP_IMPL(operator*=, *)
  ASSIGN_OP_IMPL(operator/=, /)
#undef ASSIGN_OP_IMPL

  /// @brief Converts `_member_access` to `T`.
  inline operator T() const { return this->value->at(this->index); }
};

/// @brief Takes an array and pads/trims it to the desired length by inserting
/// zeros or discarding unwanted elements
/// @tparam T source array element
/// @tparam R target array element
/// @tparam Length array length
/// @param value array to pad/trim
/// @return padded/trimmed array
template <typename T, typename R, size_t Length>
std::array<R, Length> cast_array(std::array<T, Length> value) {
  static_assert(std::is_convertible_v<T, R>, "T is not convertible to R");
  if constexpr (std::is_same_v<T, R>) { return value; }
  std::array<R, Length> buff;
  std::copy_n(value.begin(), Length, buff.begin());
  return buff;
}

/// @brief Takes an array and pads/trims it to the desired length by inserting
/// zeros or discarding unwanted elements
/// @tparam T array element
/// @tparam Source source array length
/// @tparam Target target array length
/// @param value array to pad/trim
/// @return padded/trimmed array
template <typename T, size_t Source, size_t Target>
std::array<T, Target> resize_array(std::array<T, Source> value) {
  if constexpr (Source == Target) { return value; }
  auto buff = std::array<T, Target>{0};
  std::copy_n(value.begin(), std::min(Source, Target), buff.begin());
  return buff;
}

class _no_z_member {};
template <typename Of, typename T>
struct _has_z_member {
  static_assert(std::is_arithmetic_v<T>,
                "vector z member type (T) must be a number");
  using ComponentRef = _member_access<Of, T>;

  /// @brief vec z value.
  ///
  /// Treat it as `T`. Use `*z` in case type coertion is needed.
  const ComponentRef z = ComponentRef(this, 2);
};

class _no_w_member {};
template <typename Of, typename T>
struct _has_w_member {
  static_assert(std::is_arithmetic_v<T>,
                "vector w member type (T) must be a number");
  using ComponentRef = _member_access<Of, T>;

  /// @brief vec w value.
  ///
  /// Treat it as `T`. Use `*w` in case type coertion is needed.
  const ComponentRef w = ComponentRef(this, 3);
};

}  // namespace _priv_geom

/// @brief A 2D vector representation.
///
/// Uses eigen under to hood to provide SIMD optimizations on appropriate
/// platforms if available. Doesn't expose any of the eigen functionality
/// directly to avoid pollution.
///
/// @tparam T numeric component type.
template <typename T, size_t Length>
struct vec
    // conditionally add z member accessor
    : public std::conditional_t<(Length >= 3),
                                _priv_geom::_has_z_member<vec<T, Length>, T>,
                                _priv_geom::_no_z_member>,
      // conditionally add w member accessor
      public std::conditional_t<(Length >= 4),
                                _priv_geom::_has_w_member<vec<T, Length>, T>,
                                _priv_geom::_no_w_member> {
  static_assert(std::is_arithmetic_v<T>, "T must be a number");
  static_assert(Length >= 2, "Length must be greater than 2");

  /// @brief Compile time component type information.
  using Component = T;

 private:
  /// Sealed type used to disable constructor selection.
  class _disabled {};

 protected:
  using ComponentRef = _priv_geom::_member_access<vec<T, Length>, T>;
  using Data = Vc::array<T, Length>;

  Data value;

 public:
  vec() : value(Data{0}) {}
  vec(Data value) : value(value) {}
  vec(std::array<T, Length> array) {
    Vc::array<T, Length> vc_array;
    for (size_t i = 0; i < Length; ++i) { vc_array[i] = array[i]; }
    this->value = vc_array;
  }

  template <typename O = T>
  vec(vec<O, Length> other)
      : vec(_priv_geom::cast_array<O, T, Length>(other.to_array())) {}

  vec(T x, T y) : vec<T, 2>(std::array<T, 2>{x, y}) {
    static_assert(Length == 2, "constructor only valid for vec2<T>");
  }
  vec(T x, T y, T z) : vec<T, 3>(std::array<T, 3>{x, y, z, 0}) {
    static_assert(Length == 3, "constructor only valid for vec3<T>");
  }
  vec(T x, T y, T z, T w) : vec<T, 4>(std::array<T, 4>{x, y, z, w}) {
    static_assert(Length == 4, "constructor only valid for vec4<T>");
  }

  static inline vec<T, Length> uniform(T x) {
    return vec<T, Length>(std::array<T, Length>{x});
  }

  std::array<T, Length> to_array() const {
    std::array<T, Length> result;
    for (size_t i = 0; i < Length; ++i) { result[i] = this->value[i]; }
    return result;
  }

  /// @brief Returns vec component at `index`.
  /// @param index component index.
  /// @return component at `index` of this vec.
  T at(size_t index) const { return static_cast<T>(this->value[index]); }

  /// @brief vec x component.
  /// @return x value of this vec.
  inline T get_x() const { return this->at(0); }
  /// @brief vec y component.
  /// @return y value of this vec.
  inline T get_y() const { return this->at(1); }
  /// @brief vec z component.
  /// @return z value of this vec.
  inline T get_z() const {
    static_assert(Length >= 3, "vector doesn't have a z component");
    return this->at(2);
  }
  /// @brief vec w component.
  /// @return w value of this vec.
  inline T get_w() const {
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

  /// @brief vec x value.
  ///
  /// Treat it as `T`. Use `*x` in case type coertion is needed.
  const ComponentRef x = ComponentRef(this, 0);
  /// @brief vec y value.
  ///
  /// Treat it as `T`. Use `*y` in case type coertion is needed.
  const ComponentRef y = ComponentRef(this, 1);

  // z & w are conditionally defined by _has_z_member and _has_w_member in
  // _priv_geom because member templates aren't allowed.

  vec<T, Length> &operator=(vec<T, Length> other) {
    this->value = other.value;
    return *this;
  }
  template <typename O = T>
  vec<T, Length> &operator=(std::array<O, Length> other) {
    Data buffer = _priv_geom::cast_array(other);
    this->value = buffer;
    return *this;
  }

  inline const ComponentRef &operator[](size_t index) {
    assert_print(index < std::min(Length, static_cast<size_t>(4)),
                 "index out of bounds");
    switch (index) {
      case 0:
        return this->x;
      case 1:
        return this->y;
      case 2:
        return this->z;
      case 3:
        return this->w;
      default:
        UNREACHABLE();
    }
  }

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
    return this->get_x() * this->get_y();
  }
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

/// @brief 2D rectangle representation using position and size vectors.
/// @tparam T component number type.
template <typename T = std::int32_t>
struct rect {
  static_assert(std::is_arithmetic_v<T>, "T must be a number");

  using Component = T;
  using ComponentRef = _priv_geom::_member_access<rect<T>, T>;

  vec2<T> pos;
  vec2<T> size;

  rect() : pos(vec2<T>::Zero()), size(vec2<T>::Zero()) {}
  rect(vec2<T> pos, vec2<T> size) : pos(pos), size(size) {}

  /// @brief Rectangle x position.
  /// @return x position of this rectangle.
  T get_x() const { return this->pos.get_x(); }
  /// @brief Rectangle y position.
  /// @return y position of this rectangle.
  T get_y() const { return this->pos.get_y(); }
  /// @brief Rectangle width.
  /// @return width of this rectangle.
  T get_width() const { return this->size.get_x(); }
  /// @brief Rectangle height.
  /// @return height of this rectangle.
  T get_height() const { return this->size.get_y(); }

  /// @brief Returns rectangle component at `index`.
  /// @param index component index.
  /// @return component at `index` of this rectangle.
  T at(size_t index) const {
    assert_print(index < static_cast<size_t>(4), "index out of bounds");
    switch (index) {
      case 0:
        return this->get_x();
      case 1:
        return this->get_y();
      case 2:
        return this->get_width();
      case 3:
        return this->get_height();
      default:
        UNREACHABLE();
    }
  }

  void set_x(T value) { this->pos.set_x(value); }
  void set_y(T value) { this->pos.set_y(value); }
  void set_width(T value) { this->size.set_x(value); }
  void set_height(T value) { this->size.set_y(value); }

  void set(size_t index, T value) {
    assert_print(index < static_cast<size_t>(4), "index out of bounds");
    switch (index) {
      case 0:
        return this->set_x(value);
      case 1:
        return this->set_y(value);
      case 2:
        return this->set_width(value);
      case 3:
        return this->set_height(value);
      default:
        UNREACHABLE();
    }
  }

  /// @brief rectangle position x value.
  ///
  /// Treat it as `T`. Use `*x` in case type coertion is needed.
  const ComponentRef x = ComponentRef(this, 0);
  /// @brief rectangle position y value.
  ///
  /// Treat it as `T`. Use `*y` in case type coertion is needed.
  const ComponentRef y = ComponentRef(this, 1);
  /// @brief rectangle width value.
  ///
  /// Treat it as `T`. Use `*width` in case type coertion is needed.
  const ComponentRef width = ComponentRef(this, 2);
  /// @brief rectangle height value.
  ///
  /// Treat it as `T`. Use `*height` in case type coertion is needed.
  const ComponentRef height = ComponentRef(this, 3);

  std::array<vec2<T>, 4> corners() const {
    return std::array<vec2<T>, 4>{
        this->pos,
        this->pos + vec2<T>(this->get_width(), 0),
        this->pos + this->size,
        this->pos + vec2<T>(0, this->get_height()),
    };
  }

  template <typename O = T>
  bool contains(vec2<O> p) const {
    return p.get_x() >= this->get_x() &&
           p.get_x() < this->get_x() + this->get_width() &&
           p.get_y() >= this->get_y() &&
           p.get_y() < this->get_y() + this->get_height();
  }

  template <typename O = T>
  bool contains(rect<O> other) const {
    return contains(other.pos) &&
           contains(other.pos + vec2<O>(other.get_width(), 0)) &&
           contains(other.pos + other.size) &&
           contains(other.pos + vec2<O>(0, other.get_height()));
  }

 private:
  template <typename O = T>
  bool _intersects_partial(rect<O> other) const {
    return contains(other.pos) ||
           contains(other.pos + vec2<O>(other.get_width(), 0)) ||
           contains(other.pos + other.size) ||
           contains(other.pos + vec2<O>(0, other.get_height()));
  }

 public:
  template <typename O = T>
  bool intersects(rect<O> other) const {
    return this->_intersects_partial(other) || other._intersects_partial(*this);
  }

  constexpr const ComponentRef &operator[](size_t index) {
    assert_print(index < static_cast<size_t>(4), "index out of bounds");
    switch (index) {
      case 0:
        return this->x;
      case 1:
        return this->y;
      case 2:
        return this->width;
      case 3:
        return this->height;
      default:
        UNREACHABLE();
    }
  }

#ifdef BUILD_X11
  XRectangle to_xrectangle() const {
    return XRectangle{
        .x = static_cast<short>(this->get_x()),
        .y = static_cast<short>(this->get_y()),
        .width = static_cast<unsigned short>(this->get_width()),
        .height = static_cast<unsigned short>(this->get_height())};
  }
#endif /* BUILD_X11 */
};

}  // namespace conky

#endif /* _CONKY_GEOMETRY_H_ */
