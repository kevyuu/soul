#pragma once

#include "core/type_traits.h"

#include <memory>

namespace soul
{
  template <typename F>
  struct Generator {
    F f;

    using FuncType = F;
    using ReturnType = decltype(f());

    constexpr explicit inline Generator(F f) noexcept : f(f) {}
    constexpr Generator(const Generator& f) = delete;
    constexpr Generator(Generator&& f) noexcept = default;
    constexpr auto operator=(const Generator& f) -> Generator& = delete;
    constexpr auto operator=(Generator&& f) noexcept -> Generator& = default;
    constexpr ~Generator() noexcept = default;

    constexpr inline operator ReturnType() && // NOLINT(hicpp-explicit-conversions)
    {
      return std::move(f)();
    }
  };

  template <typename T>
  auto duplicate(const T& val) -> T
  {
    if constexpr (ts_clone<T>) {
      return val.clone();
    } else {
      return val;
    }
  }

  template <typeset T>
  void duplicate_from(T& dst, const T& src)
  {
    if constexpr (can_clone_v<T>) {
      dst.clone_from(src);
    } else {
      dst = src;
    }
  }

  template <typename T>
  auto duplicate_fn(const T& val)
  {
    return [&val] { return duplicate(val); };
  }

  template <ts_clone T>
  auto clone_fn(const T& val)
  {
    return [&val] { return val.clone(); };
  }

  template <class T>
  inline constexpr void destroy_at(T* const p) noexcept
  {
    if constexpr (!std::is_trivially_destructible_v<T>) {
      if constexpr (std::is_array_v<T>) {
        for (auto& elem : *p) {
          (destroy_at)(std::addressof(elem));
        }
      } else {
        p->~T();
      }
    }
  }

  template <typename T, typename... Args>
  inline constexpr void construct_at(T* const location, Args&&... args) noexcept
  {
    if (std::is_constant_evaluated()) {
      std::construct_at(location, std::forward<Args>(args)...);
    } else {
      new (location) T(std::forward<Args>(args)...);
    }
  }

  template <typename T>
  inline constexpr void clone_at(T* const location, const T& item) noexcept
  {
    if (std::is_constant_evaluated()) {
      std::construct_at(location, Generator([&] { return item.clone(); }));
    } else {
      new (location) T(item.clone());
    }
  }

  template <typename T>
  inline constexpr void duplicate_at(T* const location, const T& item) noexcept
  {
    if constexpr (can_clone_v<T>) {
      clone_at(location, item);
    } else {
      construct_at(location, item);
    }
  }

  template <typename T, std::input_iterator IteratorT>
  inline constexpr void uninitialized_clone_n(IteratorT src_it, size_t size, T* dst) noexcept
  {
    if (std::is_constant_evaluated()) {
      for (; size != 0; ++src_it, ++dst, (void)--size) {
        std::construct_at(dst, Generator([&] { return dst->clone(); }));
      }
    } else {
      for (; size != 0; ++src_it, ++dst, (void)--size) {
        new (dst) T(src_it->clone());
      }
    }
  }

  template <class T, ts_generate_fn<T> Fn>
  inline constexpr void generate_at(T* const location, Fn fn) noexcept
  {
    if (std::is_constant_evaluated()) {
      std::construct_at(location, Generator(fn));
    } else {
      new (location) T(std::invoke(fn));
    }
  }

  template <class T, ts_generate_fn<T> Fn>
  inline constexpr void uninitialized_generate_n(Fn fn, size_t size, T* dst) noexcept
  {
    if (std::is_constant_evaluated()) {
      for (; size != 0; ++dst, (void)--size) {
        std::construct_at(dst, Generator(fn));
      }
    } else {
      for (; size != 0; ++dst, (void)--size) {
        new (dst) T(std::invoke(fn));
      }
    }
  }

  template <typename T, std::input_iterator IteratorT, ts_fn<T, std::iter_value_t<IteratorT>> Fn>
  inline constexpr void transform_construct_at(T* const location, IteratorT it, Fn fn) noexcept
  {
    if (std::is_constant_evaluated()) {
      std::construct_at(
        &location, Generator([fn = std::move(fn), it] { return std::invoke(fn, *it); }));
    } else {
      new (&location) T(std::invoke(fn, *it));
    }
  }

  template <class T, std::input_iterator IteratorT, ts_fn<T, std::iter_value_t<IteratorT>> Fn>
  inline constexpr void uninitialized_transform_construct_n(
    IteratorT src_it, Fn fn, size_t size, T* dst) noexcept
  {
    if (std::is_constant_evaluated()) {
      for (; size != 0; ++src_it, ++dst, (void)--size) {
        std::construct_at(
          dst, Generator([fn = std::move(fn), &src_it] { return std::invoke(fn, *src_it); }));
      }
    } else {
      for (; size != 0; ++src_it, ++dst, (void)--size) {
        new (dst) T(std::invoke(fn, *src_it));
      }
    }
  }

  template <typename T>
  inline constexpr void uninitialized_value_construct_n(T* dst, size_t size)
  {
    if (std::is_constant_evaluated()) {
      for (; size != 0; ++dst, (void)--size) {
        std::construct_at(dst);
      }
    } else {
      std::uninitialized_value_construct_n(dst, size);
    }
  }

  template <typename T, std::input_iterator InputIteratorT>
  inline constexpr void uninitialized_copy_n(InputIteratorT src_it, size_t size, T* dst)
  {
    if (std::is_constant_evaluated()) {
      for (; size != 0; ++src_it, ++dst, (void)--size) {
        std::construct_at(dst, *src_it);
      }
    } else {
      std::uninitialized_copy_n(std::move(src_it), size, dst);
    }
  }

  template <typename T, std::input_iterator InputIteratorT>
  inline constexpr void uninitialized_move_n(InputIteratorT src_it, size_t size, T* dst)
  {
    if (std::is_constant_evaluated()) {
      for (; size != 0; ++src_it, ++dst, (void)--size) {
        std::construct_at(dst, std::move(*src_it));
      }
    } else {
      std::uninitialized_move_n(std::move(src_it), size, dst);
    }
  }

} // namespace soul
