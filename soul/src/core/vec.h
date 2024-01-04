#pragma once

#include "core/boolean.h"
#include "core/floating_point.h"
#include "core/integer.h"
#include "core/type_traits.h"

namespace soul
{
  template <typename T, u8 DimensionV>
  struct Vec;

  template <typename T>
  struct Vec<T, 1>
  {
    static constexpr int DIMENSION = 1;
    using value_type               = T;

    union
    {
      T data[1];
      T x;
      T r;
      T s;
    };

    constexpr Vec(const Vec& other) noexcept = default;

    constexpr Vec(Vec&& other) noexcept = default;

    constexpr auto operator=(const Vec& other) -> Vec& = default;

    constexpr auto operator=(Vec&& other) noexcept -> Vec& = default;

    constexpr ~Vec() = default;

    constexpr Vec() noexcept = default;

    explicit constexpr Vec(T x) noexcept : x{x} {}

    template <typename U>
    explicit constexpr Vec(U x) noexcept : x{T(x)}
    {
    }

    template <typename U>
    constexpr explicit Vec(const Vec<U, 1>& other) noexcept : x{T(other.x)} {};

    [[nodiscard]]
    static constexpr auto FromData(const T* data) -> Vec
    {
      return Vec(data[0]);
    }

    [[nodiscard]]
    constexpr auto
    operator[](int index) noexcept -> T&
    {
      return data[index];
    }

    [[nodiscard]]
    constexpr auto
    operator[](int index) const noexcept -> const T&
    {
      return data[index];
    }
  };

  template <typename T>
  struct Vec<T, 2>
  {
    static constexpr u8 DIMENSION = 2;
    using value_type              = T;

    union
    {
      T data[2];

      struct
      {
        T x, y;
      };

      struct
      {
        T r, g;
      };

      struct
      {
        T s, t;
      };
    };

    constexpr Vec(const Vec& other) noexcept = default;

    constexpr Vec(Vec&& other) noexcept = default;

    constexpr auto operator=(const Vec& other) -> Vec& = default;

    constexpr auto operator=(Vec&& other) noexcept -> Vec& = default;

    constexpr ~Vec() = default;

    constexpr Vec() noexcept = default;

    constexpr Vec(T x, T y) noexcept : x{x}, y{y} {}

    explicit constexpr Vec(T scalar) noexcept : x{scalar}, y{scalar} {}

    template <typename U>
    explicit constexpr Vec(U scalar) noexcept : x{T(scalar)}, y{T(scalar)}
    {
    }

    template <typename X, typename Y>
    constexpr Vec(X x, Y y) noexcept : x{T(x)}, y{T(y)}
    {
    }

    template <typename U>
    constexpr explicit Vec(const Vec<U, 2>& other) noexcept : x{T(other.x)}, y{T(other.y)} {};

    [[nodiscard]]
    static constexpr auto FromData(const T* data) -> Vec
    {
      return Vec(data[0], data[1]);
    }

    [[nodiscard]]
    constexpr auto
    operator[](u8 index) noexcept -> T&
    {
      return data[index];
    }

    [[nodiscard]]
    constexpr auto
    operator[](u8 index) const noexcept -> const T&
    {
      return data[index];
    }

#include "vec2_swizzles.inl"
  };

  template <typename T>
  struct Vec<T, 3>
  {
    static constexpr u8 DIMENSION = 3;
    using value_type              = T;

    union
    {
      T data[3];

      struct
      {
        T x, y, z;
      };

      struct
      {
        T r, g, b;
      };

      struct
      {
        T s, t, p;
      };
    };

    constexpr Vec(const Vec& other) = default;

    constexpr Vec(Vec&& other) noexcept = default;

    constexpr auto operator=(const Vec& other) -> Vec& = default;

    constexpr auto operator=(Vec&& other) noexcept -> Vec& = default;

    constexpr ~Vec() = default;

    constexpr Vec() noexcept = default;

    constexpr Vec(T x, T y, T z) noexcept : x{x}, y{y}, z{z} {}

    explicit constexpr Vec(T scalar) noexcept : x{scalar}, y{scalar}, z{scalar} {}

    template <typename U>
    explicit constexpr Vec(U scalar) noexcept : x{T(scalar)}, y{T(scalar)}, z{T(scalar)}
    {
    }

    template <typename X, typename Y, typename Z>
    constexpr Vec(X x, Y y, Z z) noexcept : x{T(x)}, y{T(y)}, z{T(z)}
    {
    }

    template <typename XY, typename Z>
    constexpr Vec(Vec<XY, 2> xy, Z z) noexcept : x{T(xy.x)}, y{T(xy.y)}, z{T(z)}
    {
    }

    template <typename X, typename YZ>
    constexpr Vec(X x, Vec<YZ, 2> yz) noexcept : x{T(x)}, y{T(yz.x)}, z{T(yz.y)}
    {
    }

    template <typename U>
    constexpr explicit Vec(const Vec<U, 3>& other) noexcept
        : x{T(other.x)}, y{T(other.y)}, z{T(other.z)} {};

    [[nodiscard]]
    static constexpr auto FromData(const T* data) -> Vec
    {
      return Vec(data[0], data[1], data[2]);
    }

    [[nodiscard]]
    constexpr auto
    operator[](u8 index) noexcept -> T&
    {
      return data[index];
    }

    [[nodiscard]]
    constexpr auto
    operator[](u8 index) const noexcept -> const T&
    {
      return data[index];
    }

#include "vec3_swizzles.inl"
  };

  template <typename T>
  struct Vec<T, 4>
  {
    static constexpr u8 DIMENSION = 4;
    using value_type              = T;

    union
    {
      T data[4];

      struct
      {
        T x, y, z, w;
      };

      struct
      {
        T r, g, b, a;
      };

      struct
      {
        T s, t, p, q;
      };
    };

    constexpr Vec(const Vec& other) = default;

    constexpr Vec(Vec&& other) noexcept = default;

    constexpr auto operator=(const Vec& other) -> Vec& = default;

    constexpr auto operator=(Vec&& other) noexcept -> Vec& = default;

    constexpr ~Vec() = default;

    constexpr Vec() noexcept = default;

    constexpr Vec(T x, T y, T z, T w) noexcept : x{x}, y{y}, z{z}, w{w} {}

    explicit constexpr Vec(T scalar) noexcept : x{scalar}, y{scalar}, z{scalar}, w{scalar} {}

    template <typename U>
    explicit constexpr Vec(U scalar) noexcept
        : x{T(scalar)}, y{T(scalar)}, z{T(scalar)}, w{T(scalar)}
    {
    }

    template <typename X, typename Y, typename Z, typename W>
    constexpr Vec(X x, Y y, Z z, W w) noexcept : x{T(x)}, y{T(y)}, z{T(z)}, w{T(w)}
    {
    }

    template <typename XY, typename Z, typename W>
    constexpr Vec(Vec<XY, 2> xy, Z z, W w) noexcept : x{T(xy.x)}, y{T(xy.y)}, z{T(z)}, w{T(w)}
    {
    }

    template <typename X, typename YZ, typename W>
    constexpr Vec(X x, Vec<YZ, 2> yz, W w) noexcept : x{T(x)}, y{T(yz.x)}, z{T(yz.y)}, w{T(w)}
    {
    }

    template <typename X, typename Y, typename ZW>
    constexpr Vec(X x, Y y, Vec<ZW, 2> zw) noexcept : x{T(x)}, y{T(y)}, z{T(zw.x)}, w{T(zw.y)}
    {
    }

    template <typename XY, typename ZW>
    constexpr Vec(Vec<XY, 2> xy, Vec<ZW, 2> zw) noexcept
        : x{T(xy.x)}, y{T(xy.y)}, z{T(zw.x)}, w{T(zw.y)}
    {
    }

    template <typename XYZ, typename W>
    constexpr Vec(Vec<XYZ, 3> xyz, W w) noexcept : x{T(xyz.x)}, y{T(xyz.y)}, z{T(xyz.z)}, w{T(w)}
    {
    }

    template <typename X, typename YZW>
    constexpr Vec(X x, Vec<YZW, 3> yzw) noexcept : x{T(x)}, y{T(yzw.x)}, z{T(yzw.y)}, w{T(yzw.z)}
    {
    }

    template <typename U>
    constexpr explicit Vec(const Vec<U, 4>& other) noexcept
        : x{T(other.x)}, y{T(other.y)}, z{T(other.z)}, w{T(other.w)} {};

    [[nodiscard]]
    static constexpr auto FromData(const T* data) -> Vec
    {
      return Vec(data[0], data[1], data[2], data[3]);
    }

    [[nodiscard]]
    constexpr auto
    operator[](u8 index) noexcept -> T&
    {
      return data[index];
    }

    [[nodiscard]]
    constexpr auto
    operator[](u8 index) const noexcept -> const T&
    {
      return data[index];
    }

#include "vec4_swizzles.inl"
  };

  inline namespace builtin
  {
    template <typename T>
    using vec1 = Vec<T, 1>;

    template <typename T>
    using vec2 = Vec<T, 2>;

    template <typename T>
    using vec3 = Vec<T, 3>;

    template <typename T>
    using vec4 = Vec<T, 4>;

    using vec1i16 = vec1<i16>;
    using vec2i16 = vec2<i16>;
    using vec3i16 = vec3<i16>;
    using vec4i16 = vec4<i16>;

    using vec1u16 = vec1<u16>;
    using vec2u16 = vec2<u16>;
    using vec3u16 = vec3<u16>;
    using vec4u16 = vec4<u16>;

    using vec1i32 = vec1<i32>;
    using vec2i32 = vec2<i32>;
    using vec3i32 = vec3<i32>;
    using vec4i32 = vec4<i32>;

    using vec1u32 = vec1<u32>;
    using vec2u32 = vec2<u32>;
    using vec3u32 = vec3<u32>;
    using vec4u32 = vec4<u32>;

    using vec1i64 = vec1<i64>;
    using vec2i64 = vec2<i64>;
    using vec3i64 = vec3<i64>;
    using vec4i64 = vec4<i64>;

    using vec1u64 = vec1<u64>;
    using vec2u64 = vec2<u64>;
    using vec3u64 = vec3<u64>;
    using vec4u64 = vec4<u64>;

    using vec1f32 = vec1<f32>;
    using vec2f32 = vec2<f32>;
    using vec3f32 = vec3<f32>;
    using vec4f32 = vec4<f32>;

    using vec1f64 = vec1<f64>;
    using vec2f64 = vec2<f64>;
    using vec3f64 = vec3<f64>;
    using vec4f64 = vec4<f64>;

    using vec1b8 = vec1<b8>;
    using vec2b8 = vec2<b8>;
    using vec3b8 = vec3<b8>;
    using vec4b8 = vec4<b8>;
  } // namespace builtin

  // --------------------------------------------------------------------------------------
  // Unary operators
  // --------------------------------------------------------------------------------------

  template <ts_arithmetic T, u8 DimensionV>
  [[nodiscard]]
  constexpr auto
  operator+(const Vec<T, DimensionV> v) noexcept
  {
    return v;
  }

  template <ts_arithmetic T, u8 DimensionV>
  [[nodiscard]]
  constexpr auto
  operator-(const Vec<T, DimensionV> v) noexcept
  {
    if constexpr (DimensionV == 1)
    {
      return Vec<T, DimensionV>{-v.x};
    } else if constexpr (DimensionV == 2)
    {
      return Vec<T, DimensionV>{-v.x, -v.y};
    } else if constexpr (DimensionV == 3)
    {
      return Vec<T, DimensionV>{-v.x, -v.y, -v.z};
    } else
    {
      return Vec<T, DimensionV>{-v.x, -v.y, -v.z, -v.w};
    }
  }

  /// Unary not operator
  template <typename T, u8 DimensionV>
  [[nodiscard]]
  constexpr auto
  operator!(const Vec<T, DimensionV> v) noexcept
  {
    if constexpr (DimensionV == 1)
    {
      return vec1b8{!v.x};
    } else if constexpr (DimensionV == 2)
    {
      return vec2b8{!v.x, !v.y};
    } else if constexpr (DimensionV == 3)
    {
      return vec3b8{!v.x, !v.y, !v.z};
    } else
    {
      return vec4b8{!v.x, !v.y, !v.z, !v.w};
    }
  }

  /// Unary not operator
  template <ts_arithmetic T, u8 DimensionV>
  [[nodiscard]]
  constexpr auto
  operator~(const Vec<T, DimensionV> v) noexcept
  {
    if constexpr (DimensionV == 1)
    {
      return Vec<T, DimensionV>{~v.x};
    } else if constexpr (DimensionV == 2)
    {
      return Vec<T, DimensionV>{~v.x, ~v.y};
    } else if constexpr (DimensionV == 3)
    {
      return Vec<T, DimensionV>{~v.x, ~v.y, ~v.z};
    } else
    {
      return Vec<T, DimensionV>{~v.x, ~v.y, ~v.z, ~v.w};
    }
  }

  // --------------------------------------------------------------------------------------
  // Binary operators
  // --------------------------------------------------------------------------------------

  /// Binary + operator
  template <ts_arithmetic T, u8 DimensionV>
  [[nodiscard]]
  constexpr auto
  operator+(const Vec<T, DimensionV>& lhs, const Vec<T, DimensionV>& rhs) -> Vec<T, DimensionV>
  {
    if constexpr (DimensionV == 1)
    {
      return Vec<T, DimensionV>{lhs.x + rhs.x};
    } else if constexpr (DimensionV == 2)
    {
      return Vec<T, DimensionV>{lhs.x + rhs.x, lhs.y + rhs.y};
    } else if constexpr (DimensionV == 3)
    {
      return Vec<T, DimensionV>{lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z};
    } else if constexpr (DimensionV == 4)
    {
      return Vec<T, DimensionV>{lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z, lhs.w + rhs.w};
    }
  }

  /// Binary + operator
  template <ts_arithmetic T, u8 DimensionV>
  [[nodiscard]]
  constexpr auto
  operator+(const Vec<T, DimensionV>& lhs, T rhs) -> Vec<T, DimensionV>
  {
    return lhs + Vec<T, DimensionV>(rhs);
  }

  /// Binary + operator
  template <ts_arithmetic T, u8 DimensionV>
  [[nodiscard]]
  constexpr auto
  operator+(T lhs, const Vec<T, DimensionV>& rhs) -> Vec<T, DimensionV>
  {
    return Vec<T, DimensionV>(lhs) + rhs;
  }

  /// Binary - operator
  template <ts_arithmetic T, u8 DimensionV>
  [[nodiscard]]
  constexpr auto
  operator-(const Vec<T, DimensionV>& lhs, const Vec<T, DimensionV>& rhs) -> Vec<T, DimensionV>
  {
    if constexpr (DimensionV == 1)
    {
      return Vec<T, DimensionV>{lhs.x - rhs.x};
    } else if constexpr (DimensionV == 2)
    {
      return Vec<T, DimensionV>{lhs.x - rhs.x, lhs.y - rhs.y};
    } else if constexpr (DimensionV == 3)
    {
      return Vec<T, DimensionV>{lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z};
    } else if constexpr (DimensionV == 4)
    {
      return Vec<T, DimensionV>{lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z, lhs.w - rhs.w};
    }
  }

  /// Binary - operator
  template <ts_arithmetic T, u8 DimensionV>
  [[nodiscard]]
  constexpr auto
  operator-(const Vec<T, DimensionV>& lhs, T rhs) -> Vec<T, DimensionV>
  {
    return lhs - Vec<T, DimensionV>(rhs);
  }

  /// Binary - operator
  template <ts_arithmetic T, u8 DimensionV>
  [[nodiscard]]
  constexpr auto
  operator-(T lhs, const Vec<T, DimensionV>& rhs) -> Vec<T, DimensionV>
  {
    return Vec<T, DimensionV>(lhs) - rhs;
  }

  /// Binary * operator
  template <ts_arithmetic T, u8 DimensionV>
  [[nodiscard]]
  constexpr auto
  operator*(const Vec<T, DimensionV>& lhs, const Vec<T, DimensionV>& rhs) -> Vec<T, DimensionV>
  {
    if constexpr (DimensionV == 1)
    {
      return Vec<T, DimensionV>{lhs.x * rhs.x};
    } else if constexpr (DimensionV == 2)
    {
      return Vec<T, DimensionV>{lhs.x * rhs.x, lhs.y * rhs.y};
    } else if constexpr (DimensionV == 3)
    {
      return Vec<T, DimensionV>{lhs.x * rhs.x, lhs.y * rhs.y, lhs.z * rhs.z};
    } else if constexpr (DimensionV == 4)
    {
      return Vec<T, DimensionV>{lhs.x * rhs.x, lhs.y * rhs.y, lhs.z * rhs.z, lhs.w * rhs.w};
    }
  }

  /// Binary * operator
  template <ts_arithmetic T, u8 DimensionV>
  [[nodiscard]]
  constexpr auto
  operator*(const Vec<T, DimensionV>& lhs, T rhs) -> Vec<T, DimensionV>
  {
    return lhs * Vec<T, DimensionV>(rhs);
  }

  /// Binary * operator
  template <ts_arithmetic T, u8 DimensionV>
  [[nodiscard]]
  constexpr auto
  operator*(T lhs, const Vec<T, DimensionV>& rhs) -> Vec<T, DimensionV>
  {
    return Vec<T, DimensionV>(lhs) * rhs;
  }

  /// Binary / operator
  template <ts_arithmetic T, u8 DimensionV>
  [[nodiscard]]
  constexpr auto
  operator/(const Vec<T, DimensionV>& lhs, const Vec<T, DimensionV>& rhs) -> Vec<T, DimensionV>
  {
    if constexpr (DimensionV == 1)
    {
      return Vec<T, DimensionV>{lhs.x / rhs.x};
    } else if constexpr (DimensionV == 2)
    {
      return Vec<T, DimensionV>{lhs.x / rhs.x, lhs.y / rhs.y};
    } else if constexpr (DimensionV == 3)
    {
      return Vec<T, DimensionV>{lhs.x / rhs.x, lhs.y / rhs.y, lhs.z / rhs.z};
    } else if constexpr (DimensionV == 4)
    {
      return Vec<T, DimensionV>{lhs.x / rhs.x, lhs.y / rhs.y, lhs.z / rhs.z, lhs.w / rhs.w};
    }
  }

  /// Binary / operator
  template <ts_arithmetic T, u8 DimensionV>
  [[nodiscard]]
  constexpr auto
  operator/(const Vec<T, DimensionV>& lhs, T rhs) -> Vec<T, DimensionV>
  {
    return lhs / Vec<T, DimensionV>(rhs);
  }

  /// Binary / operator
  template <ts_arithmetic T, u8 DimensionV>
  [[nodiscard]]
  constexpr auto
  operator/(T lhs, const Vec<T, DimensionV>& rhs) -> Vec<T, DimensionV>
  {
    return Vec<T, DimensionV>(lhs) / rhs;
  }

  /// Binary % operator
  template <ts_integral T, u8 DimensionV>
  [[nodiscard]]
  constexpr auto
  operator%(const Vec<T, DimensionV>& lhs, const Vec<T, DimensionV>& rhs) -> Vec<T, DimensionV>
  {
    if constexpr (DimensionV == 1)
    {
      return Vec<T, DimensionV>{lhs.x % rhs.x};
    } else if constexpr (DimensionV == 2)
    {
      return Vec<T, DimensionV>{lhs.x % rhs.x, lhs.y % rhs.y};
    } else if constexpr (DimensionV == 3)
    {
      return Vec<T, DimensionV>{lhs.x % rhs.x, lhs.y % rhs.y, lhs.z % rhs.z};
    } else if constexpr (DimensionV == 4)
    {
      return Vec<T, DimensionV>{lhs.x % rhs.x, lhs.y % rhs.y, lhs.z % rhs.z, lhs.w % rhs.w};
    }
  }

  /// Binary % operator
  template <ts_integral T, u8 DimensionV>
  [[nodiscard]]
  constexpr auto
  operator%(const Vec<T, DimensionV>& lhs, T rhs) -> Vec<T, DimensionV>
  {
    return lhs % Vec<T, DimensionV>(rhs);
  }

  /// Binary % operator
  template <ts_integral T, u8 DimensionV>
  [[nodiscard]]
  constexpr auto
  operator%(T lhs, const Vec<T, DimensionV>& rhs) -> Vec<T, DimensionV>
  {
    return Vec<T, DimensionV>(lhs) % rhs;
  }

  /// Binary << operator
  template <ts_integral T, u8 DimensionV>
  [[nodiscard]]
  constexpr auto
  operator<<(const Vec<T, DimensionV>& lhs, const Vec<T, DimensionV>& rhs) -> Vec<T, DimensionV>
  {
    if constexpr (DimensionV == 1)
    {
      return Vec<T, DimensionV>{lhs.x << rhs.x};
    } else if constexpr (DimensionV == 2)
    {
      return Vec<T, DimensionV>{lhs.x << rhs.x, lhs.y << rhs.y};
    } else if constexpr (DimensionV == 3)
    {
      return Vec<T, DimensionV>{lhs.x << rhs.x, lhs.y << rhs.y, lhs.z << rhs.z};
    } else if constexpr (DimensionV == 4)
    {
      return Vec<T, DimensionV>{lhs.x << rhs.x, lhs.y << rhs.y, lhs.z << rhs.z, lhs.w << rhs.w};
    }
  }

  /// Binary << operator
  template <ts_integral T, u8 DimensionV>
  [[nodiscard]]
  constexpr auto
  operator<<(const Vec<T, DimensionV>& lhs, T rhs) -> Vec<T, DimensionV>
  {
    return lhs << Vec<T, DimensionV>(rhs);
  }

  /// Binary << operator
  template <ts_integral T, u8 DimensionV>
  [[nodiscard]]
  constexpr auto
  operator<<(T lhs, const Vec<T, DimensionV>& rhs) -> Vec<T, DimensionV>
  {
    return Vec<T, DimensionV>(lhs) << rhs;
  }

  /// Binary >> operator
  template <ts_integral T, u8 DimensionV>
  [[nodiscard]]
  constexpr auto
  operator>>(const Vec<T, DimensionV>& lhs, const Vec<T, DimensionV>& rhs) -> Vec<T, DimensionV>
  {
    if constexpr (DimensionV == 1)
    {
      return Vec<T, DimensionV>{lhs.x >> rhs.x};
    } else if constexpr (DimensionV == 2)
    {
      return Vec<T, DimensionV>{lhs.x >> rhs.x, lhs.y >> rhs.y};
    } else if constexpr (DimensionV == 3)
    {
      return Vec<T, DimensionV>{lhs.x >> rhs.x, lhs.y >> rhs.y, lhs.z >> rhs.z};
    } else if constexpr (DimensionV == 4)
    {
      return Vec<T, DimensionV>{lhs.x >> rhs.x, lhs.y >> rhs.y, lhs.z >> rhs.z, lhs.w >> rhs.w};
    }
  }

  /// Binary >> operator
  template <ts_integral T, u8 DimensionV>
  [[nodiscard]]
  constexpr auto
  operator>>(const Vec<T, DimensionV>& lhs, T rhs) -> Vec<T, DimensionV>
  {
    return lhs >> Vec<T, DimensionV>(rhs);
  }

  /// Binary >> operator
  template <ts_integral T, u8 DimensionV>
  [[nodiscard]]
  constexpr auto
  operator>>(T lhs, const Vec<T, DimensionV>& rhs) -> Vec<T, DimensionV>
  {
    return Vec<T, DimensionV>(lhs) >> rhs;
  }

  /// Binary | operator
  template <ts_integral T, u8 DimensionV>
  [[nodiscard]]
  constexpr auto
  operator|(const Vec<T, DimensionV>& lhs, const Vec<T, DimensionV>& rhs) -> Vec<T, DimensionV>
  {
    if constexpr (DimensionV == 1)
    {
      return Vec<T, DimensionV>{lhs.x | rhs.x};
    } else if constexpr (DimensionV == 2)
    {
      return Vec<T, DimensionV>{lhs.x | rhs.x, lhs.y | rhs.y};
    } else if constexpr (DimensionV == 3)
    {
      return Vec<T, DimensionV>{lhs.x | rhs.x, lhs.y | rhs.y, lhs.z | rhs.z};
    } else if constexpr (DimensionV == 4)
    {
      return Vec<T, DimensionV>{lhs.x | rhs.x, lhs.y | rhs.y, lhs.z | rhs.z, lhs.w | rhs.w};
    }
  }

  /// Binary | operator
  template <ts_integral T, u8 DimensionV>
  [[nodiscard]]
  constexpr auto
  operator|(const Vec<T, DimensionV>& lhs, T rhs) -> Vec<T, DimensionV>
  {
    return lhs | Vec<T, DimensionV>(rhs);
  }

  /// Binary | operator
  template <ts_integral T, u8 DimensionV>
  [[nodiscard]]
  constexpr auto
  operator|(T lhs, const Vec<T, DimensionV>& rhs) -> Vec<T, DimensionV>
  {
    return Vec<T, DimensionV>(lhs) | rhs;
  }

  /// Binary & operator
  template <ts_integral T, u8 DimensionV>
  [[nodiscard]]
  constexpr auto
  operator&(const Vec<T, DimensionV>& lhs, const Vec<T, DimensionV>& rhs) -> Vec<T, DimensionV>
  {
    if constexpr (DimensionV == 1)
    {
      return Vec<T, DimensionV>{lhs.x & rhs.x};
    } else if constexpr (DimensionV == 2)
    {
      return Vec<T, DimensionV>{lhs.x & rhs.x, lhs.y & rhs.y};
    } else if constexpr (DimensionV == 3)
    {
      return Vec<T, DimensionV>{lhs.x & rhs.x, lhs.y & rhs.y, lhs.z & rhs.z};
    } else if constexpr (DimensionV == 4)
    {
      return Vec<T, DimensionV>{lhs.x & rhs.x, lhs.y & rhs.y, lhs.z & rhs.z, lhs.w & rhs.w};
    }
  }

  /// Binary & operator
  template <ts_integral T, u8 DimensionV>
  [[nodiscard]]
  constexpr auto
  operator&(const Vec<T, DimensionV>& lhs, T rhs) -> Vec<T, DimensionV>
  {
    return lhs & Vec<T, DimensionV>(rhs);
  }

  /// Binary & operator
  template <ts_integral T, u8 DimensionV>
  [[nodiscard]]
  constexpr auto
  operator&(T lhs, const Vec<T, DimensionV>& rhs) -> Vec<T, DimensionV>
  {
    return Vec<T, DimensionV>(lhs) & rhs;
  }

  /// Binary ^ operator
  template <ts_integral T, u8 DimensionV>
  [[nodiscard]]
  constexpr auto
  operator^(const Vec<T, DimensionV>& lhs, const Vec<T, DimensionV>& rhs) -> Vec<T, DimensionV>
  {
    if constexpr (DimensionV == 1)
    {
      return Vec<T, DimensionV>{lhs.x ^ rhs.x};
    } else if constexpr (DimensionV == 2)
    {
      return Vec<T, DimensionV>{lhs.x ^ rhs.x, lhs.y ^ rhs.y};
    } else if constexpr (DimensionV == 3)
    {
      return Vec<T, DimensionV>{lhs.x ^ rhs.x, lhs.y ^ rhs.y, lhs.z ^ rhs.z};
    } else if constexpr (DimensionV == 4)
    {
      return Vec<T, DimensionV>{lhs.x ^ rhs.x, lhs.y ^ rhs.y, lhs.z ^ rhs.z, lhs.w ^ rhs.w};
    }
  }

  /// Binary ^ operator
  template <ts_integral T, u8 DimensionV>
  [[nodiscard]]
  constexpr auto
  operator^(const Vec<T, DimensionV>& lhs, T rhs) -> Vec<T, DimensionV>
  {
    return lhs ^ Vec<T, DimensionV>(rhs);
  }

  /// Binary ^ operator
  template <ts_integral T, u8 DimensionV>
  [[nodiscard]]
  constexpr auto
  operator^(T lhs, const Vec<T, DimensionV>& rhs) -> Vec<T, DimensionV>
  {
    return Vec<T, DimensionV>(lhs) ^ rhs;
  }

  /// Binary || operator
  template <same_as<b8> T, u8 DimensionV>
  [[nodiscard]]
  constexpr auto
  operator||(const Vec<T, DimensionV>& lhs, const Vec<T, DimensionV>& rhs)
  {
    if constexpr (DimensionV == 1)
    {
      return vec1b8{lhs.x || rhs.x};
    } else if constexpr (DimensionV == 2)
    {
      return vec2b8{lhs.x || rhs.x, lhs.y || rhs.y};
    } else if constexpr (DimensionV == 3)
    {
      return vec3b8{lhs.x || rhs.x, lhs.y || rhs.y, lhs.z || rhs.z};
    } else if constexpr (DimensionV == 4)
    {
      return vec4b8{lhs.x || rhs.x, lhs.y || rhs.y, lhs.z || rhs.z, lhs.w || rhs.w};
    }
  }

  /// Binary || operator
  template <same_as<b8> T, u8 DimensionV>
  [[nodiscard]]
  constexpr auto
  operator||(const Vec<T, DimensionV>& lhs, T rhs)
  {
    return lhs || Vec<T, DimensionV>(rhs);
  }

  /// Binary || operator
  template <same_as<b8> T, u8 DimensionV>
  [[nodiscard]]
  constexpr auto
  operator||(T lhs, const Vec<T, DimensionV>& rhs)
  {
    return Vec<T, DimensionV>(lhs) || rhs;
  }

  /// Binary && operator
  template <same_as<b8> T, u8 DimensionV>
  [[nodiscard]]
  constexpr auto
  operator&&(const Vec<T, DimensionV>& lhs, const Vec<T, DimensionV>& rhs)
  {
    if constexpr (DimensionV == 1)
    {
      return vec1b8{lhs.x && rhs.x};
    } else if constexpr (DimensionV == 2)
    {
      return vec2b8{lhs.x && rhs.x, lhs.y && rhs.y};
    } else if constexpr (DimensionV == 3)
    {
      return vec3b8{lhs.x && rhs.x, lhs.y && rhs.y, lhs.z && rhs.z};
    } else if constexpr (DimensionV == 4)
    {
      return vec4b8{lhs.x && rhs.x, lhs.y && rhs.y, lhs.z && rhs.z, lhs.w && rhs.w};
    }
  }

  /// Binary && operator
  template <same_as<b8> T, u8 DimensionV>
  [[nodiscard]]
  constexpr auto
  operator&&(const Vec<T, DimensionV>& lhs, T rhs)
  {
    return lhs && Vec<T, DimensionV>(rhs);
  }

  /// Binary && operator
  template <same_as<b8> T, u8 DimensionV>
  [[nodiscard]]
  constexpr auto
  operator&&(T lhs, const Vec<T, DimensionV>& rhs)
  {
    return Vec<T, DimensionV>(lhs) && rhs;
  }

  /// Binary == operator
  template <typename T, u8 DimensionV>
  [[nodiscard]]
  constexpr auto
  operator==(const Vec<T, DimensionV>& lhs, const Vec<T, DimensionV>& rhs)
  {
    if constexpr (DimensionV == 1)
    {
      return vec1b8{lhs.x == rhs.x};
    } else if constexpr (DimensionV == 2)
    {
      return vec2b8{lhs.x == rhs.x, lhs.y == rhs.y};
    } else if constexpr (DimensionV == 3)
    {
      return vec3b8{lhs.x == rhs.x, lhs.y == rhs.y, lhs.z == rhs.z};
    } else if constexpr (DimensionV == 4)
    {
      return vec4b8{lhs.x == rhs.x, lhs.y == rhs.y, lhs.z == rhs.z, lhs.w == rhs.w};
    }
  }

  /// Binary == operator
  template <typename T, u8 DimensionV>
  [[nodiscard]]
  constexpr auto
  operator==(const Vec<T, DimensionV>& lhs, T rhs)
  {
    return lhs == Vec<T, DimensionV>(rhs);
  }

  /// Binary == operator
  template <typename T, u8 DimensionV>
  [[nodiscard]]
  constexpr auto
  operator==(T lhs, const Vec<T, DimensionV>& rhs)
  {
    return Vec<T, DimensionV>(lhs) == rhs;
  }

  /// Binary != operator
  template <typename T, u8 DimensionV>
  [[nodiscard]]
  constexpr auto
  operator!=(const Vec<T, DimensionV>& lhs, const Vec<T, DimensionV>& rhs)
  {
    if constexpr (DimensionV == 1)
    {
      return vec1b8{lhs.x != rhs.x};
    } else if constexpr (DimensionV == 2)
    {
      return vec2b8{lhs.x != rhs.x, lhs.y != rhs.y};
    } else if constexpr (DimensionV == 3)
    {
      return vec3b8{lhs.x != rhs.x, lhs.y != rhs.y, lhs.z != rhs.z};
    } else if constexpr (DimensionV == 4)
    {
      return vec4b8{lhs.x != rhs.x, lhs.y != rhs.y, lhs.z != rhs.z, lhs.w != rhs.w};
    }
  }

  /// Binary != operator
  template <typename T, u8 DimensionV>
  [[nodiscard]]
  constexpr auto
  operator!=(const Vec<T, DimensionV>& lhs, T rhs)
  {
    return lhs != Vec<T, DimensionV>(rhs);
  }

  /// Binary != operator
  template <typename T, u8 DimensionV>
  [[nodiscard]]
  constexpr auto
  operator!=(T lhs, const Vec<T, DimensionV>& rhs)
  {
    return Vec<T, DimensionV>(lhs) != rhs;
  }

  /// Binary < operator
  template <ts_arithmetic T, u8 DimensionV>
  [[nodiscard]]
  constexpr auto
  operator<(const Vec<T, DimensionV>& lhs, const Vec<T, DimensionV>& rhs)
  {
    if constexpr (DimensionV == 1)
    {
      return vec1b8{lhs.x < rhs.x};
    } else if constexpr (DimensionV == 2)
    {
      return vec2b8{lhs.x < rhs.x, lhs.y < rhs.y};
    } else if constexpr (DimensionV == 3)
    {
      return vec3b8{lhs.x < rhs.x, lhs.y < rhs.y, lhs.z < rhs.z};
    } else if constexpr (DimensionV == 4)
    {
      return vec4b8{lhs.x < rhs.x, lhs.y < rhs.y, lhs.z < rhs.z, lhs.w < rhs.w};
    }
  }

  /// Binary < operator
  template <ts_arithmetic T, u8 DimensionV>
  [[nodiscard]]
  constexpr auto
  operator<(const Vec<T, DimensionV>& lhs, T rhs)
  {
    return lhs < Vec<T, DimensionV>(rhs);
  }

  /// Binary < operator
  template <ts_arithmetic T, u8 DimensionV>
  [[nodiscard]]
  constexpr auto
  operator<(T lhs, const Vec<T, DimensionV>& rhs)
  {
    return Vec<T, DimensionV>(lhs) < rhs;
  }

  /// Binary > operator
  template <ts_arithmetic T, u8 DimensionV>
  [[nodiscard]]
  constexpr auto
  operator>(const Vec<T, DimensionV>& lhs, const Vec<T, DimensionV>& rhs)
  {
    if constexpr (DimensionV == 1)
    {
      return vec1b8{lhs.x > rhs.x};
    } else if constexpr (DimensionV == 2)
    {
      return vec2b8{lhs.x > rhs.x, lhs.y > rhs.y};
    } else if constexpr (DimensionV == 3)
    {
      return vec3b8{lhs.x > rhs.x, lhs.y > rhs.y, lhs.z > rhs.z};
    } else if constexpr (DimensionV == 4)
    {
      return vec4b8{lhs.x > rhs.x, lhs.y > rhs.y, lhs.z > rhs.z, lhs.w > rhs.w};
    }
  }

  /// Binary > operator
  template <ts_arithmetic T, u8 DimensionV>
  [[nodiscard]]
  constexpr auto
  operator>(const Vec<T, DimensionV>& lhs, T rhs)
  {
    return lhs > Vec<T, DimensionV>(rhs);
  }

  /// Binary > operator
  template <ts_arithmetic T, u8 DimensionV>
  [[nodiscard]]
  constexpr auto
  operator>(T lhs, const Vec<T, DimensionV>& rhs)
  {
    return Vec<T, DimensionV>(lhs) > rhs;
  }

  /// Binary <= operator
  template <ts_arithmetic T, u8 DimensionV>
  [[nodiscard]]
  constexpr auto
  operator<=(const Vec<T, DimensionV>& lhs, const Vec<T, DimensionV>& rhs)
  {
    if constexpr (DimensionV == 1)
    {
      return vec1b8{lhs.x <= rhs.x};
    } else if constexpr (DimensionV == 2)
    {
      return vec2b8{lhs.x <= rhs.x, lhs.y <= rhs.y};
    } else if constexpr (DimensionV == 3)
    {
      return vec3b8{lhs.x <= rhs.x, lhs.y <= rhs.y, lhs.z <= rhs.z};
    } else if constexpr (DimensionV == 4)
    {
      return vec4b8{lhs.x <= rhs.x, lhs.y <= rhs.y, lhs.z <= rhs.z, lhs.w <= rhs.w};
    }
  }

  /// Binary <= operator
  template <ts_arithmetic T, u8 DimensionV>
  [[nodiscard]]
  constexpr auto
  operator<=(const Vec<T, DimensionV>& lhs, T rhs)
  {
    return lhs <= Vec<T, DimensionV>(rhs);
  }

  /// Binary <= operator
  template <ts_arithmetic T, u8 DimensionV>
  [[nodiscard]]
  constexpr auto
  operator<=(T lhs, const Vec<T, DimensionV>& rhs)
  {
    return Vec<T, DimensionV>(lhs) <= rhs;
  }

  /// Binary >= operator
  template <ts_arithmetic T, u8 DimensionV>
  [[nodiscard]]
  constexpr auto
  operator>=(const Vec<T, DimensionV>& lhs, const Vec<T, DimensionV>& rhs)
  {
    if constexpr (DimensionV == 1)
    {
      return vec1b8{lhs.x >= rhs.x};
    } else if constexpr (DimensionV == 2)
    {
      return vec2b8{lhs.x >= rhs.x, lhs.y >= rhs.y};
    } else if constexpr (DimensionV == 3)
    {
      return vec3b8{lhs.x >= rhs.x, lhs.y >= rhs.y, lhs.z >= rhs.z};
    } else if constexpr (DimensionV == 4)
    {
      return vec4b8{lhs.x >= rhs.x, lhs.y >= rhs.y, lhs.z >= rhs.z, lhs.w >= rhs.w};
    }
  }

  /// Binary >= operator
  template <ts_arithmetic T, u8 DimensionV>
  [[nodiscard]]
  constexpr auto
  operator>=(const Vec<T, DimensionV>& lhs, T rhs)
  {
    return lhs >= Vec<T, DimensionV>(rhs);
  }

  /// Binary >= operator
  template <ts_arithmetic T, u8 DimensionV>
  [[nodiscard]]
  constexpr auto
  operator>=(T lhs, const Vec<T, DimensionV>& rhs)
  {
    return Vec<T, DimensionV>(lhs) >= rhs;
  }

  /// += assignment operator
  template <ts_arithmetic T, u8 DimensionV>
  constexpr auto operator+=(Vec<T, DimensionV>& lhs, const Vec<T, DimensionV>& rhs)
    -> Vec<T, DimensionV>
  {
    lhs.x += rhs.x;
    if constexpr (DimensionV >= 2)
    {
      lhs.y += rhs.y;
    }
    if constexpr (DimensionV >= 3)
    {
      lhs.z += rhs.z;
    }
    if constexpr (DimensionV >= 4)
    {
      lhs.w += rhs.w;
    }
    return lhs;
  }

  /// += assignment operator
  template <ts_arithmetic T, u8 DimensionV>
  constexpr auto operator+=(Vec<T, DimensionV>& lhs, T rhs) -> Vec<T, DimensionV>
  {
    return (lhs += Vec<T, DimensionV>(rhs));
  }

  /// -= assignment operator
  template <ts_arithmetic T, u8 DimensionV>
  constexpr auto operator-=(Vec<T, DimensionV>& lhs, const Vec<T, DimensionV>& rhs)
    -> Vec<T, DimensionV>
  {
    lhs.x -= rhs.x;
    if constexpr (DimensionV >= 2)
    {
      lhs.y -= rhs.y;
    }
    if constexpr (DimensionV >= 3)
    {
      lhs.z -= rhs.z;
    }
    if constexpr (DimensionV >= 4)
    {
      lhs.w -= rhs.w;
    }
    return lhs;
  }

  /// -= assignment operator
  template <ts_arithmetic T, u8 DimensionV>
  constexpr auto operator-=(Vec<T, DimensionV>& lhs, T rhs) -> Vec<T, DimensionV>
  {
    return (lhs -= Vec<T, DimensionV>(rhs));
  }

  /// *= assignment operator
  template <ts_arithmetic T, u8 DimensionV>
  constexpr auto operator*=(Vec<T, DimensionV>& lhs, const Vec<T, DimensionV>& rhs)
    -> Vec<T, DimensionV>
  {
    lhs.x *= rhs.x;
    if constexpr (DimensionV >= 2)
    {
      lhs.y *= rhs.y;
    }
    if constexpr (DimensionV >= 3)
    {
      lhs.z *= rhs.z;
    }
    if constexpr (DimensionV >= 4)
    {
      lhs.w *= rhs.w;
    }
    return lhs;
  }

  /// *= assignment operator
  template <ts_arithmetic T, u8 DimensionV>
  constexpr auto operator*=(Vec<T, DimensionV>& lhs, T rhs) -> Vec<T, DimensionV>
  {
    return (lhs *= Vec<T, DimensionV>(rhs));
  }

  /// /= assignment operator
  template <ts_arithmetic T, u8 DimensionV>
  constexpr auto operator/=(Vec<T, DimensionV>& lhs, const Vec<T, DimensionV>& rhs)
    -> Vec<T, DimensionV>
  {
    lhs.x /= rhs.x;
    if constexpr (DimensionV >= 2)
    {
      lhs.y /= rhs.y;
    }
    if constexpr (DimensionV >= 3)
    {
      lhs.z /= rhs.z;
    }
    if constexpr (DimensionV >= 4)
    {
      lhs.w /= rhs.w;
    }
    return lhs;
  }

  /// /= assignment operator
  template <ts_arithmetic T, u8 DimensionV>
  constexpr auto operator/=(Vec<T, DimensionV>& lhs, T rhs) -> Vec<T, DimensionV>
  {
    return (lhs /= Vec<T, DimensionV>(rhs));
  }

  /// %= assignment operator
  template <same_as<b8> T, u8 DimensionV>
  constexpr auto operator%=(Vec<T, DimensionV>& lhs, const Vec<T, DimensionV>& rhs)
    -> Vec<T, DimensionV>
  {
    lhs.x %= rhs.x;
    if constexpr (DimensionV >= 2)
    {
      lhs.y %= rhs.y;
    }
    if constexpr (DimensionV >= 3)
    {
      lhs.z %= rhs.z;
    }
    if constexpr (DimensionV >= 4)
    {
      lhs.w %= rhs.w;
    }
    return lhs;
  }

  /// %= assignment operator
  template <same_as<b8> T, u8 DimensionV>
  constexpr auto operator%=(Vec<T, DimensionV>& lhs, T rhs) -> Vec<T, DimensionV>
  {
    return (lhs %= Vec<T, DimensionV>(rhs));
  }

  /// <<= assignment operator
  template <same_as<b8> T, u8 DimensionV>
  constexpr auto operator<<=(Vec<T, DimensionV>& lhs, const Vec<T, DimensionV>& rhs)
    -> Vec<T, DimensionV>
  {
    lhs.x <<= rhs.x;
    if constexpr (DimensionV >= 2)
    {
      lhs.y <<= rhs.y;
    }
    if constexpr (DimensionV >= 3)
    {
      lhs.z <<= rhs.z;
    }
    if constexpr (DimensionV >= 4)
    {
      lhs.w <<= rhs.w;
    }
    return lhs;
  }

  /// <<= assignment operator
  template <same_as<b8> T, u8 DimensionV>
  constexpr auto operator<<=(Vec<T, DimensionV>& lhs, T rhs) -> Vec<T, DimensionV>
  {
    return (lhs <<= Vec<T, DimensionV>(rhs));
  }

  /// >>= assignment operator
  template <same_as<b8> T, u8 DimensionV>
  constexpr auto operator>>=(Vec<T, DimensionV>& lhs, const Vec<T, DimensionV>& rhs)
    -> Vec<T, DimensionV>
  {
    lhs.x >>= rhs.x;
    if constexpr (DimensionV >= 2)
    {
      lhs.y >>= rhs.y;
    }
    if constexpr (DimensionV >= 3)
    {
      lhs.z >>= rhs.z;
    }
    if constexpr (DimensionV >= 4)
    {
      lhs.w >>= rhs.w;
    }
    return lhs;
  }

  /// >>= assignment operator
  template <same_as<b8> T, u8 DimensionV>
  constexpr auto operator>>=(Vec<T, DimensionV>& lhs, T rhs) -> Vec<T, DimensionV>
  {
    return (lhs >>= Vec<T, DimensionV>(rhs));
  }

  /// |= assignment operator
  template <same_as<b8> T, u8 DimensionV>
  constexpr auto operator|=(Vec<T, DimensionV>& lhs, const Vec<T, DimensionV>& rhs)
    -> Vec<T, DimensionV>
  {
    lhs.x |= rhs.x;
    if constexpr (DimensionV >= 2)
    {
      lhs.y |= rhs.y;
    }
    if constexpr (DimensionV >= 3)
    {
      lhs.z |= rhs.z;
    }
    if constexpr (DimensionV >= 4)
    {
      lhs.w |= rhs.w;
    }
    return lhs;
  }

  /// |= assignment operator
  template <same_as<b8> T, u8 DimensionV>
  constexpr auto operator|=(Vec<T, DimensionV>& lhs, T rhs) -> Vec<T, DimensionV>
  {
    return (lhs |= Vec<T, DimensionV>(rhs));
  }

  /// &= assignment operator
  template <same_as<b8> T, u8 DimensionV>
  constexpr auto operator&=(Vec<T, DimensionV>& lhs, const Vec<T, DimensionV>& rhs)
    -> Vec<T, DimensionV>
  {
    lhs.x &= rhs.x;
    if constexpr (DimensionV >= 2)
    {
      lhs.y &= rhs.y;
    }
    if constexpr (DimensionV >= 3)
    {
      lhs.z &= rhs.z;
    }
    if constexpr (DimensionV >= 4)
    {
      lhs.w &= rhs.w;
    }
    return lhs;
  }

  /// &= assignment operator
  template <same_as<b8> T, u8 DimensionV>
  constexpr auto operator&=(Vec<T, DimensionV>& lhs, T rhs) -> Vec<T, DimensionV>
  {
    return (lhs &= Vec<T, DimensionV>(rhs));
  }

  /// ^= assignment operator
  template <same_as<b8> T, u8 DimensionV>
  constexpr auto operator^=(Vec<T, DimensionV>& lhs, const Vec<T, DimensionV>& rhs)
    -> Vec<T, DimensionV>
  {
    lhs.x ^= rhs.x;
    if constexpr (DimensionV >= 2)
    {
      lhs.y ^= rhs.y;
    }
    if constexpr (DimensionV >= 3)
    {
      lhs.z ^= rhs.z;
    }
    if constexpr (DimensionV >= 4)
    {
      lhs.w ^= rhs.w;
    }
    return lhs;
  }

  /// ^= assignment operator
  template <same_as<b8> T, u8 DimensionV>
  constexpr auto operator^=(Vec<T, DimensionV>& lhs, T rhs) -> Vec<T, DimensionV>
  {
    return (lhs ^= Vec<T, DimensionV>(rhs));
  }

  // ----------------------------------------------------------------------------
  // Boolean reductions
  // ----------------------------------------------------------------------------
  // clang-format off
  [[nodiscard]] constexpr auto any(const vec1b8 v) -> b8 { return v.x; }
  [[nodiscard]] constexpr auto any(const vec2b8 v) -> b8 { return v.x || v.y; }
  [[nodiscard]] constexpr auto any(const vec3b8 v) -> b8 { return v.x || v.y || v.z; }
  [[nodiscard]] constexpr auto any(const vec4b8 v) -> b8 { return v.x || v.y || v.z || v.w; }

  [[nodiscard]] constexpr auto all(const vec1b8 v) -> b8 { return v.x; }
  [[nodiscard]] constexpr auto all(const vec2b8 v) -> b8 { return v.x && v.y; }
  [[nodiscard]] constexpr auto all(const vec3b8 v) -> b8 { return v.x && v.y && v.z; }
  [[nodiscard]] constexpr auto all(const vec4b8 v) -> b8 { return v.x && v.y && v.z && v.w; }

  [[nodiscard]] constexpr auto none(const vec1b8 v) -> b8 { return !any(v); }
  [[nodiscard]] constexpr auto none(const vec2b8 v) -> b8 { return !any(v); }
  [[nodiscard]] constexpr auto none(const vec3b8 v) -> b8 { return !any(v); }
  [[nodiscard]] constexpr auto none(const vec4b8 v) -> b8 { return !any(v); }

  // clang-format on

  template <typename T, u8 DimensionV>
  constexpr void soul_op_hash_combine(auto& hasher, const Vec<T, DimensionV>& val)
  {
    const auto hash_combine = [&hasher, &val]<usize... idx>(std::index_sequence<idx...>)
    {
      ((hasher.combine(val[idx])), ...);
    };
    return hash_combine(std::make_index_sequence<DimensionV>());
  }

} // namespace soul
