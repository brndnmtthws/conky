#ifndef _CONKY_GEOMETRY_H_
#define _CONKY_GEOMETRY_H_

#include <array>
#include <cstdint>
#include <type_traits>

#ifdef Success
// Undefine X11 Success definition; not used by us; conflicts with Eigen
// (namespaced) enum declarations
#undef Success
#endif
#include <eigen3/Eigen/Core>

namespace conky {

/// @brief A 2D vector representation.
///
/// Uses eigen under to hood to provide SIMD optimizations on appropriate
/// platforms if available. Doesn't expose any of the eigen functionality
/// directly to avoid pollution.
///
/// @tparam T numeric component type
template <typename T = std::intmax_t,
          typename = typename std::enable_if<std::is_arithmetic<T>::value>>
struct point {
  /// @brief Compile time component type information
  using Component = T;

  using Vec2 = Eigen::Array<T, 2, 1>;

  Vec2 value;

 protected:
  point(Vec2 vec) : value(vec) {}

 public:
  template <typename O = T,
            typename = typename std::enable_if<std::is_arithmetic<O>::value>>
  point(O x, O y) : point(Vec2(static_cast<T>(x), static_cast<T>(y))) {}

  /// @brief Zero vector value
  static inline point<T> Zero() { return point<T>(0, 0); }
  /// @brief Unit vector value
  static inline point<T> One() { return point<T>(1, 1); }
  /// @brief X unit vector value
  static inline point<T> UnitX() { return point<T>(1, 0); }
  /// @brief Y unit vector value
  static inline point<T> UnitY() { return point<T>(0, 1); }

  point() : value(Vec2::Zero()) {}

  /// @brief Point x component
  /// @return x value of this point
  inline T x() const { return value.x(); }
  /// @brief Point y component
  /// @return y value of this point
  inline T y() const { return value.y(); }

  template <typename O = T,
            typename = typename std::enable_if<std::is_arithmetic<O>::value>>
  inline void set_x(O value) {
    this->value = Vec2(static_cast<T>(value), this->value.y());
  }
  template <typename O = T,
            typename = typename std::enable_if<std::is_arithmetic<O>::value>>
  inline void set_y(O value) {
    this->value = Vec2(this->value.x(), static_cast<T>(value));
  }

  template <typename O = T>
  point(point<O> other)
      : point(static_cast<T>(other.x()), static_cast<T>(other.y())) {}
  template <typename O = T,
            typename = typename std::enable_if<std::is_arithmetic<O>::value>>
  point(std::array<O, 2> other)
      : value(Vec2(static_cast<T>(other[0]), static_cast<T>(other[1]))) {}

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

  T surface() const { return this->x() * this->y(); }

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
    return this->x() == other.x() && this->y() == other.y();
  }
  bool operator!=(point<T> other) const {
    return this->x() != other.x() || this->y() != other.y();
  }

  point<T> operator-() const { return point<T>(-this->value); }
  point<T> abs() const { return point<T>(this->value.abs()); }

  inline std::array<T, 2> to_array() const {
    return std::array<T, 2>{this->x(), this->y()};
  }
};

/// @brief 2D rectangle representation using position and size vectors.
/// @tparam T component number type
template <typename T = std::intmax_t,
          typename = typename std::enable_if<std::is_arithmetic<T>::value>>
struct rect {
  point<T> pos;
  point<T> size;

  rect() : pos(point<T>::Zero()), size(point<T>::Zero()) {}
  rect(point<T> pos, point<T> size) : pos(pos), size(size) {}

  /// @brief Rectangle x position
  /// @return x position of this rectangle
  inline T x() const { return this->pos.x(); }
  /// @brief Rectangle y position
  /// @return y position of this rectangle
  inline T y() const { return this->pos.y(); }
  /// @brief Rectangle width
  /// @return width of this rectangle
  inline T width() const { return this->size.x(); }
  /// @brief Rectangle height
  /// @return height of this rectangle
  inline T height() const { return this->size.y(); }

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

  inline std::array<point<T>, 4> corners() const {
    return std::array<point<T>, 4>{
        this->pos,
        this->pos + point<T>(this->width(), 0),
        this->pos + this->size,
        this->pos + point<T>(0, this->height()),
    };
  }

  template <typename O = T>
  bool contains(point<O> p) const {
    return p.x() >= this->x() && p.x() < this->x() + this->width() &&
           p.y() >= this->y() && p.y() < this->y() + this->height();
  }

  template <typename O = T>
  bool contains(rect<O> other) const {
    return contains(other.pos) &&
           contains(other.pos + point<O>(other.width(), 0)) &&
           contains(other.pos + other.size) &&
           contains(other.pos + point<O>(0, other.height()));
  }

 private:
  template <typename O = T>
  inline bool _intersects_partial(rect<O> other) const {
    return contains(other.pos) ||
           contains(other.pos + point<O>(other.width(), 0)) ||
           contains(other.pos + other.size) ||
           contains(other.pos + point<O>(0, other.height()));
  }

 public:
  template <typename O = T>
  bool intersects(rect<O> other) const {
    return this->_intersects_partial(other) || other._intersects_partial(*this);
  }
};

}  // namespace conky

#endif /* _CONKY_GEOMETRY_H_ */
