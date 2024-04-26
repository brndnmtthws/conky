#ifndef _CONKY_GEOMETRY_H_
#define _CONKY_GEOMETRY_H_

#include <array>
#include <cstdint>
#include <type_traits>

#ifdef BUILD_X11
#include <X11/Xlib.h>
#endif /* BUILD_X11 */

#ifdef Success
// Undefine X11 Success definition; not used by us; conflicts with Eigen
// (namespaced) enum declarations
#undef Success
#endif
#include <eigen3/Eigen/Core>

namespace conky {

namespace _priv_impl_geometry {

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
}  // namespace _priv_impl_geometry

/// @brief A 2D vector representation.
///
/// Uses eigen under to hood to provide SIMD optimizations on appropriate
/// platforms if available. Doesn't expose any of the eigen functionality
/// directly to avoid pollution.
///
/// @tparam T numeric component type.
template <typename T = std::intmax_t,
          typename = typename std::enable_if<std::is_arithmetic<T>::value>>
struct point {
  /// @brief Compile time component type information.
  using Component = T;
  using ComponentRef = _priv_impl_geometry::_member_access<point<T>, T>;

  using Data = Eigen::Array<T, 2, 1>;

  Data value;

 protected:
  point(Data vec) : value(vec) {}

 public:
  template <typename O = T,
            typename = typename std::enable_if<std::is_arithmetic<O>::value>>
  point(O x, O y) : point(Data(static_cast<T>(x), static_cast<T>(y))) {}

  /// @brief Zero vector value.
  static inline point<T> Zero() { return point<T>(0, 0); }
  /// @brief Unit vector value.
  static inline point<T> One() { return point<T>(1, 1); }
  /// @brief X unit vector value.
  static inline point<T> UnitX() { return point<T>(1, 0); }
  /// @brief Y unit vector value.
  static inline point<T> UnitY() { return point<T>(0, 1); }

  point() : value(Data::Zero()) {}

  /// @brief Point x component.
  /// @return x value of this point.
  template <typename R = T,
            typename = typename std::enable_if<std::is_arithmetic<R>::value>>
  inline R get_x() const {
    return static_cast<R>(this->value.x());
  }
  /// @brief Point y component.
  /// @return y value of this point.
  template <typename R = T,
            typename = typename std::enable_if<std::is_arithmetic<R>::value>>
  inline R get_y() const {
    return static_cast<R>(this->value.y());
  }

  /// @brief Returns point component at `index`.
  /// @tparam R value return type.
  /// @param index component index.
  /// @return component at `index` of this point.
  template <typename R = T,
            typename = typename std::enable_if<std::is_arithmetic<R>::value>>
  inline R at(size_t index) const {
    switch (index) {
      case 0:
        return this->get_x();
      case 1:
        return this->get_y();
      default:
        throw std::out_of_range("index out of range");
    }
  }

  template <typename O = T,
            typename = typename std::enable_if<std::is_arithmetic<O>::value>>
  inline void set_x(O value) {
    this->value = Data(static_cast<T>(value), this->value.y());
  }
  template <typename O = T,
            typename = typename std::enable_if<std::is_arithmetic<O>::value>>
  inline void set_y(O value) {
    this->value = Data(this->value.x(), static_cast<T>(value));
  }

  template <typename O = T,
            typename = typename std::enable_if<std::is_arithmetic<O>::value>>
  inline void set(size_t index, O value) {
    switch (index) {
      case 0:
        return this->set_x(value);
      case 1:
        return this->set_y(value);
      default:
        throw std::out_of_range("index out of range");
    }
  }

  /// @brief point x value.
  ///
  /// Treat it as `T`. Use `*x` in case type coertion is needed.
  const ComponentRef x = ComponentRef(this, 0);
  /// @brief point x value.
  ///
  /// Treat it as `T`. Use `*y` in case type coertion is needed.
  const ComponentRef y = ComponentRef(this, 1);

  template <typename O = T>
  point(point<O> other)
      : point(static_cast<T>(other.x), static_cast<T>(other.y)) {}
  template <typename O = T,
            typename = typename std::enable_if<std::is_arithmetic<O>::value>>
  point(std::array<O, 2> other)
      : value(Data(static_cast<T>(other[0]), static_cast<T>(other[1]))) {}

  template <typename O = T,
            typename = typename std::enable_if<std::is_arithmetic<O>::value>>
  static inline point<T> repeat(O v) {
    return point<T>(static_cast<T>(v), static_cast<T>(v));
  }

  template <typename O = T>
  T distance_squared(point<O> other) const {
    return (this->value - other->value).square();
  }
  template <typename O = T>
  T distance(point<O> &other) const {
    return this->distance_squared(other).sqrt();
  }
  T magnitude_squared() const {
    return this->distance_squared(point<T>::Zero());
  }
  T magnitude() const { return std::sqrt(this->magnitude_squared()); }

  T surface() const { return this->value.x() * this->value.y(); }

  point<T> &operator=(point<T> other) {
    this->value = other.value;
    return *this;
  }
  template <typename O = T>
  point<T> &operator=(std::array<O, 2> other) {
    this->set_x(other[0]);
    this->set_y(other[1]);
    return *this;
  }

  const ComponentRef &operator[](size_t index) {
    switch (index) {
      case 0:
        return this->x;
      case 1:
        return this->y;
      default:
        throw std::out_of_range("index out of range");
    }
  }

  point<T> operator+(point<T> other) const {
    return point<T>(this->value + other.value);
  }
  point<T> operator-(point<T> other) const {
    return point<T>(this->value - other.value);
  }
  template <typename O = T,
            typename = typename std::enable_if<std::is_arithmetic<O>::value>>
  point<T> operator*(O scalar) const {
    return point<T>(this->value * static_cast<T>(scalar));
  }
  point<T> operator*(point<T> other) const {
    return point<T>(this->value * other.value);
  }
  template <typename O = T,
            typename = typename std::enable_if<std::is_arithmetic<O>::value>>
  point<T> operator/(O scalar) const {
    return point<T>(this->value / static_cast<T>(scalar));
  }
  point<T> operator/(point<T> other) const {
    return point<T>(this->value / other.value);
  }

  bool operator==(point<T> other) const {
    return this->x == other.x && this->y == other.y;
  }
  bool operator!=(point<T> other) const {
    return this->x != other.x || this->y != other.y;
  }

  point<T> operator-() const { return point<T>(-this->value); }
  point<T> abs() const { return point<T>(this->value.abs()); }

  inline std::array<T, 2> to_array() const {
    return std::array<T, 2>{this->value.x(), this->value.y()};
  }
};

/// @brief 2D rectangle representation using position and size vectors.
/// @tparam T component number type.
template <typename T = std::intmax_t,
          typename = typename std::enable_if<std::is_arithmetic<T>::value>>
struct rect {
  using Component = T;
  using ComponentRef = _priv_impl_geometry::_member_access<rect<T>, T>;

  point<T> pos;
  point<T> size;

  rect() : pos(point<T>::Zero()), size(point<T>::Zero()) {}
  rect(point<T> pos, point<T> size) : pos(pos), size(size) {}

  /// @brief Rectangle x position.
  /// @return x position of this rectangle.
  template <typename R = T,
            typename = typename std::enable_if<std::is_arithmetic<R>::value>>
  inline R get_x() const {
    return static_cast<R>(this->pos.x);
  }
  /// @brief Rectangle y position.
  /// @return y position of this rectangle.
  template <typename R = T,
            typename = typename std::enable_if<std::is_arithmetic<R>::value>>
  inline R get_y() const {
    return static_cast<R>(this->pos.y);
  }
  /// @brief Rectangle width.
  /// @return width of this rectangle.
  template <typename R = T,
            typename = typename std::enable_if<std::is_arithmetic<R>::value>>
  inline R get_width() const {
    return static_cast<R>(this->size.x);
  }
  /// @brief Rectangle height.
  /// @return height of this rectangle.
  template <typename R = T,
            typename = typename std::enable_if<std::is_arithmetic<R>::value>>
  inline R get_height() const {
    return static_cast<R>(this->size.y);
  }

  /// @brief Returns rectangle component at `index`.
  /// @tparam R value return type.
  /// @param index component index.
  /// @return component at `index` of this rectangle.
  template <typename R = T,
            typename = typename std::enable_if<std::is_arithmetic<R>::value>>
  inline R at(size_t index) const {
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
        throw std::out_of_range("index out of range");
    }
  }

  template <typename O = T,
            typename = typename std::enable_if<std::is_arithmetic<O>::value>>
  inline void set_x(O value) {
    this->pos.set_x(value);
  }
  template <typename O = T,
            typename = typename std::enable_if<std::is_arithmetic<O>::value>>
  inline void set_y(O value) {
    this->pos.set_y(value);
  }
  template <typename O = T,
            typename = typename std::enable_if<std::is_arithmetic<O>::value>>
  inline void set_width(O value) {
    this->size.set_x(value);
  }
  template <typename O = T,
            typename = typename std::enable_if<std::is_arithmetic<O>::value>>
  inline void set_height(O value) {
    this->size.set_y(value);
  }

  template <typename O = T,
            typename = typename std::enable_if<std::is_arithmetic<O>::value>>
  inline void set(size_t index, O value) {
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
        throw std::out_of_range("index out of range");
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

  inline std::array<point<T>, 4> corners() const {
    return std::array<point<T>, 4>{
        this->pos,
        this->pos + point<T>(this->width, 0),
        this->pos + this->size,
        this->pos + point<T>(0, this->height),
    };
  }

  template <typename O = T>
  bool contains(point<O> p) const {
    return p.x >= this->x && p.x < this->x + this->width && p.y >= this->y &&
           p.y < this->y + this->height;
  }

  template <typename O = T>
  bool contains(rect<O> other) const {
    return contains(other.pos) &&
           contains(other.pos + point<O>(other.width, 0)) &&
           contains(other.pos + other.size) &&
           contains(other.pos + point<O>(0, other.height));
  }

 private:
  template <typename O = T>
  inline bool _intersects_partial(rect<O> other) const {
    return contains(other.pos) ||
           contains(other.pos + point<O>(other.width, 0)) ||
           contains(other.pos + other.size) ||
           contains(other.pos + point<O>(0, other.height));
  }

 public:
  template <typename O = T>
  bool intersects(rect<O> other) const {
    return this->_intersects_partial(other) || other._intersects_partial(*this);
  }

  const ComponentRef &operator[](size_t index) {
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
        throw std::out_of_range("index out of range");
    }
  }

#ifdef BUILD_X11
  XRectangle to_xrectangle() const {
    return XRectangle{.x = static_cast<short>(this->x),
                      .y = static_cast<short>(this->y),
                      .width = static_cast<unsigned short>(this->width),
                      .height = static_cast<unsigned short>(this->height)};
  }
#endif /* BUILD_X11 */
};

}  // namespace conky

#endif /* _CONKY_GEOMETRY_H_ */
