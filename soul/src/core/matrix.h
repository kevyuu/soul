#pragma once

#include "core/boolean.h"
#include "core/compiler.h"
#include "core/floating_point.h"
#include "core/integer.h"
#include "core/panic.h"
#include "core/vec.h"

namespace soul
{

  template <typename T, u8 RowCountV, u8 ColCountV>
  class Matrix
  {
    static_assert(RowCountV >= 1 && RowCountV <= 4);
    static_assert(ColCountV >= 1 && ColCountV <= 4);

  public:
    using value_type = T;
    using RowType = Vec<T, ColCountV>;
    using ColType = Vec<T, RowCountV>;
    static constexpr u8 ROW_COUNT = RowCountV;
    static constexpr u8 COL_COUNT = ColCountV;

  private:
    RowType rows_[RowCountV];

  public:
    Matrix() : Matrix(Construct::diagonal, Vec<T, RowCountV>(1)){};

    template <typename U>
    Matrix(std::initializer_list<U> v)
    {
      T* f = &rows_[0][0];
      for (auto it = v.begin(); it != v.end(); ++it, ++f) {
        *f = static_cast<T>(*it);
      }
    }

    Matrix(const Matrix&) = default;

    Matrix(Matrix&&) noexcept = default;

    auto operator=(const Matrix&) -> Matrix& = default;

    auto operator=(Matrix&&) noexcept -> Matrix& = default;

    ~Matrix() = default;

    /// Construct Matrix from another Matrix with different dimensions.
    /// In HLSL/Slang, destination Matrix must be equal or smaller than source Matrix.
    /// In Falcor, destination Matrix can be larger than source Matrix (initialized with identity).
    template <u8 R, u8 C>
    Matrix(const Matrix<T, R, C>& other) // NOLINT
    {
      for (u8 r = 0; r < std::min(RowCountV, R); ++r) {
        std::memcpy(&rows_[r], &other.rows_[r], std::min(ColCountV, C) * sizeof(T));
      }
    }

    template <u8 R, u8 C>
    auto operator=(const Matrix<T, R, C>& other) -> Matrix&
    {
      for (u8 r = 0; r < std::min(RowCountV, R); ++r) {
        std::memcpy(&rows_[r], &other.rows_[r], std::min(ColCountV, C) * sizeof(T));
      }

      return *this;
    }

    [[nodiscard]]
    static auto Fill(T val) -> Matrix
    {
      return Matrix(Construct::fill, val);
    }

    [[nodiscard]]
    static auto Zeros() -> Matrix
    {
      return Matrix(Construct::fill, 0);
    }

    [[nodiscard]]
    static auto Identity() -> Matrix
    {
      return Matrix(Construct::diagonal, Vec<T, RowCountV>(1));
    }

    [[nodiscard]]
    static auto Diagonal(Vec<T, RowCountV> diagonal_vec) -> Matrix
    {
      return Matrix(Construct::diagonal, diagonal_vec);
    }

    template <typename... VecT>
    [[nodiscard]]
    static auto FromColumns(const VecT&... column_vecs) -> Matrix
    {
      static_assert((is_same_v<VecT, Vec<T, RowCountV>> && ...), "g type mismatch");
      static_assert(sizeof...(VecT) == ColCountV, "Number of column mismatch");
      return Matrix(Construct::from_columns, column_vecs...);
    }

    [[nodiscard]]
    static auto FromRowMajorData(const T* data)
    {
      return Matrix(Construct::from_row_major_data, data);
    }

    [[nodiscard]]
    static auto FromColumnMajorData(const T* data)
    {
      return Matrix(Construct::from_column_major_data, data);
    }

    auto data() -> T* { return &rows_[0][0]; }

    auto data() const -> const T* { return &rows_[0][0]; }

    auto operator[](u8 r) -> RowType&
    {
      SOUL_ASSERT(0, r < RowCountV);
      return rows_[r];
    }

    auto operator[](u8 r) const -> const RowType&
    {
      SOUL_ASSERT(0, r < RowCountV);
      return rows_[r];
    }

    [[nodiscard]]
    SOUL_ALWAYS_INLINE auto m(const usize row, const usize column) const -> T
    {
      return rows_[row][column];
    }

    SOUL_ALWAYS_INLINE auto m(const usize row, const usize column) -> T&
    {
      return rows_[row][column];
    }

    auto row(u8 r) const -> RowType
    {
      SOUL_ASSERT(0, r < RowCountV);
      return rows_[r];
    }

    void set_row(u8 r, const RowType& v)
    {
      SOUL_ASSERT(0, r < RowCountV);
      rows_[r] = v;
    }

    auto col(u8 col) const -> ColType
    {
      SOUL_ASSERT(0, col < ColCountV);
      ColType result;
      for (u8 r = 0; r < RowCountV; ++r) {
        result[r] = rows_[r][col];
      }
      return result;
    }

    void set_col(u8 col, const ColType& v)
    {
      SOUL_ASSERT(0, col < ColCountV);
      for (u8 r = 0; r < RowCountV; ++r) {
        rows_[r][col] = v[r];
      }
    }

    auto operator==(const Matrix& rhs) const -> b8
    {
      return std::memcmp(this, &rhs, sizeof(*this)) == 0;
    }

  private:
    struct Construct {
      struct Fill {
      };
      struct Diagonal {
      };
      struct FromColumns {
      };
      struct FromRowMajorData {
      };
      struct FromColumnMajorData {
      };

      static constexpr auto fill = Fill{};
      static constexpr auto diagonal = Diagonal{};
      static constexpr auto from_columns = FromColumns{};
      static constexpr auto from_row_major_data = FromRowMajorData{};
      static constexpr auto from_column_major_data = FromColumnMajorData{};
    };

    explicit Matrix(Construct::Fill /* tag */, T val)
    {
      for (usize row_idx = 0; row_idx < RowCountV; row_idx++) {
        for (usize col_idx = 0; col_idx < ColCountV; col_idx++) {
          m(row_idx, col_idx) = val;
        }
      }
    }

    explicit Matrix(Construct::Diagonal /* tag */, Vec<T, RowCountV> val)
      requires(RowCountV == ColCountV)
        : Matrix(Construct::fill, 0)
    {
      for (usize diagonal_idx = 0; diagonal_idx < RowCountV; diagonal_idx++) {
        m(diagonal_idx, diagonal_idx) = val[diagonal_idx];
      }
    }

    template <typename... VecTs>
    explicit Matrix(Construct::FromColumns /* tag */, const VecTs&... column_vecs)
    {
      u8 column_idx = 0;
      (set_col(column_idx++, column_vecs), ...);
    }

    explicit Matrix(Construct::FromRowMajorData /* tag */, const T* data)
    {
      for (usize row_idx = 0; row_idx < RowCountV; row_idx++) {
        for (usize col_idx = 0; col_idx < ColCountV; col_idx++) {
          m(row_idx, col_idx) = data[row_idx * ColCountV + col_idx];
        }
      }
    }

    explicit Matrix(Construct::FromColumnMajorData /* tag */, const T* data)
    {
      for (usize col_idx = 0; col_idx < ColCountV; col_idx++) {
        for (usize row_idx = 0; row_idx < RowCountV; row_idx++) {
          m(row_idx, col_idx) = data[col_idx * RowCountV + row_idx];
        }
      }
    }
  };

  inline namespace builtin
  {
    template <typename T>
    using mat4 = Matrix<T, 4, 4>;

    using mat2i16 = Matrix<i16, 2, 2>;
    using mat3i16 = Matrix<i16, 3, 3>;
    using mat4i16 = Matrix<i16, 4, 4>;

    using mat2u16 = Matrix<u16, 2, 2>;
    using mat3u16 = Matrix<u16, 3, 3>;
    using mat4u16 = Matrix<u16, 4, 4>;

    using mat2i32 = Matrix<i32, 2, 2>;
    using mat3i32 = Matrix<i32, 3, 3>;
    using mat4i32 = Matrix<i32, 4, 4>;

    using mat2u32 = Matrix<u32, 2, 2>;
    using mat3u32 = Matrix<u32, 3, 3>;
    using mat4u32 = Matrix<u32, 4, 4>;

    using mat2i64 = Matrix<i64, 2, 2>;
    using mat3i64 = Matrix<i64, 3, 3>;
    using mat4i64 = Matrix<i64, 4, 4>;

    using mat2u64 = Matrix<u64, 2, 2>;
    using mat3u64 = Matrix<u64, 3, 3>;
    using mat4u64 = Matrix<u64, 4, 4>;

    using mat2f32 = Matrix<f32, 2, 2>;
    using mat3f32 = Matrix<f32, 3, 3>;
    using mat4f32 = Matrix<f32, 4, 4>;

    using mat2f64 = Matrix<f64, 2, 2>;
    using mat3f64 = Matrix<f64, 3, 3>;
    using mat4f64 = Matrix<f64, 4, 4>;

  } // namespace builtin

  // ----------------------------------------------------------------------------
  // Binary operators (component-wise)
  // ----------------------------------------------------------------------------

  /// Binary * operator
  template <typename T, u8 R, u8 C>
  [[nodiscard]]
  auto
  operator*(const Matrix<T, R, C>& lhs, const T& rhs) -> Matrix<T, R, C>
  {
    Matrix<T, R, C> result;
    for (u8 r = 0; r < R; ++r) {
      for (u8 c = 0; c < C; ++c) {
        result[r][c] = lhs[r][c] * rhs;
      }
    }
    return result;
  }

  /// Binary * operator
  template <typename T, u8 R, u8 C>
  [[nodiscard]]
  auto
  operator+(const Matrix<T, R, C>& lhs, const Matrix<T, R, C>& rhs) -> Matrix<T, R, C>
  {
    Matrix<T, R, C> result;
    for (u8 r = 0; r < R; ++r) {
      for (u8 c = 0; c < C; ++c) {
        result[r][c] = lhs[r][c] + rhs[r][c];
      }
    }
    return result;
  }
} // namespace soul
