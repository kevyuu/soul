#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>

#include <glm/glm.hpp>

#include "core/builtins.h"
#include "core/compiler.h"
#include "core/panic_lite.h"
#include "core/type_traits.h"

namespace soul
{

  constexpr usize ONE_KILOBYTE = 1024;
  constexpr usize ONE_MEGABYTE = 1024 * ONE_KILOBYTE;
  constexpr usize ONE_GIGABYTE = 1024 * ONE_MEGABYTE;

  template <typename T>
  concept ts_bit_block = std::unsigned_integral<T>;

  template <typename T, usize N>
  struct RawBuffer {
    alignas(T) std::byte buffer[sizeof(T) * N];

    auto data() -> T* { return reinterpret_cast<T*>(buffer); }

    auto data() const -> const T* { return reinterpret_cast<const T*>(buffer); }
  };

  template <typename T>
  struct RawBuffer<T, 0> {
    auto data() -> T* { return nullptr; }

    auto data() const -> const T* { return nullptr; }
  };

  template <usize dimension, typename T>
  struct vec;

  namespace impl
  {
    template <usize dimension, typename T>
    struct VecBase {

      constexpr auto operator+=(const vec<dimension, T>& other) -> vec<dimension, T>&
      {
        auto& vec = get_vec();
        vec.storage += other.storage;
        return vec;
      }

      constexpr auto operator-=(const vec<dimension, T>& other) -> vec<dimension, T>&
      {
        auto& vec = get_vec();
        vec.storage -= other.storage;
        return vec;
      }

      constexpr auto operator*=(const vec<dimension, T>& other) -> vec<dimension, T>&
      {
        auto& vec = get_vec();
        vec.storage *= other.storage;
        return vec;
      }

      constexpr auto operator*=(const T other) -> vec<dimension, T>&
      {
        auto& vec = get_vec();
        vec.storage *= other;
        return vec;
      }

      constexpr auto operator/=(const vec<dimension, T>& other) -> vec<dimension, T>&
      {
        auto& vec = get_vec();
        vec.storage /= other.storage_ref();
        return vec;
      }

      constexpr auto operator/=(const T other) -> vec<dimension, T>&
      {
        auto& vec = get_vec();
        vec.storage /= other;
        return vec;
      }

      [[nodiscard]]
      constexpr auto
      operator[](u8 index) const -> const T&
      {
        return get_vec().storage[index];
      }

      [[nodiscard]]
      constexpr auto
      operator[](u8 index) -> T&
      {
        return get_vec().storage[index];
      }

    private:
      [[nodiscard]]
      constexpr auto get_vec() -> vec<dimension, T>&
      {
        return static_cast<vec<dimension, T>&>(*this);
      }

      [[nodiscard]]
      constexpr auto get_vec() const -> const vec<dimension, T>&
      {
        return static_cast<const vec<dimension, T>&>(*this);
      }
    };
  } // namespace impl

  template <usize dimension, typename T>
  struct vec : public impl::VecBase<dimension, T> {
    using storage_type = glm::vec<dimension, T>;

    storage_type storage;

    constexpr vec() : storage() {}

    [[nodiscard]]
    static constexpr auto Zero() -> vec
    {
      return vec(Construct::from_storage, storage_type());
    }

    [[nodiscard]]
    static constexpr auto FromStorage(const storage_type& storage) -> vec
    {
      return vec(Construct::from_storage, storage);
    }

    [[nodiscard]]
    static constexpr auto FromData(const T* data) -> vec
    {
      const auto result = vec::Zero();
      for (usize idx = 0; idx < dimension; idx++) {
        result[idx] = data[idx];
      }
      return result;
    }

  private:
    struct Construct {
      struct FromStorage {
      };
      static constexpr auto from_storage = FromStorage{};
    };
    explicit vec(Construct::FromStorage /* tag */, const storage_type& storage) : storage(storage)
    {
    }
  };

  template <typename T>
  struct vec<2, T> : public impl::VecBase<2, T> {
    using storage_type = glm::vec<2, T>;

    union {
      storage_type storage;
      struct {
        T x, y;
      };
    };

    constexpr vec() : storage() {}
    constexpr vec(T x_in, T y_in) : storage(x_in, y_in) {}

    [[nodiscard]]
    static constexpr auto Zero() -> vec
    {
      return vec(Construct::from_storage, storage_type());
    }

    [[nodiscard]]
    static constexpr auto Fill(T val) -> vec
    {
      return vec(Construct::from_storage, storage_type(val));
    }

    [[nodiscard]]
    static constexpr auto FromStorage(const storage_type& storage) -> vec
    {
      return vec(Construct::from_storage, storage);
    }

    [[nodiscard]]
    static constexpr auto FromData(const T* data) -> vec
    {
      return vec(data[0], data[1]);
    }

  private:
    struct Construct {
      struct FromStorage {
      };
      static constexpr auto from_storage = FromStorage{};
    };
    explicit vec(Construct::FromStorage /* tag */, const storage_type& storage) : storage(storage)
    {
    }
  };

  template <typename T>
  struct vec<3, T> : public impl::VecBase<3, T> {
    using storage_type = glm::vec<3, T>;

    union {
      storage_type storage;
      struct {
        T x, y, z;
      };
      vec<2, T> xy;
    };

    constexpr vec() : storage() {}
    constexpr vec(T x_in, T y_in, T z_in) : storage(x_in, y_in, z_in) {}
    constexpr vec(vec<2, T> xy_in, T z_in) : storage(xy_in.x, xy_in.y, z_in) {}

    [[nodiscard]]
    static constexpr auto Zero() -> vec
    {
      return vec(Construct::from_storage, storage_type());
    }

    [[nodiscard]]
    static constexpr auto Fill(T val) -> vec
    {
      return vec(Construct::from_storage, storage_type(val));
    }

    [[nodiscard]]
    static constexpr auto FromStorage(const storage_type& storage) -> vec
    {
      return vec(Construct::from_storage, storage);
    }

    [[nodiscard]]
    static constexpr auto FromData(const T* data) -> vec
    {
      return vec(data[0], data[1], data[2]);
    }

  private:
    struct Construct {
      struct FromStorage {
      };
      static constexpr auto from_storage = FromStorage{};
    };
    constexpr explicit vec(Construct::FromStorage /* tag */, const storage_type& storage)
        : storage(storage)
    {
    }
  };

  template <typename T>
  struct vec<4, T> : public impl::VecBase<4, T> {
    using storage_type = glm::vec<4, T>;

    union {
      storage_type storage;
      struct {
        T x, y, z, w;
      };
      vec<3, T> xyz;
      vec<2, T> xy;
    };

    constexpr vec() : storage() {}
    constexpr vec(T x_in, T y_in, T z_in, T w_in) : storage(x_in, y_in, z_in, w_in) {}
    constexpr vec(vec<2, T> xy_in, T z_in, T w_in) : storage(xy_in.x, xy_in.y, z_in, w_in) {}
    constexpr vec(vec<3, T> xyz_in, T w_in) : storage(xyz_in.x, xyz_in.y, xyz_in.z, w_in) {}

    [[nodiscard]]
    static constexpr auto Zero() -> vec
    {
      return vec(Construct::Zero);
    }

    [[nodiscard]]
    static constexpr auto Fill(T val) -> vec
    {
      return vec(Construct::from_storage, storage_type(val));
    }

    [[nodiscard]]
    static constexpr auto FromStorage(const storage_type& storage) -> vec
    {
      return vec(Construct::from_storage, storage);
    }

    [[nodiscard]]
    static constexpr auto FromData(const T* data) -> vec
    {
      return vec(data[0], data[1], data[2]);
    }

  private:
    struct Construct {
      struct FromStorage {
      };
      static constexpr auto from_storage = FromStorage{};
    };
    constexpr explicit vec(Construct::FromStorage /* tag */, const storage_type& storage)
        : storage(storage)
    {
    }
  };

  template <usize dimension, typename T>
  constexpr auto operator==(const vec<dimension, T>& lhs, const vec<dimension, T>& rhs) -> b8
  {
    return lhs.storage == rhs.storage;
  }

  template <usize dimension, typename T>
  constexpr auto operator+(const vec<dimension, T>& lhs, const vec<dimension, T>& rhs)
    -> vec<dimension, T>
  {
    return vec<dimension, T>::FromStorage(lhs.storage + rhs.storage);
  }

  template <usize dimension, typename T>
  constexpr auto operator-(const vec<dimension, T>& lhs, const vec<dimension, T>& rhs)
    -> vec<dimension, T>
  {
    return vec<dimension, T>::FromStorage(lhs.storage - rhs.storage);
  }

  template <usize dimension, typename T>
  constexpr auto operator*(const vec<dimension, T>& lhs, const vec<dimension, T>& rhs)
    -> vec<dimension, T>
  {
    return vec<dimension, T>::FromStorage(lhs.storage * rhs.storage);
  }

  template <usize dimension, typename T>
  constexpr auto operator/(const vec<dimension, T>& lhs, const vec<dimension, T>& rhs)
    -> vec<dimension, T>
  {
    return vec<dimension, T>::FromStorage(lhs.storage / rhs.storage);
  }

  template <usize dimension, typename T>
  constexpr auto operator*(const vec<dimension, T>& lhs, T rhs) -> vec<dimension, T>
  {
    return vec<dimension, T>::FromStorage(lhs.storage * rhs);
  }

  template <usize dimension, typename T>
  constexpr auto operator/(const vec<dimension, T>& lhs, T rhs) -> vec<dimension, T>
  {
    return vec<dimension, T>::FromStorage(lhs.storage / rhs);
  }

  template <usize dimension, typename T>
  constexpr auto operator*(T lhs, const vec<dimension, T>& rhs) -> vec<dimension, T>
  {
    return vec<dimension, T>::FromStorage(lhs * rhs.storage);
  }

  template <usize dimension, typename T>
  constexpr auto operator/(T lhs, const vec<dimension, T>& rhs) -> vec<dimension, T>
  {
    return vec<dimension, T>::FromStorage(lhs / rhs.storage);
  }

  template <usize dimension, typename T>
  constexpr void soul_op_hash_combine(auto& hasher, const vec<dimension, T>& val)
  {
    const auto hash_combine = [&hasher, &val]<usize... idx>(std::index_sequence<idx...>) {
      ((hasher.combine(val[idx])), ...);
    };
    return hash_combine(std::make_index_sequence<dimension>());
  }

  template <typename T>
  using vec2 = vec<2, T>;

  template <typename T>
  using vec3 = vec<3, T>;

  template <typename T>
  using vec4 = vec<4, T>;

  using vec2f = vec2<f32>;
  using vec3f = vec3<f32>;
  using vec4f = vec4<f32>;

  using vec2d = vec2<f64>;
  using vec3d = vec3<f64>;
  using vec4d = vec4<f64>;

  using vec2i16 = vec2<i16>;
  using vec3i16 = vec3<i16>;
  using vec4i16 = vec4<i16>;

  using vec2ui32 = vec2<u32>;
  using vec3ui32 = vec3<u32>;
  using vec4ui32 = vec4<u32>;

  using vec2i32 = vec2<i32>;
  using vec3i32 = vec3<i32>;
  using vec4i32 = vec4<i32>;

  template <ts_arithmetic T>
  struct quat {
    union {
      struct {
        T x, y, z, w;
      }; // NOLINT(clang-diagnostic-nested-anon-types)
      vec4<T> xyzw;
      vec3<T> xyz;
      vec2<T> xy;

      struct {
        // NOLINT(clang-diagnostic-nested-anon-types)
        vec3<T> vector;
        T real;
      };

      T mem[4];
    };

    constexpr quat() noexcept : x(0), y(0), z(0), w(1) {}

    constexpr quat(T x, T y, T z, T w) noexcept : x(x), y(y), z(z), w(w) {}

    constexpr quat(vec3<T> xyz, T w) noexcept : x(xyz.x), y(xyz.y), z(xyz.z), w(w) {}

    [[nodiscard]]
    constexpr static auto FromData(const T* val) -> quat
    {
      return quat(val[0], val[1], val[2], val[3]);
    };
  };

  using quatf = quat<f32>;

  // A matrix to represent multiplication with column vector
  // Read this article if you are confused.
  // https://fgiesen.wordpress.com/2012/02/12/row-major-vs-column-major-row-vectors-vs-column-vectors/
  // Internally we use glm to store the matrix, glm use column major ordering
  // and multiply column vector (used by gilbert strang lecture).
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
      static_assert((is_same_v<VecT, vec<Row, T>> && ...), "Vector type mismatch");
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

  struct AABB {
    vec3f min = vec3f::Fill(std::numeric_limits<f32>::max());
    vec3f max = vec3f::Fill(std::numeric_limits<f32>::lowest());

    AABB() = default;

    AABB(const vec3f& min, const vec3f& max) noexcept : min{min}, max{max} {}

    [[nodiscard]]
    auto is_empty() const -> bool
    {
      return (min.x >= max.x || min.y >= max.y || min.z >= max.z);
    }

    [[nodiscard]]
    auto is_inside(const vec3f& point) const -> bool
    {
      return (point.x >= min.x && point.x <= max.x) && (point.y >= min.y && point.y <= max.y) &&
             (point.z >= min.z && point.z <= max.z);
    }

    struct Corners {
      static constexpr usize COUNT = 8;
      vec3f vertices[COUNT];
    };

    [[nodiscard]]
    auto get_corners() const -> Corners
    {
      return {
        vec3f(min.x, min.y, min.z),
        vec3f(min.x, min.y, max.z),
        vec3f(min.x, max.y, min.z),
        vec3f(min.x, max.y, max.z),
        vec3f(max.x, min.y, min.z),
        vec3f(max.x, min.y, max.z),
        vec3f(max.x, max.y, min.z),
        vec3f(max.x, max.y, max.z)};
    }

    [[nodiscard]]
    auto center() const -> vec3f
    {
      return (min + max) / 2.0f;
    }
  };

  template <typename PointerDst, typename PointerSrc>
    requires std::is_pointer_v<PointerDst> && std::is_pointer_v<PointerSrc>
  constexpr auto cast(PointerSrc srcPtr) -> PointerDst
  {
    using Dst = std::remove_pointer_t<PointerDst>;
    if constexpr (!std::is_same_v<PointerDst, void*>) {
      SOUL_ASSERT_LITE(
        0,
        reinterpret_cast<uptr>(srcPtr) % alignof(Dst) == 0,
        "Source pointer is not aligned in PointerDst alignment!");
    }
    // ReSharper disable once CppCStyleCast
    return (PointerDst)(srcPtr);
  }

  template <std::integral IntegralDst, std::integral IntegralSrc>
  constexpr auto cast(IntegralSrc src) -> IntegralDst
  {
    SOUL_ASSERT_LITE(
      0,
      static_cast<u64>(src) <= std::numeric_limits<IntegralDst>::max(),
      "Source value is larger than the destintation type maximum!");
    SOUL_ASSERT_LITE(
      0,
      static_cast<i64>(src) >= std::numeric_limits<IntegralDst>::min(),
      "Source value is smaller than the destination type minimum!");
    return static_cast<IntegralDst>(src);
  }

  template <typename PointerDst, typename PointerSrc>
    requires std::is_pointer_v<PointerDst> && std::is_pointer_v<PointerSrc>
  constexpr auto downcast(PointerSrc srcPtr) -> PointerDst
  {
    return static_cast<PointerDst>(srcPtr);
  }

  template <ts_scoped_enum E>
  constexpr auto to_underlying(E e) noexcept
  {
    return static_cast<std::underlying_type_t<E>>(e);
  }

  template <
    typename ResourceType,
    typename IDType,
    IDType NullValue = std::numeric_limits<IDType>::max()>
  struct ID {
    using store_type = IDType;
    IDType id;

    constexpr ID() : id(NullValue) {}

    constexpr explicit ID(IDType id) : id(id) {}

    template <std::integral Integral>
    constexpr explicit ID(Integral id) : id(soul::cast<IDType>(id))
    {
    }

    template <typename Pointer>
      requires std::is_pointer_v<Pointer>
    constexpr explicit ID(Pointer id) : id(id)
    {
    }

    friend auto operator<=>(const ID& lhs, const ID& rhs) = default;

    [[nodiscard]]
    auto is_null() const -> bool
    {
      return id == NullValue;
    }
    [[nodiscard]]
    auto is_valid() const -> bool
    {
      return id != NullValue;
    }

    static constexpr auto null() -> ID { return ID(NullValue); }

    friend void soul_op_hash_combine(auto& hasher, const ID& val) { hasher.combine(val.id); }
  };

  template <ts_flag Flag>
  class FlagIter
  {
    using store_type = std::underlying_type_t<Flag>;

  public:
    class Iterator
    {
    public:
      constexpr explicit Iterator(store_type index) : index_(index) {}

      constexpr auto operator++() -> Iterator
      {
        ++index_;
        return *this;
      }

      constexpr auto operator==(const Iterator& other) const -> bool = default;

      constexpr auto operator*() const -> Flag { return Flag(index_); }

    private:
      store_type index_;
    };

    [[nodiscard]]
    auto begin() const -> Iterator
    {
      return Iterator(0);
    }
    [[nodiscard]]
    auto end() const -> Iterator
    {
      return Iterator(to_underlying(Flag::COUNT));
    }

    constexpr FlagIter() = default;

    static auto Iterates() -> FlagIter { return FlagIter(); }

    static auto Count() -> u64 { return to_underlying(Flag::COUNT); }
  };

} // namespace soul
