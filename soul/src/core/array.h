#pragma once

#include <algorithm>

#include "core/span.h"
#include "core/type.h"
#include "core/type_traits.h"

namespace soul
{

  static constexpr usize MAX_BRACE_INIT_SIZE = 32;

  template <typename T, usize element_count>
  struct Array {

    using this_type = Array<T, element_count>;
    using value_type = T;
    using pointer = T*;
    using const_pointer = const T*;
    using reference = T&;
    using const_reference = const T&;
    using iterator = T*;
    using const_iterator = const T*;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    [[nodiscard]]
    static constexpr auto init_fill(const T& val) -> this_type
      requires(element_count <= MAX_BRACE_INIT_SIZE);

    template <ts_generate_fn<T> Fn>
    [[nodiscard]]
    static constexpr auto init_generate(Fn fn) -> this_type
      requires(element_count <= MAX_BRACE_INIT_SIZE);

    template <ts_fn<T, usize> Fn>
    [[nodiscard]]
    static constexpr auto init_index_transform(Fn fn) -> this_type
      requires(element_count <= MAX_BRACE_INIT_SIZE);

    constexpr void swap(this_type& other) noexcept;

    friend constexpr void swap(this_type& a, this_type& b) noexcept { a.swap(b); }

    [[nodiscard]]
    constexpr auto clone() const -> this_type
      requires(ts_clone<T> && element_count <= MAX_BRACE_INIT_SIZE);

    constexpr void clone_from(const this_type& other)
      requires ts_clone<T>;

    [[nodiscard]]
    constexpr auto data() noexcept -> pointer;

    [[nodiscard]]
    constexpr auto data() const noexcept -> const_pointer;

    template <ts_unsigned_integral SpanSizeT>
    [[nodiscard]]
    auto span() -> Span<pointer, SpanSizeT>;

    template <ts_unsigned_integral SpanSizeT>
    [[nodiscard]]
    auto span() const -> Span<const_pointer, SpanSizeT>;

    template <ts_unsigned_integral SpanSizeT>
    [[nodiscard]]
    auto cspan() const -> Span<const_pointer, SpanSizeT>;

    [[nodiscard]]
    constexpr auto front() -> reference;

    [[nodiscard]]
    constexpr auto front() const -> const_reference;

    [[nodiscard]]
    constexpr auto back() noexcept -> reference;

    [[nodiscard]]
    constexpr auto back() const noexcept -> const_reference;

    [[nodiscard]]
    constexpr auto
    operator[](usize idx) -> reference;

    [[nodiscard]]
    constexpr auto
    operator[](usize idx) const -> const_reference;

    [[nodiscard]]
    constexpr auto size() const -> usize;

    [[nodiscard]]
    constexpr auto empty() const -> b8;

    [[nodiscard]]
    constexpr auto begin() -> iterator;

    [[nodiscard]]
    constexpr auto begin() const -> const_iterator;

    [[nodiscard]]
    constexpr auto cbegin() const -> const_iterator;

    [[nodiscard]]
    constexpr auto end() -> iterator;

    [[nodiscard]]
    constexpr auto end() const -> const_iterator;

    [[nodiscard]]
    constexpr auto cend() const -> const_iterator;

    [[nodiscard]]
    constexpr auto rbegin() -> reverse_iterator;

    [[nodiscard]]
    constexpr auto rbegin() const -> const_reverse_iterator;

    [[nodiscard]]
    constexpr auto crbegin() const -> const_reverse_iterator;

    [[nodiscard]]
    constexpr auto rend() -> reverse_iterator;

    [[nodiscard]]
    constexpr auto rend() const -> const_reverse_iterator;

    [[nodiscard]]
    constexpr auto crend() const -> const_reverse_iterator;

    T buffer_[element_count];
  };

  template <typename T, usize element_count>
  [[nodiscard]]
  constexpr auto Array<T, element_count>::init_fill(const T& val) -> this_type
    requires(element_count <= MAX_BRACE_INIT_SIZE)
  {
    const auto create_array =
      []<usize... idx>(std::index_sequence<idx...>, const T& val) -> this_type {
      return {(static_cast<void>(idx), val)...};
    };
    return create_array(std::make_index_sequence<element_count>(), val);
  }

  template <typename T, usize element_count>
  template <ts_generate_fn<T> Fn>
  [[nodiscard]]
  constexpr auto Array<T, element_count>::init_generate(Fn fn) -> this_type
    requires(element_count <= MAX_BRACE_INIT_SIZE)
  {
    const auto create_array = []<usize... idx>(std::index_sequence<idx...>, Fn fn) -> this_type {
      return {(static_cast<void>(idx), std::invoke(fn))...};
    };
    return create_array(std::make_index_sequence<element_count>(), fn);
  }

  template <typename T, usize element_count>
  template <ts_fn<T, usize> Fn>
  [[nodiscard]]
  constexpr auto Array<T, element_count>::init_index_transform(Fn fn) -> this_type
    requires(element_count <= MAX_BRACE_INIT_SIZE)
  {
    static_assert(element_count <= MAX_BRACE_INIT_SIZE);
    const auto create_array = []<usize... idx>(std::index_sequence<idx...>, Fn fn) -> this_type {
      return {(std::invoke(fn, idx))...};
    };
    return create_array(std::make_index_sequence<element_count>(), fn);
  }

  template <typename T, usize element_count>
  constexpr void Array<T, element_count>::swap(this_type& other) noexcept
  {
    using std::swap;
    for (usize i = 0; i < element_count; i++) {
      swap(buffer_[i], other.buffer_[i]);
    }
  }

  template <typename T, usize element_count>
  [[nodiscard]]
  constexpr auto Array<T, element_count>::clone() const -> this_type
    requires(ts_clone<T> && element_count <= MAX_BRACE_INIT_SIZE)
  {
    const auto create_array =
      []<usize... idx>(std::index_sequence<idx...>, const this_type& original) -> this_type {
      return {original.buffer_[idx].clone()...};
    };
    return create_array(std::make_index_sequence<element_count>(), *this);
  }

  template <typename T, usize element_count>
  constexpr void Array<T, element_count>::clone_from(const this_type& other)
    requires ts_clone<T>
  {
    for (usize i = 0; i < element_count; i++) {
      buffer_[i].clone_from(other.buffer[i]);
    }
  }

  template <typename T, usize element_count>
  constexpr auto Array<T, element_count>::data() noexcept -> pointer
  {
    return buffer_;
  }

  template <typename T, usize element_count>
  constexpr auto Array<T, element_count>::data() const noexcept -> const_pointer
  {
    return buffer_;
  }

  template <typename T, usize element_count>
  template <ts_unsigned_integral SpanSizeT>
  auto Array<T, element_count>::span() -> Span<pointer, SpanSizeT>
  {
    return {data(), cast<SpanSizeT>(size())};
  }

  template <typename T, usize element_count>
  template <ts_unsigned_integral SpanSizeT>
  auto Array<T, element_count>::span() const -> Span<const_pointer, SpanSizeT>
  {
    return {data(), cast<SpanSizeT>(size())};
  }

  template <typename T, usize element_count>
  template <ts_unsigned_integral SpanSizeT>
  auto Array<T, element_count>::cspan() const -> Span<const_pointer, SpanSizeT>
  {
    return {data(), cast<SpanSizeT>(size())};
  }

  template <typename T, usize element_count>
  constexpr auto Array<T, element_count>::front() -> reference
  {
    return buffer_[0];
  }

  template <typename T, usize element_count>
  constexpr auto Array<T, element_count>::front() const -> const_reference
  {
    return buffer_[0];
  }

  template <typename T, usize element_count>
  constexpr auto Array<T, element_count>::back() noexcept -> reference
  {
    return buffer_[element_count - 1];
  }

  template <typename T, usize element_count>
  constexpr auto Array<T, element_count>::back() const noexcept -> const_reference
  {
    return buffer_[element_count - 1];
  }

  template <typename T, usize element_count>
  constexpr auto Array<T, element_count>::operator[](usize idx) -> reference
  {
    SOUL_ASSERT(
      0,
      idx < element_count,
      "Out of bound access to array detected. idx = {}, _size = ",
      idx,
      element_count);
    return buffer_[idx];
  }

  template <typename T, usize element_count>
  constexpr auto Array<T, element_count>::operator[](usize idx) const -> const_reference
  {
    SOUL_ASSERT(
      0,
      idx < element_count,
      "Out of bound access to array detected. idx = {}, _size={}",
      idx,
      element_count);
    return buffer_[idx];
  }

  template <typename T, usize element_count>
  [[nodiscard]]
  constexpr auto Array<T, element_count>::size() const -> usize
  {
    return element_count;
  }

  template <typename T, usize element_count>
  [[nodiscard]]
  constexpr auto Array<T, element_count>::empty() const -> b8
  {
    return element_count == 0;
  }

  template <typename T, usize element_count>
  constexpr auto Array<T, element_count>::begin() -> iterator
  {
    return buffer_;
  }

  template <typename T, usize element_count>
  constexpr auto Array<T, element_count>::begin() const -> const_iterator
  {
    return buffer_;
  }

  template <typename T, usize element_count>
  constexpr auto Array<T, element_count>::cbegin() const -> const_iterator
  {
    return buffer_;
  }

  template <typename T, usize element_count>
  constexpr auto Array<T, element_count>::end() -> iterator
  {
    return buffer_ + element_count;
  }

  template <typename T, usize element_count>
  constexpr auto Array<T, element_count>::end() const -> const_iterator
  {
    return buffer_ + element_count;
  }

  template <typename T, usize element_count>
  constexpr auto Array<T, element_count>::cend() const -> const_iterator
  {
    return buffer_ + element_count;
  }

  template <typename T, usize element_count>
  constexpr auto Array<T, element_count>::rbegin() -> reverse_iterator
  {
    return reverse_iterator(end());
  }

  template <typename T, usize element_count>
  constexpr auto Array<T, element_count>::rbegin() const -> const_reverse_iterator
  {
    return const_reverse_iterator(cend());
  }

  template <typename T, usize element_count>
  constexpr auto Array<T, element_count>::crbegin() const -> const_reverse_iterator
  {
    return const_reverse_iterator(cend());
  }

  template <typename T, usize element_count>
  constexpr auto Array<T, element_count>::rend() -> reverse_iterator
  {
    return reverse_iterator(begin());
  }

  template <typename T, usize element_count>
  constexpr auto Array<T, element_count>::rend() const -> const_reverse_iterator
  {
    return const_reverse_iterator(cbegin());
  }

  template <typename T, usize element_count>
  constexpr auto Array<T, element_count>::crend() const -> const_reverse_iterator
  {
    return const_reverse_iterator(cbegin());
  }

  template <typename T>
  struct Array<T, 0> {

    using this_type = Array<T, 0>;
    using value_type = T;
    using pointer = T*;
    using const_pointer = const T*;
    using reference = T&;
    using const_reference = const T&;
    using iterator = T*;
    using const_iterator = const T*;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    [[nodiscard]]
    static constexpr auto init_fill(const T& /* val */) -> this_type
    {
      return this_type();
    }

    template <ts_generate_fn<T> Fn>
    [[nodiscard]]
    static constexpr auto init_generate(Fn /* fn */) -> this_type
    {
      return this_type();
    }

    template <ts_fn<T, usize> Fn>
    [[nodiscard]]
    static constexpr auto init_index_transform(Fn /* fn */) -> this_type
    {
      return this_type();
    }

    [[nodiscard]]
    constexpr auto clone() const -> this_type
      requires ts_clone<T>
    {
      return this_type();
    }

    constexpr void clone_from(const this_type& other)
      requires ts_clone<T>
    {
    }

    [[nodiscard]]
    constexpr auto data() noexcept -> pointer
    {
      return nullptr;
    }

    [[nodiscard]]
    constexpr auto data() const noexcept -> const_pointer
    {
      return nullptr;
    }

    [[nodiscard]]
    constexpr auto
    operator[](usize /* idx */) -> reference
    {
      return *data();
    }

    [[nodiscard]]
    constexpr auto
    operator[](usize /* idx */) const -> const_reference
    {
      return *data();
    }

    [[nodiscard]]
    constexpr auto size() const -> usize
    {
      return 0;
    }

    [[nodiscard]]
    constexpr auto empty() const -> b8
    {
      return true;
    }

    [[nodiscard]]
    constexpr auto begin() -> iterator
    {
      return nullptr;
    }

    [[nodiscard]]
    constexpr auto begin() const -> const_iterator
    {
      return nullptr;
    }

    [[nodiscard]]
    constexpr auto cbegin() const -> const_iterator
    {
      return nullptr;
    }

    [[nodiscard]]
    constexpr auto end() -> iterator
    {
      return nullptr;
    }

    [[nodiscard]]
    constexpr auto end() const -> const_iterator
    {
      return nullptr;
    }

    [[nodiscard]]
    constexpr auto cend() const -> const_iterator
    {
      return nullptr;
    }

    [[nodiscard]]
    constexpr auto rbegin() -> reverse_iterator
    {
      return nullptr;
    }

    [[nodiscard]]
    constexpr auto rbegin() const -> const_reverse_iterator
    {
      return nullptr;
    }

    [[nodiscard]]
    constexpr auto crbegin() const -> const_reverse_iterator
    {
      return nullptr;
    }

    [[nodiscard]]
    constexpr auto rend() -> reverse_iterator
    {
      return nullptr;
    }

    [[nodiscard]]
    constexpr auto rend() const -> const_reverse_iterator
    {
      return nullptr;
    }

    [[nodiscard]]
    constexpr auto crend() const -> const_reverse_iterator
    {
      return nullptr;
    }
  };
  template <class T, class... U>
  Array(T, U...) -> Array<T, 1 + sizeof...(U)>;

  template <typename T, usize ArrSizeV>
  constexpr auto operator==(const Array<T, ArrSizeV>& lhs, const Array<T, ArrSizeV>& rhs) -> b8
  {
    return std::ranges::equal(lhs, rhs);
  }

} // namespace soul
