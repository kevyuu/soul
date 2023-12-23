#pragma once

#include "core/boolean.h"
#include "core/compiler.h"
#include "core/floating_point.h"
#include "core/integer.h"
#include "core/quaternion.h"
#include "core/vec.h"

#include <glm/glm.hpp>

namespace soul
{

  // A matrix to represent multiplication with column g
  // Read this article if you are confused.
  // https://fgiesen.wordpress.com/2012/02/12/row-major-vs-column-major-row-vectors-vs-column-vectors/
  // Internally we use glm to store the matrix, glm use column major ordering
  // and multiply column g (used by gilbert strang lecture).
  // This struct provide and accessor m(row, column) both row and column refer
  // to the row and column of matrix, not the c++ 2d array that represent it
  // This is done to be consistent with our shader.
  template <usize Row, usize Column, typename T>
  struct matrix {
    using this_type = matrix<Column, Row, T>;
    using storage_type = glm::mat<Column, Row, T, glm::defaultp>;
    storage_type storage;

    constexpr matrix() = default;

    constexpr explicit matrix(T val) : storage(val) {}

    constexpr explicit matrix(storage_type mat) : storage(mat) {}

    [[nodiscard]]
    SOUL_ALWAYS_INLINE auto m(const usize row, const usize column) const -> T
    {
      return storage[column][row];
    }

    SOUL_ALWAYS_INLINE auto m(const usize row, const usize column) -> T&
    {
      return storage[column][row];
    }

    SOUL_ALWAYS_INLINE void set_column(usize column, const vec<Row, T>& vec)
    {
      for (usize i = 0; i < Row; i++) {
        storage[column][i] = vec[i];
      }
    }

    [[nodiscard]]
    static constexpr auto identity() -> this_type
    {
      return this_type(storage_type(1));
    }

    template <typename... VecT>
    [[nodiscard]]
    static auto FromColumns(const VecT&... column_vecs) -> matrix
    {
      static_assert((is_same_v<VecT, vec<Row, T>> && ...), "g type mismatch");
      static_assert(sizeof...(VecT) == Column, "Number of column mismatch");
      matrix mat;
      u8 column_idx = 0;
      (mat.set_column(column_idx++, column_vecs), ...);
      return mat;
    }

    [[nodiscard]]
    static auto FromRowMajorData(const T* data)
    {
      matrix mat;
      for (usize row_idx = 0; row_idx < Row; row_idx++) {
        for (usize col_idx = 0; col_idx < Column; col_idx++) {
          mat.m(row_idx, col_idx) = data[row_idx * Column + col_idx];
        }
      }
      return mat;
    };

    [[nodiscard]]
    static auto FromColumnMajorData(const T* data) -> matrix
    {
      matrix mat;
      for (usize col_idx = 0; col_idx < Column; col_idx++) {
        for (usize row_idx = 0; row_idx < Row; row_idx++) {
          mat.m(row_idx, col_idx) = data[col_idx * Row + row_idx];
        }
      }
      return mat;
    }

    [[nodiscard]]
    static auto ComposeTransform(
      const vec3<T>& translation, const quat<T>& rotation, const vec3<T>& scale)
      requires(Row == 4 && Column == 4)
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

      return matrix::FromColumns(column0, column1, column2, column3);
    }
  };

  template <typename T>
  using matrix4x4 = matrix<4, 4, T>;

  using mat3f = matrix<3, 3, f32>;
  using mat4f = matrix<4, 4, f32>;

} // namespace soul
