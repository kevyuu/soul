#pragma once

#include "core/matrix.h"

#include "math/common.h"
#include "math/quaternion.h"

namespace soul::math
{

  // ----------------------------------------------------------------------------
  // Multiplication
  // ----------------------------------------------------------------------------

  /// Multiply Matrix and Matrix.
  template <typename T, u8 LhsRowCountV, u8 LhsColCountV, u8 RhsColCountV>
  [[nodiscard]]
  auto mul(
    const Matrix<T, LhsRowCountV, LhsColCountV>& lhs,
    const Matrix<T, LhsColCountV, RhsColCountV>& rhs) -> Matrix<T, LhsRowCountV, RhsColCountV>
  {
    Matrix<T, LhsRowCountV, RhsColCountV> result;
    for (i32 m = 0; m < LhsRowCountV; ++m) {
      for (i32 p = 0; p < RhsColCountV; ++p) {
        result[m][p] = dot(lhs.row(m), rhs.col(p));
      }
    }
    return result;
  }
  /// Multiply Matrix and Vec. Vec is treated as a column Vec.
  template <typename T, u8 RowCountV, u8 ColCountV>
  [[nodiscard]]
  auto mul(const Matrix<T, RowCountV, ColCountV>& lhs, const Vec<T, ColCountV>& rhs)
    -> Vec<T, RowCountV>
  {
    Vec<T, RowCountV> result;
    for (i32 r = 0; r < RowCountV; ++r) {
      result[r] = dot(lhs.row(r), rhs);
    }
    return result;
  }

  /// Multiply Vec and Matrix. Vec is treated as a row Vec.
  template <typename T, u8 RowCountV, u8 ColCountV>
  [[nodiscard]]
  auto mul(const Vec<T, RowCountV>& lhs, const Matrix<T, RowCountV, ColCountV>& rhs)
    -> Vec<T, ColCountV>
  {
    Vec<T, ColCountV> result;
    for (i32 c = 0; c < ColCountV; ++c) {
      result[c] = dot(lhs, rhs.col(c));
    }
    return result;
  }

  /// Transform a point by a 4x4 Matrix. The point is treated as a column Vec with a 1 in the 4th
  /// component.
  template <typename T>
  [[nodiscard]]
  auto transform_point(const Matrix<T, 4, 4>& m, const Vec<T, 3>& v) -> Vec<T, 3>
  {
    return mul(m, Vec<T, 4>(v, T(1))).xyz();
  }

  /// Transform a Vec by a 3x3 Matrix.
  template <typename T>
  [[nodiscard]]
  auto transform_vector(const Matrix<T, 3, 3>& m, const Vec<T, 3>& v) -> Vec<T, 3>
  {
    return mul(m, v);
  }

  /// Transform a vectir by a 4x4 Matrix. The Vec is treated as a column Vec with a 0 in the
  /// 4th component.
  template <typename T>
  [[nodiscard]]
  auto transform_vector(const Matrix<T, 4, 4>& m, const Vec<T, 3>& v) -> Vec<T, 3>
  {
    return mul(m, Vec<T, 4>(v, T(0))).xyz();
  }

  // ----------------------------------------------------------------------------
  // Functions
  // ----------------------------------------------------------------------------

  /// Tranpose a Matrix.
  template <typename T, u8 RowCountV, u8 ColCountV>
  auto transpose(const Matrix<T, RowCountV, ColCountV>& m) -> Matrix<T, ColCountV, RowCountV>
  {
    Matrix<T, ColCountV, RowCountV> result;
    for (i32 r = 0; r < RowCountV; ++r) {
      for (i32 c = 0; c < ColCountV; ++c) {
        result[c][r] = m[r][c];
      }
    }
    return result;
  }

  /// Apply a translation to a 4x4 Matrix.
  template <typename T>
  auto translate(const Matrix<T, 4, 4>& m, const Vec<T, 3>& v) -> Matrix<T, 4, 4>
  {
    Matrix<T, 4, 4> result(m);
    result.set_col(3, m.col(0) * v.x + m.col(1) * v.y + m.col(2) * v.z + m.col(3));
    return result;
  }

  /// Apply a rotation around an axis to a 4x4 Matrix.
  template <typename T>
  auto rotate(const Matrix<T, 4, 4>& m, T angle, const Vec<T, 3>& axis_) -> Matrix<T, 4, 4>
  {
    T a = angle;
    T c = cos(a);
    T s = sin(a);

    Vec<T, 3> axis(normalize(axis_));
    Vec<T, 3> temp((T(1) - c) * axis);

    Matrix<T, 4, 4> rotate;
    rotate[0][0] = c + temp[0] * axis[0];
    rotate[0][1] = temp[1] * axis[0] - s * axis[2];
    rotate[0][2] = temp[2] * axis[0] + s * axis[1];

    rotate[1][0] = temp[0] * axis[1] + s * axis[2];
    rotate[1][1] = c + temp[1] * axis[1];
    rotate[1][2] = temp[2] * axis[1] - s * axis[0];

    rotate[2][0] = temp[0] * axis[2] - s * axis[1];
    rotate[2][1] = temp[1] * axis[2] + s * axis[0];
    rotate[2][2] = c + temp[2] * axis[2];

    Matrix<T, 4, 4> result;
    result.set_col(0, m.col(0) * rotate[0][0] + m.col(1) * rotate[1][0] + m.col(2) * rotate[2][0]);
    result.set_col(1, m.col(0) * rotate[0][1] + m.col(1) * rotate[1][1] + m.col(2) * rotate[2][1]);
    result.set_col(2, m.col(0) * rotate[0][2] + m.col(1) * rotate[1][2] + m.col(2) * rotate[2][2]);
    result.set_col(3, m.col(3));

    return result;
  }

  /// Apply a scale to a 4x4 Matrix.
  template <typename T>
  auto scale(const Matrix<T, 4, 4>& m, const Vec<T, 3>& v) -> Matrix<T, 4, 4>
  {
    Matrix<T, 4, 4> result;
    result.set_col(0, m.col(0) * v[0]);
    result.set_col(1, m.col(1) * v[1]);
    result.set_col(2, m.col(2) * v[2]);
    result.set_col(3, m.col(3));
    return result;
  }

  /// Compute determinant of a 2x2 Matrix.
  template <typename T>
  [[nodiscard]]
  inline auto determinant(const Matrix<T, 2, 2>& m) -> T
  {
    return m[0][0] * m[1][1] - m[1][0] * m[0][1];
  }

  /// Compute determinant of a 3x3 Matrix.
  template <typename T>
  [[nodiscard]]
  inline auto determinant(const Matrix<T, 3, 3>& m) -> T
  {
    T a = m[0][0] * (m[1][1] * m[2][2] - m[2][1] * m[1][2]);
    T b = m[1][0] * (m[0][1] * m[2][2] - m[2][1] * m[0][2]);
    T c = m[2][0] * (m[0][1] * m[1][2] - m[1][1] * m[0][2]);
    return a - b + c;
  }

  /// Compute determinant of a 4x4 Matrix.
  template <typename T>
  [[nodiscard]]
  inline auto determinant(const Matrix<T, 4, 4>& m) -> T
  {
    T subFactor00 = m[2][2] * m[3][3] - m[3][2] * m[2][3];
    T subFactor01 = m[2][1] * m[3][3] - m[3][1] * m[2][3];
    T subFactor02 = m[2][1] * m[3][2] - m[3][1] * m[2][2];
    T subFactor03 = m[2][0] * m[3][3] - m[3][0] * m[2][3];
    T subFactor04 = m[2][0] * m[3][2] - m[3][0] * m[2][2];
    T subFactor05 = m[2][0] * m[3][1] - m[3][0] * m[2][1];

    Vec<T, 4> detCof(
      +(m[1][1] * subFactor00 - m[1][2] * subFactor01 + m[1][3] * subFactor02), //
      -(m[1][0] * subFactor00 - m[1][2] * subFactor03 + m[1][3] * subFactor04), //
      +(m[1][0] * subFactor01 - m[1][1] * subFactor03 + m[1][3] * subFactor05), //
      -(m[1][0] * subFactor02 - m[1][1] * subFactor04 + m[1][2] * subFactor05)  //
    );

    return m[0][0] * detCof[0] + m[0][1] * detCof[1] + m[0][2] * detCof[2] + m[0][3] * detCof[3];
  }

  /// Compute inverse of a 2x2 Matrix.
  template <typename T>
  [[nodiscard]]
  inline auto inverse(const Matrix<T, 2, 2>& m) -> Matrix<T, 2, 2>
  {
    T det_rcp = T(1) / determinant(m);
    return Matrix<T, 2, 2>{
      +m[1][1] * det_rcp, // 0,0
      -m[0][1] * det_rcp, // 0,1
      -m[1][0] * det_rcp, // 1,0
      +m[0][0] * det_rcp, // 1,1
    };
  }

  /// Compute inverse of a 3x3 Matrix.
  template <typename T>
  [[nodiscard]]
  inline auto inverse(const Matrix<T, 3, 3>& m) -> Matrix<T, 3, 3>
  {
    T det_rcp = T(1) / determinant(m);

    Matrix<T, 3, 3> result;
    result[0][0] = +(m[1][1] * m[2][2] - m[1][2] * m[2][1]) * det_rcp;
    result[0][1] = -(m[0][1] * m[2][2] - m[0][2] * m[2][1]) * det_rcp;
    result[0][2] = +(m[0][1] * m[1][2] - m[0][2] * m[1][1]) * det_rcp;
    result[1][0] = -(m[1][0] * m[2][2] - m[1][2] * m[2][0]) * det_rcp;
    result[1][1] = +(m[0][0] * m[2][2] - m[0][2] * m[2][0]) * det_rcp;
    result[1][2] = -(m[0][0] * m[1][2] - m[0][2] * m[1][0]) * det_rcp;
    result[2][0] = +(m[1][0] * m[2][1] - m[1][1] * m[2][0]) * det_rcp;
    result[2][1] = -(m[0][0] * m[2][1] - m[0][1] * m[2][0]) * det_rcp;
    result[2][2] = +(m[0][0] * m[1][1] - m[0][1] * m[1][0]) * det_rcp;
    return result;
  }

  /// Compute inverse of a 4x4 Matrix.
  template <typename T>
  [[nodiscard]]
  inline auto inverse(const Matrix<T, 4, 4>& m) -> Matrix<T, 4, 4>
  {
    T c00 = m[2][2] * m[3][3] - m[2][3] * m[3][2];
    T c02 = m[2][1] * m[3][3] - m[2][3] * m[3][1];
    T c03 = m[2][1] * m[3][2] - m[2][2] * m[3][1];

    T c04 = m[1][2] * m[3][3] - m[1][3] * m[3][2];
    T c06 = m[1][1] * m[3][3] - m[1][3] * m[3][1];
    T c07 = m[1][1] * m[3][2] - m[1][2] * m[3][1];

    T c08 = m[1][2] * m[2][3] - m[1][3] * m[2][2];
    T c10 = m[1][1] * m[2][3] - m[1][3] * m[2][1];
    T c11 = m[1][1] * m[2][2] - m[1][2] * m[2][1];

    T c12 = m[0][2] * m[3][3] - m[0][3] * m[3][2];
    T c14 = m[0][1] * m[3][3] - m[0][3] * m[3][1];
    T c15 = m[0][1] * m[3][2] - m[0][2] * m[3][1];

    T c16 = m[0][2] * m[2][3] - m[0][3] * m[2][2];
    T c18 = m[0][1] * m[2][3] - m[0][3] * m[2][1];
    T c19 = m[0][1] * m[2][2] - m[0][2] * m[2][1];

    T c20 = m[0][2] * m[1][3] - m[0][3] * m[1][2];
    T c22 = m[0][1] * m[1][3] - m[0][3] * m[1][1];
    T c23 = m[0][1] * m[1][2] - m[0][2] * m[1][1];

    Vec<T, 4> fac0(c00, c00, c02, c03);
    Vec<T, 4> fac1(c04, c04, c06, c07);
    Vec<T, 4> fac2(c08, c08, c10, c11);
    Vec<T, 4> fac3(c12, c12, c14, c15);
    Vec<T, 4> fac4(c16, c16, c18, c19);
    Vec<T, 4> fac5(c20, c20, c22, c23);

    Vec<T, 4> vec0(m[0][1], m[0][0], m[0][0], m[0][0]);
    Vec<T, 4> vec1(m[1][1], m[1][0], m[1][0], m[1][0]);
    Vec<T, 4> vec2(m[2][1], m[2][0], m[2][0], m[2][0]);
    Vec<T, 4> vec3(m[3][1], m[3][0], m[3][0], m[3][0]);

    Vec<T, 4> inv0(vec1 * fac0 - vec2 * fac1 + vec3 * fac2);
    Vec<T, 4> inv1(vec0 * fac0 - vec2 * fac3 + vec3 * fac4);
    Vec<T, 4> inv2(vec0 * fac1 - vec1 * fac3 + vec3 * fac5);
    Vec<T, 4> inv3(vec0 * fac2 - vec1 * fac4 + vec2 * fac5);

    Vec<T, 4> signA(+1, -1, +1, -1);
    Vec<T, 4> signB(-1, +1, -1, +1);
    const auto inverse =
      Matrix<T, 4, 4>::FromColumns(inv0 * signA, inv1 * signB, inv2 * signA, inv3 * signB);

    Vec<T, 4> row0(inverse[0][0], inverse[0][1], inverse[0][2], inverse[0][3]);

    Vec<T, 4> dot0(m.col(0) * row0);
    T dot1 = (dot0.x + dot0.y) + (dot0.z + dot0.w);

    T det_rcp = T(1) / dot1;

    return inverse * det_rcp;
  }

  /// Compute the (X * Y * Z) euler angles of a 4x4 Matrix.
  template <typename T>
  auto extract_euler_angle_xyz(const Matrix<T, 4, 4>& m) -> Vec<T, 3>
  {
    T t1 = atan2(m[1][2], m[2][2]);
    T c2 = sqrt(m[0][0] * m[0][0] + m[0][1] * m[0][1]);
    T t2 = atan2(-m[0][2], c2);
    T s1 = sin(t1);
    T c1 = cos(t1);
    T t3 = atan2(s1 * m[2][0] - c1 * m[1][0], c1 * m[1][1] - s1 * m[2][1]);
    return {-t1, -t2, -t3};
  }

  template <typename T>
  inline auto decompose(
    const Matrix<T, 4, 4>& model_matrix,
    Vec<T, 3>& scale,
    Quaternion<T>& orientation,
    Vec<T, 3>& translation,
    Vec<T, 3>& skew,
    Vec<T, 4>& perspective) -> b8
  {
    // See https://caff.de/posts/4X4-Matrix-decomposition/decomposition.pdf

    const T eps = std::numeric_limits<T>::epsilon();

    Matrix<T, 4, 4> local_matrix(model_matrix);

    // Abort if zero Matrix.
    if (abs(local_matrix[3][3]) < eps) {
      return false;
    }

    // Normalize the Matrix.
    for (i32 i = 0; i < 4; ++i) {
      for (i32 j = 0; j < 4; ++j) {
        local_matrix[i][j] /= local_matrix[3][3];
      }
    }

    // perspective_matrix is used to solve for perspective, but it also provides
    // an easy way to test for singularity of the upper 3x3 component.
    Matrix<T, 4, 4> perspective_matrix(local_matrix);
    perspective_matrix[3] = Vec<T, 4>(0, 0, 0, 1);
    if (abs(determinant(perspective_matrix)) < eps) {
      return false;
    }

    // First, isolate perspective. This is the messiest.
    if (
      abs(local_matrix[3][0]) >= eps || abs(local_matrix[3][1]) >= eps ||
      abs(local_matrix[3][2]) >= eps) {
      // right_hand_side is the right hand side of the equation.
      Vec<T, 4> right_hand_side = local_matrix[3];

      // Solve the equation by inverting perspective_matrix and multiplying
      // right_hand_side by the inverse.
      // (This is the easiest way, not necessarily the best.)
      Matrix<T, 4, 4> inverse_perspective_matrix = inverse(perspective_matrix);
      Matrix<T, 4, 4> transposed_inverse_perspective_matrix = transpose(inverse_perspective_matrix);

      perspective = mul(transposed_inverse_perspective_matrix, right_hand_side);

      // Clear the perspective partition.
      local_matrix[3] = Vec<T, 4>(0, 0, 0, 1);
    } else {
      // No perspective.
      perspective = Vec<T, 4>(0, 0, 0, 1);
    }

    // Next take care of translation (easy).
    translation = local_matrix.col(3).xyz();
    local_matrix.set_row(3, Vec<T, 4>(0, 0, 0, 1));

    Vec<T, 3> row[3];

    // Now get scale and shear.
    for (i32 i = 0; i < 3; ++i) {
      for (i32 j = 0; j < 3; ++j) {
        row[i][j] = local_matrix[j][i];
      }
    }

    // Compute X scale factor and normalize first row.
    scale.x = length(row[0]);
    row[0] = normalize(row[0]);

    // Compute XY shear factor and make 2nd row orthogonal to 1st.
    skew.z = dot(row[0], row[1]);
    row[1] = row[1] - skew.z * row[0];

    // Now, compute Y scale and normalize 2nd row.
    scale.y = length(row[1]);
    row[1] = normalize(row[1]);
    skew.z /= scale.y;

    // Compute XZ and YZ shears, orthogonalize 3rd row.
    skew.y = dot(row[0], row[2]);
    row[2] = row[2] - skew.y * row[0];
    skew.x = dot(row[1], row[2]);
    row[2] = row[2] - skew.x * row[1];

    // Next, get Z scale and normalize 3rd row.
    scale.z = length(row[2]);
    row[2] = normalize(row[2]);
    skew.y /= scale.z;
    skew.x /= scale.z;

    // At this point, the Matrix (in rows[]) is orthonormal.
    // Check for a coordinate system flip. If the determinant
    // is -1, then negate the Matrix and the scaling factors.
    if (dot(row[0], cross(row[1], row[2])) < T(0)) {
      scale *= T(-1);
      for (auto& i : row) {
        i *= T(-1);
      }
    }

    // Now, get the rotations out, as described in the gem.
    i32 i = 0;
    i32 j = 0;
    i32 k = 0;
    T root, trace = row[0].x + row[1].y + row[2].z; // NOLINT
    if (trace > T(0)) {
      root = sqrt(trace + T(1));
      orientation.w = T(0.5) * root;
      root = T(0.5) / root;
      orientation.x = root * (row[1].z - row[2].y);
      orientation.y = root * (row[2].x - row[0].z);
      orientation.z = root * (row[0].y - row[1].x);
    } // end if > 0
    else {
      static i32 next[3] = {1, 2, 0};
      i = 0;
      if (row[1].y > row[0].x) {
        i = 1;
      }
      if (row[2].z > row[i][i]) {
        i = 2;
      }
      j = next[i];
      k = next[j];

      root = sqrt(row[i][i] - row[j][j] - row[k][k] + T(1));

      orientation[i] = T(0.5) * root;
      root = T(0.5) / root;
      orientation[j] = root * (row[i][j] + row[j][i]);
      orientation[k] = root * (row[i][k] + row[k][i]);
      orientation.w = root * (row[j][k] - row[k][j]);
    } // end if <= 0

    return true;
  }

  template <typename T>
  [[nodiscard]]
  inline auto compose_transform(
    const Vec<T, 3>& translation, const Quaternion<T>& rotation, const Vec<T, 3>& scale)
    -> Matrix<T, 4, 4>
  {
    const T tx = translation.x;
    const T ty = translation.y;
    const T tz = translation.z;
    const T qx = rotation.x;
    const T qy = rotation.y;
    const T qz = rotation.z;
    const T qw = rotation.w;
    const T sx = scale.x;
    const T sy = scale.y;
    const T sz = scale.z;

    const vec4<T> column0 = {
      (1 - 2 * qy * qy - 2 * qz * qz) * sx,
      (2 * qx * qy + 2 * qz * qw) * sx,
      (2 * qx * qz - 2 * qy * qw) * sx,
      0.f,
    };

    const vec4<T> column1 = {
      (2 * qx * qy - 2 * qz * qw) * sy,
      (1 - 2 * qx * qx - 2 * qz * qz) * sy,
      (2 * qy * qz + 2 * qx * qw) * sy,
      0.f,
    };

    const vec4<T> column2 = {
      (2 * qx * qz + 2 * qy * qw) * sz,
      (2 * qy * qz - 2 * qx * qw) * sz,
      (1 - 2 * qx * qx - 2 * qy * qy) * sz,
      0.f,
    };

    const vec4<T> column3 = {
      tx,
      ty,
      tz,
      1.f,
    };

    return Matrix<T, 4, 4>::FromColumns(column0, column1, column2, column3);
  }

  template <typename T>
  [[nodiscard]]
  inline auto perspective(T fovy, T aspect, T z_near, T z_far) -> Matrix<T, 4, 4>
  {
    SOUL_ASSERT(0, abs(aspect - std::numeric_limits<T>::epsilon()) > T(0));

    T fov_rad = fovy;
    T focal_length = 1.0f / std::tan(fov_rad / 2.0f);

    T x = focal_length / aspect;
    T y = -focal_length;
    T A = z_far / (z_near - z_far);
    T B = z_near * A;

    mat4<T> result;

    result.m(0, 0) = x;
    result.m(0, 1) = 0.0f;
    result.m(0, 2) = 0.0f;
    result.m(0, 3) = 0.0f;

    result.m(1, 0) = 0.0f;
    result.m(1, 1) = y;
    result.m(1, 2) = 0.0f;
    result.m(1, 3) = 0.0f;

    result.m(2, 0) = 0.0f;
    result.m(2, 1) = 0.0f;
    result.m(2, 2) = A;
    result.m(2, 3) = B;

    result.m(3, 0) = 0.0f;
    result.m(3, 1) = 0.0f;
    result.m(3, 2) = T(-1.0);
    result.m(3, 3) = 0.0f;

    return result;
  }

  /// Creates a right-handed orthographic projection Matrix. Depth is mapped to [0, 1].
  template <typename T>
  [[nodiscard]]
  inline auto ortho(T left, T right, T bottom, T top, T z_near, T z_far) -> Matrix<T, 4, 4>
  {
    Matrix<T, 4, 4> m = Matrix<T, 4, 4>::Identity();
    m[0][0] = T(2) / (right - left);
    m[1][1] = T(2) / (top - bottom);
    m[2][2] = -T(1) / (z_far - z_near);
    m[0][3] = -(right + left) / (right - left);
    m[1][3] = -(top + bottom) / (top - bottom);
    m[2][3] = -z_near / (z_far - z_near);
    return m;
  }

  /// Creates a translation Matrix.
  template <typename T>
  [[nodiscard]]
  inline auto translate(const Vec<T, 3>& v) -> Matrix<T, 4, 4>
  {
    return translate(Matrix<T, 4, 4>::Identity(), v);
  }

  /// Creates a rotation Matrix from an angle and an axis.
  template <typename T>
  [[nodiscard]]
  inline auto rotation(T angle, const Vec<T, 3>& axis) -> Matrix<T, 4, 4>
  {
    return rotate(Matrix<T, 4, 4>::Identity(), angle, axis);
  }

  /// Creates a rotation Matrix around the X-axis.
  template <typename T>
  [[nodiscard]]
  inline auto rotation_x(T angle) -> Matrix<T, 4, 4>
  {
    T c = cos(angle);
    T s = sin(angle);

    // clang-format off
    return Matrix<T, 4, 4>{
        T(1),   T(0),   T(0),   T(0),   // row 0
        T(0),   c,      -s,     T(0),   // row 1
        T(0),   s,      c,      T(0),   // row 2
        T(0),   T(0),   T(0),   T(1)    // row 3
    };
    // clang-format on
  }

  /// Creates a rotation Matrix around the Y-axis.
  template <typename T>
  [[nodiscard]]
  inline auto rotation_y(T angle) -> Matrix<T, 4, 4>
  {
    T c = cos(angle);
    T s = sin(angle);

    // clang-format off
    return Matrix<T, 4, 4>{
        c,      T(0),   s,      T(0),   // row 0
        T(0),   T(1),   T(0),   T(0),   // row 1
        -s,     T(0),   c,      T(0),   // row 2
        T(0),   T(0),   T(0),   T(1)    // row 3
    };
    // clang-format on
  }

  /// Creates a rotation Matrix around the Z-axis.
  template <typename T>
  [[nodiscard]]
  inline auto rotation_z(T angle) -> Matrix<T, 4, 4>
  {
    T c = cos(angle);
    T s = sin(angle);

    // clang-format off
    return Matrix<T, 4, 4>{
        c,      -s,     T(0),   T(0),   // row 0
        s,      c,      T(0),   T(0),   // row 1
        T(0),   T(0),   T(1),   T(0),   // row 2
        T(0),   T(0),   T(0),   T(1)    // row 3
    };
    // clang-format on
  }

  /// Creates a rotation Matrix (X * Y * Z).
  template <typename T>
  [[nodiscard]]
  inline auto rotation_xyz(T angle_x, T angle_y, T angle_z) -> Matrix<T, 4, 4>
  {
    T c1 = cos(-angle_x);
    T c2 = cos(-angle_y);
    T c3 = cos(-angle_z);
    T s1 = sin(-angle_x);
    T s2 = sin(-angle_y);
    T s3 = sin(-angle_z);

    Matrix<T, 4, 4> m;
    m[0][0] = c2 * c3;
    m[0][1] = c2 * s3;
    m[0][2] = -s2;
    m[0][3] = T(0);

    m[1][0] = -c1 * s3 + s1 * s2 * c3;
    m[1][1] = c1 * c3 + s1 * s2 * s3;
    m[1][2] = s1 * c2;
    m[1][3] = T(0);

    m[2][0] = s1 * s3 + c1 * s2 * c3;
    m[2][1] = -s1 * c3 + c1 * s2 * s3;
    m[2][2] = c1 * c2;
    m[2][3] = T(0);

    m[3][0] = T(0);
    m[3][1] = T(0);
    m[3][2] = T(0);
    m[3][3] = T(1);

    return m;
  }

  /// Creates a scaling Matrix.
  template <typename T>
  [[nodiscard]]
  inline auto scale(const Vec<T, 3>& v) -> Matrix<T, 4, 4>
  {
    return scale(Matrix<T, 4, 4>::Identity(), v);
  }

  /**
   * Build a look-at Matrix.
   * If right handed, forward direction is mapped onto -Z axis.
   * If left handed, forward direction is mapped onto +Z axis.
   * @param eye Eye position
   * @param center Center position
   * @param up Up Vec
   * @param handedness Coordinate system handedness.
   */
  template <typename T>
  [[nodiscard]]
  inline auto look_at(
    const Vec<T, 3>& eye,
    const Vec<T, 3>& center,
    const Vec<T, 3>& up,
    Handedness handedness = Handedness::RightHanded) -> Matrix<T, 4, 4>
  {
    Vec<T, 3> f(
      handedness == Handedness::RightHanded ? normalize(eye - center) : normalize(center - eye));
    Vec<T, 3> r(normalize(cross(up, f)));
    Vec<T, 3> u(cross(f, r));

    Matrix<T, 4, 4> result = Matrix<T, 4, 4>::Identity();
    result[0][0] = r.x;
    result[0][1] = r.y;
    result[0][2] = r.z;
    result[1][0] = u.x;
    result[1][1] = u.y;
    result[1][2] = u.z;
    result[2][0] = f.x;
    result[2][1] = f.y;
    result[2][2] = f.z;
    result[0][3] = -dot(r, eye);
    result[1][3] = -dot(u, eye);
    result[2][3] = -dot(f, eye);

    return result;
  }

  template <typename T>
  [[nodiscard]]
  inline auto into_matrix(const Quaternion<T>& q) -> Matrix<T, 3, 3>
  {
    Matrix<T, 3, 3> m;
    T qxx(q.x * q.x);
    T qyy(q.y * q.y);
    T qzz(q.z * q.z);
    T qxz(q.x * q.z);
    T qxy(q.x * q.y);
    T qyz(q.y * q.z);
    T qwx(q.w * q.x);
    T qwy(q.w * q.y);
    T qwz(q.w * q.z);

    m[0][0] = T(1) - T(2) * (qyy + qzz);
    m[0][1] = T(2) * (qxy - qwz);
    m[0][2] = T(2) * (qxz + qwy);

    m[1][0] = T(2) * (qxy + qwz);
    m[1][1] = T(1) - T(2) * (qxx + qzz);
    m[1][2] = T(2) * (qyz - qwx);

    m[2][0] = T(2) * (qxz - qwy);
    m[2][1] = T(2) * (qyz + qwx);
    m[2][2] = T(1) - T(2) * (qxx + qyy);

    return m;
  }

} // namespace soul::math
