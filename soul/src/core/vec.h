#pragma once

#include "core/boolean.h"
#include "core/floating_point.h"
#include "core/integer.h"

#include <glm/glm.hpp>

namespace soul
{
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

  inline namespace builtin
  {
    using vec2i16 = vec2<i16>;
    using vec3i16 = vec3<i16>;
    using vec4i16 = vec4<i16>;

    using vec2u16 = vec2<u16>;
    using vec3u16 = vec3<u16>;
    using vec4u16 = vec4<u16>;

    using vec2i32 = vec2<i32>;
    using vec3i32 = vec3<i32>;
    using vec4i32 = vec4<i32>;

    using vec2u32 = vec2<u32>;
    using vec3u32 = vec3<u32>;
    using vec4u32 = vec4<u32>;

    using vec2i64 = vec2<i64>;
    using vec3i64 = vec3<i64>;
    using vec4i64 = vec4<i64>;

    using vec2u64 = vec2<u64>;
    using vec3u64 = vec3<u64>;
    using vec4u64 = vec4<u64>;

    using vec2f32 = vec2<f32>;
    using vec3f32 = vec3<f32>;
    using vec4f32 = vec4<f32>;

    using vec2f64 = vec2<f64>;
    using vec3f64 = vec3<f64>;
    using vec4f64 = vec4<f64>;
  } // namespace builtin
} // namespace soul
