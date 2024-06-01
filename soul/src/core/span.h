#pragma once

#include "core/not_null.h"
#include "core/option.h"
#include "core/panic.h"
#include "core/type_traits.h"

namespace soul
{
  struct NilSpan
  {
  };

  constexpr auto nilspan = NilSpan{};

  template <ts_pointer T, ts_unsigned_integral SizeT = usize>
  class Span
  {
  public:
    using value_type             = remove_pointer_t<T>;
    using pointer                = value_type*;
    using const_pointer          = const value_type*;
    using reference              = value_type&;
    using const_reference        = const value_type&;
    using iterator               = value_type*;
    using const_iterator         = const value_type*;
    using reverse_iterator       = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;
    using size_type              = SizeT;

    Span() = delete;

    constexpr explicit Span(const char* data)
      requires(same_as<T, const char*>)
        : data_(data == nullptr ? "" : data), size_(data == nullptr ? 0 : strlen(data))
    {
    }

    [[nodiscard]]
    constexpr auto is_null_terminated() const -> b8
      requires(same_as<T, const char*>)
    {
      return data_[size_] == '\0';
    }

    constexpr Span(MaybeNull<pointer> data, size_type size) : data_(data), size_(size)
    {
      SOUL_ASSERT(
        0, (size != 0 && data != nullptr) || (size == 0), "Non zero size cannot hold nullptr");
    }

    constexpr Span(NotNull<pointer> data, size_type size) : data_(data), size_(size) {}

    constexpr Span(pointer data, size_type size) : data_(data), size_(size)
    {
      SOUL_ASSERT(
        0, (size != 0 && data != nullptr) || (size == 0), "Non zero size cannot hold nullptr");
    }

    template <typename U>
    constexpr Span(Span<U, SizeT> other) // NOLINT
      requires(can_convert_v<U, T>)
        : data_(other.begin()), size_(other.size())
    {
    }

    constexpr Span(NilSpan /* nil */) : Span(nullptr, 0) {} // NOLINT

    [[nodiscard]]
    constexpr auto begin() -> iterator
    {
      return data_;
    }

    [[nodiscard]]
    constexpr auto begin() const -> const_iterator
    {
      return data_;
    }

    [[nodiscard]]
    constexpr auto cbegin() const -> const_iterator
    {
      return data_;
    }

    [[nodiscard]]
    constexpr auto end() -> iterator
    {
      return data_ + size_;
    }

    [[nodiscard]]
    constexpr auto end() const -> const_iterator
    {
      return data_ + size_;
    }

    [[nodiscard]]
    constexpr auto cend() const -> const_iterator
    {
      return data_ + size_;
    }

    [[nodiscard]]
    constexpr auto rbegin() -> reverse_iterator
    {
      return reverse_iterator(end());
    }

    [[nodiscard]]
    constexpr auto rbegin() const -> const_reverse_iterator
    {
      return const_reverse_iterator(cend());
    }

    [[nodiscard]]
    constexpr auto crbegin() const -> const_reverse_iterator
    {
      return const_reverse_iterator(cend());
    }

    [[nodiscard]]
    constexpr auto rend() -> reverse_iterator
    {
      return reverse_iterator(begin());
    }

    [[nodiscard]]
    constexpr auto rend() const -> const_reverse_iterator
    {
      return const_reverse_iterator(cbegin());
    }

    [[nodiscard]]
    constexpr auto crend() const -> const_reverse_iterator
    {
      return const_reverse_iterator(cbegin());
    }

    constexpr auto operator[](usize idx) -> reference
    {
      SOUL_ASSERT_UPPER_BOUND_CHECK(idx, size_);
      return data_[idx];
    }

    constexpr auto operator[](usize idx) const -> const_reference
    {
      SOUL_ASSERT_UPPER_BOUND_CHECK(idx, size_);
      return data_[idx];
    }

    [[nodiscard]]
    constexpr auto data() noexcept -> MaybeNull<pointer>
    {
      return data_;
    }

    [[nodiscard]]
    constexpr auto data() const noexcept -> MaybeNull<const_pointer>
    {
      return data_;
    }

    [[nodiscard]]
    constexpr auto size() const noexcept -> size_type
    {
      return size_;
    }

    [[nodiscard]]
    constexpr auto size_in_bytes() const noexcept -> size_type
    {
      return size_ * sizeof(remove_pointer_t<pointer>);
    }

    friend constexpr void soul_op_hash_combine(auto& hasher, Span span)
    {
      hasher.combine_span(span);
    }

  private:
    pointer data_;
    size_type size_;
  };

  template <typename T, typename PointerT = match_any, typename SizeT = match_any>
  inline constexpr b8 is_span_v = []
  {
    if constexpr (is_specialization_v<T, Span>)
    {
      return is_match_v<PointerT, typename T::pointer> && is_match_v<SizeT, typename T::size_type>;
    }
    return false;
  }();

  template <typename T, typename PointerT = match_any, typename SizeT = match_any>
  concept ts_span = is_span_v<T, PointerT, SizeT>;

  template <typename T>
  [[nodiscard]]
  constexpr auto u8span(T* data, u8 size) -> Span<T*, u8>
  {
    return {data, size};
  }

  template <typename T>
  [[nodiscard]]
  constexpr auto u16span(T* data, u16 size) -> Span<T*, u16>
  {
    return {data, size};
  }

  template <typename T>
  [[nodiscard]]
  constexpr auto u32span(T* data, u32 size) -> Span<T*, u32>
  {
    return {data, size};
  }

  template <typename T>
  [[nodiscard]]
  constexpr auto u64span(T* data, u64 size) -> Span<T*, u64>
  {
    return {data, size};
  }

  template <typename T>
  [[nodiscard]]
  constexpr auto u8span(NotNull<T*> data, u8 size) -> Span<T*, u8>
  {
    return {data, size};
  }

  template <typename T>
  [[nodiscard]]
  constexpr auto u16span(NotNull<T*> data, u16 size) -> Span<T*, u16>
  {
    return {data, size};
  }

  template <typename T>
  [[nodiscard]]
  constexpr auto u32span(NotNull<T*> data, u32 size) -> Span<T*, u32>
  {
    return {data, size};
  }

  template <typename T>
  [[nodiscard]]
  constexpr auto u64span(NotNull<T*> data, u64 size) -> Span<T*, u64>
  {
    return {data, size};
  }

  template <typename T>
  [[nodiscard]]
  constexpr auto u8span(MaybeNull<T*> data, u8 size) -> Span<T*, u8>
  {
    return {data, size};
  }

  template <typename T>
  [[nodiscard]]
  constexpr auto u16span(MaybeNull<T*> data, u16 size) -> Span<T*, u16>
  {
    return {data, size};
  }

  template <typename T>
  [[nodiscard]]
  constexpr auto u32span(MaybeNull<T*> data, u32 size) -> Span<T*, u32>
  {
    return {data, size};
  }

  template <typename T>
  [[nodiscard]]
  constexpr auto u64span(MaybeNull<T*> data, u64 size) -> Span<T*, u64>
  {
    return {data, size};
  }

  template <typename T>
  [[nodiscard]]
  constexpr auto u8cspan(const T* data, u8 size) -> Span<const T*, u8>
  {
    return {data, size};
  }

  template <typename T>
  [[nodiscard]]
  constexpr auto u16cspan(const T* data, u16 size) -> Span<const T*, u16>
  {
    return {data, size};
  }

  template <typename T>
  [[nodiscard]]
  constexpr auto u32cspan(const T* data, u32 size) -> Span<const T*, u32>
  {
    return {data, size};
  }

  template <typename T>
  [[nodiscard]]
  constexpr auto u64cspan(const T* data, u64 size) -> Span<const T*, u64>
  {
    return {data, size};
  }

  template <typename T>
  [[nodiscard]]
  constexpr auto u8cspan(NotNull<const T*> data, u8 size) -> Span<const T*, u8>
  {
    return {data, size};
  }

  template <typename T>
  [[nodiscard]]
  constexpr auto u16cspan(NotNull<const T*> data, u16 size) -> Span<const T*, u16>
  {
    return {data, size};
  }

  template <typename T>
  [[nodiscard]]
  constexpr auto u32cspan(NotNull<const T*> data, u32 size) -> Span<const T*, u32>
  {
    return {data, size};
  }

  template <typename T>
  [[nodiscard]]
  constexpr auto u64cspan(NotNull<const T*> data, u64 size) -> Span<const T*, u64>
  {
    return {data, size};
  }

  template <typename T>
  [[nodiscard]]
  constexpr auto u8cspan(MaybeNull<const T*> data, u8 size) -> Span<const T*, u8>
  {
    return {data, size};
  }

  template <typename T>
  [[nodiscard]]
  constexpr auto u16cspan(MaybeNull<const T*> data, u16 size) -> Span<const T*, u16>
  {
    return {data, size};
  }

  template <typename T>
  [[nodiscard]]
  constexpr auto u32cspan(MaybeNull<const T*> data, u32 size) -> Span<const T*, u32>
  {
    return {data, size};
  }

  template <typename T>
  [[nodiscard]]
  constexpr auto u64cspan(MaybeNull<const T*> data, u64 size) -> Span<const T*, u64>
  {
    return {data, size};
  }

  template <typename T, typename Size1T, typename Size2T>
  [[nodiscard]]
  auto
  operator==(Span<T, Size1T> lhs, Span<T, Size2T> rhs) -> b8
  {
    if (lhs.size() != rhs.size())
    {
      return false;
    }
    for (usize i = 0; i < lhs.size(); i++)
    {
      if (lhs[i] != rhs[i])
      {
        return false;
      }
    }
    return true;
  }
} // namespace soul
