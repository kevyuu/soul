#pragma once

#include "core/compiler.h"
#include "core/matrix.h"

#include "math/common.h"
#include "math/constant.h"

namespace soul::math
{
  template <ts_arithmetic T>
  struct Quaternion {
    T x, y, z, w;

    constexpr Quaternion() noexcept : x(0), y(0), z(0), w(1) {}

    constexpr Quaternion(T x, T y, T z, T w) noexcept : x(x), y(y), z(z), w(w) {}

    constexpr Quaternion(vec3<T> xyz, T w) noexcept : x(xyz.x), y(xyz.y), z(xyz.z), w(w) {}

    [[nodiscard]]
    static constexpr auto Identity() -> Quaternion
    {
      return Quaternion(T(0), T(0), T(0), T(1));
    }

    [[nodiscard]]
    constexpr static auto FromData(const T* val) -> Quaternion
    {
      return Quaternion(val[0], val[1], val[2], val[3]);
    };
  };

  inline namespace builtin
  {
    using quatf32 = Quaternion<f32>;
    using quatf64 = Quaternion<f64>;
  } // namespace builtin

  // ----------------------------------------------------------------------------
  // Unary operators (component-wise)
  // ----------------------------------------------------------------------------

  /// Unary plus operator
  template <typename T>
  [[nodiscard]]
  constexpr auto
  operator+(const Quaternion<T> q) -> Quaternion<T>
  {
    return q;
  }

  /// Unary minus operator
  template <typename T>
  [[nodiscard]]
  constexpr auto
  operator-(const Quaternion<T> q) -> Quaternion<T>
  {
    return Quaternion<T>{-q.x, -q.y, -q.z, -q.w};
  }

  // ----------------------------------------------------------------------------
  // Binary operators (component-wise)
  // ----------------------------------------------------------------------------

  /// Binary + operator
  template <typename T>
  [[nodiscard]]
  constexpr auto
  operator+(const Quaternion<T>& lhs, const Quaternion<T>& rhs) -> Quaternion<T>
  {
    return Quaternion<T>{lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z, lhs.w + rhs.w};
  }

  /// Binary + operator
  template <typename T>
  [[nodiscard]]
  constexpr auto
  operator+(const Quaternion<T>& lhs, T rhs) -> Quaternion<T>
  {
    return Quaternion<T>{lhs.x + rhs, lhs.y + rhs, lhs.z + rhs, lhs.w + rhs};
  }

  /// Binary + operator
  template <typename T>
  [[nodiscard]]
  constexpr auto
  operator+(T lhs, const Quaternion<T>& rhs) -> Quaternion<T>
  {
    return Quaternion<T>{lhs + rhs.x, lhs + rhs.y, lhs + rhs.z, lhs + rhs.w};
  }

  /// Binary - operator
  template <typename T>
  [[nodiscard]]
  constexpr auto
  operator-(const Quaternion<T>& lhs, const Quaternion<T>& rhs) -> Quaternion<T>
  {
    return Quaternion<T>{lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z, lhs.w - rhs.w};
  }

  /// Binary - operator
  template <typename T>
  [[nodiscard]]
  constexpr auto
  operator-(const Quaternion<T>& lhs, T rhs) -> Quaternion<T>
  {
    return Quaternion<T>{lhs.x - rhs, lhs.y - rhs, lhs.z - rhs, lhs.w - rhs};
  }

  /// Binary - operator
  template <typename T>
  [[nodiscard]]
  constexpr auto
  operator-(T lhs, const Quaternion<T>& rhs) -> Quaternion<T>
  {
    return Quaternion<T>{lhs - rhs.x, lhs - rhs.y, lhs - rhs.z, lhs - rhs.w};
  }

  /// Binary * operator
  template <typename T>
  [[nodiscard]]
  constexpr auto
  operator*(const Quaternion<T>& lhs, T rhs) -> Quaternion<T>
  {
    return Quaternion<T>{lhs.x * rhs, lhs.y * rhs, lhs.z * rhs, lhs.w * rhs};
  }

  /// Binary * operator
  template <typename T>
  [[nodiscard]]
  constexpr auto
  operator*(T lhs, const Quaternion<T>& rhs) -> Quaternion<T>
  {
    return Quaternion<T>{lhs * rhs.x, lhs * rhs.y, lhs * rhs.z, lhs * rhs.w};
  }

  /// Binary / operator
  template <typename T>
  [[nodiscard]]
  constexpr auto
  operator/(const Quaternion<T>& lhs, T rhs) -> Quaternion<T>
  {
    return Quaternion<T>{lhs.x / rhs, lhs.y / rhs, lhs.z / rhs, lhs.w / rhs};
  }

  /// Binary == operator
  template <typename T>
  [[nodiscard]]
  constexpr auto
  operator==(const Quaternion<T>& lhs, const Quaternion<T>& rhs) -> Vec<bool, 4>
  {
    return vec4b8{lhs.x == rhs.x, lhs.y == rhs.y, lhs.z == rhs.z, lhs.w == rhs.w};
  }

  /// Binary != operator
  template <typename T>
  [[nodiscard]]
  constexpr auto
  operator!=(const Quaternion<T>& lhs, const Quaternion<T>& rhs) -> Vec<bool, 4>
  {
    return vec4b8{lhs.x != rhs.x, lhs.y != rhs.y, lhs.z != rhs.z, lhs.w != rhs.w};
  }

  // ----------------------------------------------------------------------------
  // Multiplication
  // ----------------------------------------------------------------------------

  template <typename T>
  [[nodiscard]]
  constexpr auto mul(const Quaternion<T>& lhs, const Quaternion<T>& rhs) noexcept -> Quaternion<T>
  {
    return Quaternion<T>{
      lhs.w * rhs.x + lhs.x * rhs.w + lhs.y * rhs.z - lhs.z * rhs.y, // x
      lhs.w * rhs.y + lhs.y * rhs.w + lhs.z * rhs.x - lhs.x * rhs.z, // y
      lhs.w * rhs.z + lhs.z * rhs.w + lhs.x * rhs.y - lhs.y * rhs.x, // z
      lhs.w * rhs.w - lhs.x * rhs.x - lhs.y * rhs.y - lhs.z * rhs.z  // w
    };
  }

  template <typename T>
  [[nodiscard]]
  constexpr auto mul(const Quaternion<T>& q, const Vec<T, 3>& v) noexcept -> Vec<T, 3>
  {
    Vec<T, 3> qv(q.x, q.y, q.z);
    Vec<T, 3> uv(cross(qv, v));
    Vec<T, 3> uuv(cross(qv, uv));
    return v + ((uv * q.w) + uuv) * T(2);
  }

  template <typename T>
  [[nodiscard]]
  constexpr auto transform_vec(const Quaternion<T>& q, const Vec<T, 3>& v) noexcept -> Vec<T, 3>
  {
    return mul(q, v);
  }

  // ----------------------------------------------------------------------------
  // Floating point checks
  // ----------------------------------------------------------------------------

  /// isfinite
  template <typename T>
  [[nodiscard]]
  constexpr auto isfinite(const Quaternion<T>& q) -> Vec<bool, 4>
  {
    return Vec<bool, 4>{isfinite(q.x), isfinite(q.y), isfinite(q.z), isfinite(q.w)};
  }

  /// isinf
  template <typename T>
  [[nodiscard]]
  constexpr auto isinf(const Quaternion<T>& q) -> Vec<bool, 4>
  {
    return Vec<bool, 4>{isinf(q.x), isinf(q.y), isinf(q.z), isinf(q.w)};
  }

  /// isnan
  template <typename T>
  [[nodiscard]]
  constexpr auto isnan(const Quaternion<T>& q) -> Vec<bool, 4>
  {
    return Vec<bool, 4>{isnan(q.x), isnan(q.y), isnan(q.z), isnan(q.w)};
  }

  // ----------------------------------------------------------------------------
  // Geometryic functions
  // ----------------------------------------------------------------------------

  /// dot
  template <typename T>
  [[nodiscard]]
  constexpr auto dot(const Quaternion<T>& lhs, const Quaternion<T>& rhs) -> T
  {
    Vec<T, 4> tmp{lhs.w * rhs.w, lhs.x * rhs.x, lhs.y * rhs.y, lhs.z * rhs.z};
    return (tmp.x + tmp.y) + (tmp.z + tmp.w);
  }

  /// cross
  template <typename T>
  [[nodiscard]]
  constexpr auto cross(const Quaternion<T>& lhs, const Quaternion<T>& rhs) -> Quaternion<T>
  {
    return Quaternion<T>(
      lhs.w * rhs.x + lhs.x * rhs.w + lhs.y * rhs.z - lhs.z * rhs.y, // x
      lhs.w * rhs.y + lhs.y * rhs.w + lhs.z * rhs.x - lhs.x * rhs.z, // y
      lhs.w * rhs.z + lhs.z * rhs.w + lhs.x * rhs.y - lhs.y * rhs.x, // z
      lhs.w * rhs.w - lhs.x * rhs.x - lhs.y * rhs.y - lhs.z * rhs.z  // w
    );
  }

  /// length
  template <typename T>
  [[nodiscard]]
  auto length(const Quaternion<T>& q) -> T
  {
    return sqrt(dot(q, q));
  }

  /// normalize
  template <typename T>
  [[nodiscard]]
  constexpr auto normalize(const Quaternion<T>& q) -> Quaternion<T>
  {
    T len = length(q);
    if (len <= T(0)) {
      return Quaternion<T>(T(0), T(0), T(0), T(1));
    }
    return q * (T(1) / len);
  }

  /// conjugate
  template <typename T>
  [[nodiscard]]
  constexpr auto conjugate(const Quaternion<T>& q) -> Quaternion<T>
  {
    return Quaternion<T>(-q.x, -q.y, -q.z, q.w);
  }

  /// inverse
  template <typename T>
  auto inverse(const Quaternion<T>& q) -> Quaternion<T>
  {
    return conjugate(q) / dot(q, q);
  }

  /// Linear interpolation.
  template <typename T>
  [[nodiscard]]
  constexpr auto lerp(const Quaternion<T>& q1, const Quaternion<T>& q2, T t) -> Quaternion<T>
  {
    return q1 * (T(1) - t) + q2 * t;
  }

  /// Spherical linear interpolation.
  template <typename T>
  [[nodiscard]]
  constexpr auto slerp(const Quaternion<T>& q1, const Quaternion<T>& q2_, T t) -> Quaternion<T>
  {
    Quaternion<T> q2 = q2_;

    T cos_theta = dot(q1, q2);

    // If cosTheta < 0, the interpolation will take the long way around the sphere.
    // To fix this, one Quaternion must be negated.
    if (cos_theta < T(0)) {
      q2 = -q2;
      cos_theta = -cos_theta;
    }

    // Perform a linear interpolation when cosTheta is close to 1 to avoid side effect of sin(angle)
    // becoming a zero denominator
    if (cos_theta > T(1) - std::numeric_limits<T>::epsilon()) {
      // Linear interpolation
      return lerp(q1, q2, t);
    } else {
      // Essential Mathematics, page 467
      T angle = acos(cos_theta);
      return (sin((T(1) - t) * angle) * q1 + sin(t * angle) * q2) / sin(angle);
    }
  }

  // ----------------------------------------------------------------------------
  // Misc
  // ----------------------------------------------------------------------------

  /// Returns pitch value of euler angles expressed in radians.
  template <typename T>
  [[nodiscard]]
  constexpr auto pitch(const Quaternion<T>& q) -> T
  {
    T y = T(2) * (q.y * q.z + q.w * q.x);
    T x = q.w * q.w - q.x * q.x - q.y * q.y + q.z * q.z;

    // Handle sigularity, avoid atan2(0,0)
    if (abs(x) < std::numeric_limits<T>::epsilon() && abs(y) < std::numeric_limits<T>::epsilon()) {
      return T(T(2) * atan2(q.x, q.w));
    }

    return atan2(y, x);
  }

  /// Returns yaw value of euler angles expressed in radians.
  template <typename T>
  [[nodiscard]]
  constexpr auto yaw(const Quaternion<T>& q) -> T
  {
    return asin(clamp(T(-2) * (q.x * q.z - q.w * q.y), T(-1), T(1)));
  }

  /// Returns roll value of euler angles expressed in radians.
  template <typename T>
  [[nodiscard]]
  constexpr auto roll(const Quaternion<T>& q) -> T
  {
    return atan2(T(2) * (q.x * q.y + q.w * q.z), q.w * q.w + q.x * q.x - q.y * q.y - q.z * q.z);
  }

  /// Extract the euler angles in radians from a quaternion (pitch as x, yaw as y, roll as z).
  template <typename T>
  [[nodiscard]]
  constexpr auto euler_angles(const Quaternion<T>& q) -> Vec<T, 3>
  {
    return Vec<T, 3>(pitch(q), yaw(q), roll(q));
  }

  // ----------------------------------------------------------------------------
  // Construction
  // ----------------------------------------------------------------------------

  /**
   * Build a quaternion from an angle and a normalized axis.
   * @param angle Angle expressed in radians.
   * @param axis Axis of the quaternion (must be normalized).
   */
  template <typename T>
  [[nodiscard]]
  inline auto quat_angle_axis(T angle, const Vec<T, 3>& axis) -> Quaternion<T>
  {
    T s = sin(angle * T(0.5));
    T c = cos(angle * T(0.5));
    return Quaternion<T>(axis * s, c);
  }

  /**
   * Compute the rotation between two vectors.
   * @param orig Origin vector (must to be normalized).
   * @param dest Destination vector (must to be normalized).
   */
  template <typename T>
  [[nodiscard]]
  inline auto quat_rotation_between_vectors(const Vec<T, 3>& orig, const Vec<T, 3>& dest)
    -> Quaternion<T>
  {
    T cosTheta = dot(orig, dest);
    Vec<T, 3> axis;

    if (cosTheta >= T(1) - std::numeric_limits<T>::epsilon()) {
      // orig and dest point in the same direction
      return Quaternion<T>::identity();
    }

    if (cosTheta < T(-1) + std::numeric_limits<T>::epsilon()) {
      // special case when vectors in opposite directions :
      // there is no "ideal" rotation axis
      // So guess one; any will do as long as it's perpendicular to start
      // This implementation favors a rotation around the Up axis (Y),
      // since it's often what you want to do.
      axis = cross(vector<T, 3>(0, 0, 1), orig);
      if (dot(axis, axis) < std::numeric_limits<T>::epsilon()) { // bad luck, they were parallel,
                                                                 // try again!
        axis = cross(vector<T, 3>(1, 0, 0), orig);
      }

      axis = normalize(axis);
      return quat_angle_axis(T(f32const::PI), axis);
    }

    // Implementation from Stan Melax's Game Programming Gems 1 article
    axis = cross(orig, dest);

    T s = sqrt((T(1) + cosTheta) * T(2));
    T invs = T(1) / s;

    return Quaternion<T>(axis.x * invs, axis.y * invs, axis.z * invs, s * T(0.5f));
  }

  /**
   * Build a quaternion from euler angles (pitch, yaw, roll), in radians.
   */
  template <typename T>
  [[nodiscard]]
  inline auto quat_euler_angles(const Vec<T, 3>& euler_angles) -> Quaternion<T>
  {
    Vec<T, 3> c = cos(euler_angles * T(0.5));
    Vec<T, 3> s = sin(euler_angles * T(0.5));

    return Quaternion<T>(
      s.x * c.y * c.z - c.x * s.y * s.z, // x
      c.x * s.y * c.z + s.x * c.y * s.z, // y
      c.x * c.y * s.z - s.x * s.y * c.z, // z
      c.x * c.y * c.z + s.x * s.y * s.z  // w
    );
  }

  /**
   * Construct a quaternion from a 3x3 rotation matrix.
   */
  template <typename T>
  [[nodiscard]]
  inline auto into_quat(const Matrix<T, 3, 3>& m) -> Quaternion<T>
  {
    T four_x_squared_minus1 = m[0][0] - m[1][1] - m[2][2];
    T four_y_squared_minus1 = m[1][1] - m[0][0] - m[2][2];
    T four_z_squared_minus1 = m[2][2] - m[0][0] - m[1][1];
    T four_w_squared_minus1 = m[0][0] + m[1][1] + m[2][2];

    int biggest_index = 0;
    T four_biggest_squared_minus1 = four_w_squared_minus1;
    if (four_x_squared_minus1 > four_biggest_squared_minus1) {
      four_biggest_squared_minus1 = four_x_squared_minus1;
      biggest_index = 1;
    }
    if (four_y_squared_minus1 > four_biggest_squared_minus1) {
      four_biggest_squared_minus1 = four_y_squared_minus1;
      biggest_index = 2;
    }
    if (four_z_squared_minus1 > four_biggest_squared_minus1) {
      four_biggest_squared_minus1 = four_z_squared_minus1;
      biggest_index = 3;
    }

    T biggestVal = sqrt(four_biggest_squared_minus1 + T(1)) * T(0.5);
    T mult = T(0.25) / biggestVal;

    switch (biggest_index) {
    case 0:
      return Quaternion<T>(
        (m[2][1] - m[1][2]) * mult,
        (m[0][2] - m[2][0]) * mult,
        (m[1][0] - m[0][1]) * mult,
        biggestVal);
    case 1:
      return Quaternion<T>(
        biggestVal,
        (m[1][0] + m[0][1]) * mult,
        (m[0][2] + m[2][0]) * mult,
        (m[2][1] - m[1][2]) * mult);
    case 2:
      return Quaternion<T>(
        (m[1][0] + m[0][1]) * mult,
        biggestVal,
        (m[2][1] + m[1][2]) * mult,
        (m[0][2] - m[2][0]) * mult);
    case 3:
      return Quaternion<T>(
        (m[0][2] + m[2][0]) * mult,
        (m[2][1] + m[1][2]) * mult,
        biggestVal,
        (m[1][0] - m[0][1]) * mult);
    default: unreachable(); return Quaternion<T>::identity();
    }
  }

  /**
   * Build a look-at quaternion.
   * If right handed, forward direction is mapped onto -Z axis.
   * If left handed, forward direction is mapped onto +Z axis.
   * @param dir Forward direction (must to be normalized).
   * @param up Up Vec (must be normalized).
   * @param handedness Coordinate system handedness.
   */
  template <typename T>
  [[nodiscard]]
  inline auto quat_look_at(
    const Vec<T, 3>& dir, const Vec<T, 3>& up, Handedness handedness = Handedness::RightHanded)
    -> Quaternion<T>
  {
    Matrix<T, 3, 3> m;
    m.setCol(2, handedness == Handedness::RightHanded ? -dir : dir);
    Vec<T, 3> right = normalize(cross(up, m.getCol(2)));
    m.setCol(0, right);
    m.setCol(1, cross(m.getCol(2), m.getCol(0)));

    return into_quat(m);
  }
} // namespace soul::math
