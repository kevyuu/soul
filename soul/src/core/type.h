#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>

#include "glm/vec2.hpp"
#include "glm/vec3.hpp"
#include "glm/vec4.hpp"

#include "core/builtins.h"
#include "core/panic.h"
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

  template <usize Dim, typename T>
  using vec = glm::vec<Dim, T, glm::defaultp>;

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
    using store_type = glm::mat<Column, Row, T, glm::defaultp>;
    store_type mat;

    constexpr matrix() = default;

    constexpr explicit matrix(T val) : mat(val) {}

    constexpr explicit matrix(store_type mat) : mat(mat) {}

    [[nodiscard]]
    SOUL_ALWAYS_INLINE auto m(const u8 row, const u8 column) const -> f32
    {
      return mat[column][row];
    }

    SOUL_ALWAYS_INLINE auto m(const u8 row, const u8 column) -> f32& { return mat[column][row]; }

    static constexpr auto identity() -> this_type { return this_type(store_type(1)); }
  };

  template <typename T>
  using matrix4x4 = matrix<4, 4, T>;

  using mat3f = matrix<3, 3, f32>;
  using mat4f = matrix<4, 4, f32>;

  template <ts_arithmetic T>
  struct Quaternion {
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

    constexpr Quaternion() noexcept : x(0), y(0), z(0), w(1) {}

    constexpr Quaternion(T x, T y, T z, T w) noexcept : x(x), y(y), z(z), w(w) {}

    constexpr Quaternion(vec3<T> xyz, T w) noexcept : x(xyz.x), y(xyz.y), z(xyz.z), w(w) {}

    constexpr explicit Quaternion(const T* val) noexcept
        : x(val[0]), y(val[1]), z(val[2]), w(val[3])
    {
    }
  };

  using Quaternionf = Quaternion<f32>;

  struct Transformf {
    vec3f position;
    vec3f scale;
    Quaternionf rotation;
  };

  struct AABB {
    vec3f min = vec3f(std::numeric_limits<f32>::max());
    vec3f max = vec3f(std::numeric_limits<f32>::lowest());

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
      SOUL_ASSERT(
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
    SOUL_ASSERT(
      0,
      static_cast<u64>(src) <= std::numeric_limits<IntegralDst>::max(),
      "Source value is larger than the destintation type maximum!");
    SOUL_ASSERT(
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

    auto operator==(const ID& other) const -> bool { return other.id == id; }
    auto operator!=(const ID& other) const -> bool { return other.id != id; }
    auto operator<(const ID& other) const -> bool { return id < other.id; }
    auto operator<=(const ID& other) const -> bool { return id <= other.id; }
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

      constexpr auto operator!=(const Iterator& other) const -> bool
      {
        return index_ != other.index_;
      }
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
