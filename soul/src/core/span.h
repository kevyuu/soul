#pragma once

#include "core/not_null.h"
#include "core/option.h"
#include "core/type_traits.h"

namespace soul
{

  template <ts_pointer T, ts_unsigned_integral SizeT>
  class Span
  {
  public:
    using value_type = remove_pointer_t<T>;
    using pointer = value_type*;
    using const_pointer = const value_type*;
    using reference = value_type&;
    using const_reference = const value_type&;
    using iterator = value_type*;
    using const_iterator = const value_type*;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;
    using size_type = SizeT;

    Span() = delete;

    Span(MaybeNull<pointer> data, size_type size) : data_(data), size_(size)
    {
      SOUL_ASSERT(
        0,
        (size != 0 && data != nullptr) || (size == 0 && data == nullptr),
        "Non zero size cannot hold nullptr, and zero size must be nullptr");
    }

    Span(NotNull<pointer> data, size_type size) : data_(data), size_(size)
    {
      SOUL_ASSERT(0, size != 0, "Span from not null cannot have zero count");
    }

    Span(pointer data, size_type size) : data_(data), size_(size)
    {
      SOUL_ASSERT(
        0,
        (size != 0 && data != nullptr) || (size == 0 && data == nullptr),
        "Non zero size cannot hold nullptr, and zero size must be nullptr");
    }

    template <typename U>
    Span(Span<U, SizeT> other) // NOLINT
      requires(can_convert_v<U, T>)
        : data_(other.begin()), size_(other.size())
    {
    }

    [[nodiscard]]
    auto begin() -> iterator
    {
      return data_;
    }

    [[nodiscard]]
    auto begin() const -> const_iterator
    {
      return data_;
    }

    [[nodiscard]]
    auto cbegin() const -> const_iterator
    {
      return data_;
    }

    [[nodiscard]]
    auto end() -> iterator
    {
      return data_ + size_;
    }

    [[nodiscard]]
    auto end() const -> const_iterator
    {
      return data_ + size_;
    }

    [[nodiscard]]
    auto cend() const -> const_iterator
    {
      return data_ + size_;
    }

    [[nodiscard]]
    auto rbegin() -> reverse_iterator
    {
      return reverse_iterator(end());
    }

    [[nodiscard]]
    auto rbegin() const -> const_reverse_iterator
    {
      return const_reverse_iterator(cend());
    }

    [[nodiscard]]
    auto crbegin() const -> const_reverse_iterator
    {
      return const_reverse_iterator(cend());
    }

    [[nodiscard]]
    auto rend() -> reverse_iterator
    {
      return reverse_iterator(begin());
    }

    [[nodiscard]]
    auto rend() const -> const_reverse_iterator
    {
      return const_reverse_iterator(cbegin());
    }

    [[nodiscard]]
    auto crend() const -> const_reverse_iterator
    {
      return const_reverse_iterator(cbegin());
    }

    [[nodiscard]]
    auto size() const noexcept -> size_type
    {
      return size_;
    }

  private:
    pointer data_;
    size_type size_;
  };

  template <typename T>
  [[nodiscard]]
  auto u8span(T* data, u8 size) -> Span<T*, u8>
  {
    return {data, size};
  }

  template <typename T>
  [[nodiscard]]
  auto u16span(T* data, u16 size) -> Span<T*, u16>
  {
    return {data, size};
  }

  template <typename T>
  [[nodiscard]]
  auto u32span(T* data, u32 size) -> Span<T*, u32>
  {
    return {data, size};
  }

  template <typename T>
  [[nodiscard]]
  auto u64span(T* data, u64 size) -> Span<T*, u64>
  {
    return {data, size};
  }

  template <typename T>
  [[nodiscard]]
  auto u8span(NotNull<T*> data, u8 size) -> Span<T*, u8>
  {
    return {data, size};
  }

  template <typename T>
  [[nodiscard]]
  auto u16span(NotNull<T*> data, u16 size) -> Span<T*, u16>
  {
    return {data, size};
  }

  template <typename T>
  [[nodiscard]]
  auto u32span(NotNull<T*> data, u32 size) -> Span<T*, u32>
  {
    return {data, size};
  }

  template <typename T>
  [[nodiscard]]
  auto u64span(NotNull<T*> data, u64 size) -> Span<T*, u64>
  {
    return {data, size};
  }

  template <typename T>
  [[nodiscard]]
  auto u8span(MaybeNull<T*> data, u8 size) -> Span<T*, u8>
  {
    return {data, size};
  }

  template <typename T>
  [[nodiscard]]
  auto u16span(MaybeNull<T*> data, u16 size) -> Span<T*, u16>
  {
    return {data, size};
  }

  template <typename T>
  [[nodiscard]]
  auto u32span(MaybeNull<T*> data, u32 size) -> Span<T*, u32>
  {
    return {data, size};
  }

  template <typename T>
  [[nodiscard]]
  auto u64span(MaybeNull<T*> data, u64 size) -> Span<T*, u64>
  {
    return {data, size};
  }

  template <typename T>
  [[nodiscard]]
  auto u8cspan(T* data, u8 size) -> Span<const T*, u8>
  {
    return {data, size};
  }

  template <typename T>
  [[nodiscard]]
  auto u16cspan(T* data, u16 size) -> Span<const T*, u16>
  {
    return {data, size};
  }

  template <typename T>
  [[nodiscard]]
  auto u32cspan(T* data, u32 size) -> Span<const T*, u32>
  {
    return {data, size};
  }

  template <typename T>
  [[nodiscard]]
  auto u64cspan(T* data, u64 size) -> Span<const T*, u64>
  {
    return {data, size};
  }

  template <typename T>
  [[nodiscard]]
  auto u8cspan(NotNull<T*> data, u8 size) -> Span<const T*, u8>
  {
    return {data, size};
  }

  template <typename T>
  [[nodiscard]]
  auto u16cspan(NotNull<T*> data, u16 size) -> Span<const T*, u16>
  {
    return {data, size};
  }

  template <typename T>
  [[nodiscard]]
  auto u32cspan(NotNull<T*> data, u32 size) -> Span<const T*, u32>
  {
    return {data, size};
  }

  template <typename T>
  [[nodiscard]]
  auto u64cspan(NotNull<T*> data, u64 size) -> Span<const T*, u64>
  {
    return {data, size};
  }

  template <typename T>
  [[nodiscard]]
  auto u8cspan(MaybeNull<T*> data, u8 size) -> Span<const T*, u8>
  {
    return {data, size};
  }

  template <typename T>
  [[nodiscard]]
  auto u16cspan(MaybeNull<T*> data, u16 size) -> Span<const T*, u16>
  {
    return {data, size};
  }

  template <typename T>
  [[nodiscard]]
  auto u32cspan(MaybeNull<T*> data, u32 size) -> Span<const T*, u32>
  {
    return {data, size};
  }

  template <typename T>
  [[nodiscard]]
  auto u64cspan(MaybeNull<T*> data, u64 size) -> Span<const T*, u64>
  {
    return {data, size};
  }
} // namespace soul
