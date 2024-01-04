#pragma once

#include <algorithm>

#include "core/panic.h"
#include "core/span.h"
#include "core/type.h"
#include "core/type_traits.h"

namespace soul
{

  static constexpr usize MAX_BRACE_INIT_SIZE = 32;

  template <typeset T, usize ArrSizeV>
  struct Array
  {

    using this_type              = Array<T, ArrSizeV>;
    using value_type             = T;
    using pointer                = T*;
    using const_pointer          = const T*;
    using reference              = T&;
    using const_reference        = const T&;
    using iterator               = T*;
    using const_iterator         = const T*;
    using reverse_iterator       = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    [[nodiscard]]
    static constexpr auto Fill(const T& val) -> this_type
      requires(ArrSizeV <= MAX_BRACE_INIT_SIZE);

    template <ts_generate_fn<T> Fn>
    [[nodiscard]]
    static constexpr auto Generate(Fn fn) -> this_type
      requires(ArrSizeV <= MAX_BRACE_INIT_SIZE);

    template <ts_fn<T, usize> Fn>
    [[nodiscard]]
    static constexpr auto TransformIndex(Fn fn) -> this_type
      requires(ArrSizeV <= MAX_BRACE_INIT_SIZE);

    constexpr void swap(this_type& other) noexcept;

    friend constexpr void swap(this_type& a, this_type& b) noexcept
    {
      a.swap(b);
    }

    [[nodiscard]]
    constexpr auto clone() const -> this_type
      requires(ts_clone<T> && ArrSizeV <= MAX_BRACE_INIT_SIZE);

    constexpr void clone_from(const this_type& other)
      requires ts_clone<T>;

    [[nodiscard]]
    constexpr auto data() noexcept -> pointer;

    [[nodiscard]]
    constexpr auto data() const noexcept -> const_pointer;

    template <ts_unsigned_integral SpanSizeT = usize>
    [[nodiscard]]
    auto span() -> Span<pointer, SpanSizeT>;

    template <ts_unsigned_integral SpanSizeT = usize>
    [[nodiscard]]
    auto span() const -> Span<const_pointer, SpanSizeT>;

    template <ts_unsigned_integral SpanSizeT = usize>
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

    T list[ArrSizeV];
  };

  template <typeset T, usize ArrSizeV>
  [[nodiscard]]
  constexpr auto Array<T, ArrSizeV>::Fill(const T& val) -> this_type
    requires(ArrSizeV <= MAX_BRACE_INIT_SIZE)
  {
    const auto create_array =
      []<usize... idx>(std::index_sequence<idx...>, const T& val) -> this_type
    {
      return {(static_cast<void>(idx), val)...};
    };
    return create_array(std::make_index_sequence<ArrSizeV>(), val);
  }

  template <typeset T, usize ArrSizeV>
  template <ts_generate_fn<T> Fn>
  [[nodiscard]]
  constexpr auto Array<T, ArrSizeV>::Generate(Fn fn) -> this_type
    requires(ArrSizeV <= MAX_BRACE_INIT_SIZE)
  {
    const auto create_array = []<usize... idx>(std::index_sequence<idx...>, Fn fn) -> this_type
    {
      return {(static_cast<void>(idx), std::invoke(fn))...};
    };
    return create_array(std::make_index_sequence<ArrSizeV>(), fn);
  }

  template <typeset T, usize ArrSizeV>
  template <ts_fn<T, usize> Fn>
  [[nodiscard]]
  constexpr auto Array<T, ArrSizeV>::TransformIndex(Fn fn) -> this_type
    requires(ArrSizeV <= MAX_BRACE_INIT_SIZE)
  {
    static_assert(ArrSizeV <= MAX_BRACE_INIT_SIZE);
    const auto create_array = []<usize... idx>(std::index_sequence<idx...>, Fn fn) -> this_type
    {
      return {(std::invoke(fn, idx))...};
    };
    return create_array(std::make_index_sequence<ArrSizeV>(), fn);
  }

  template <typeset T, usize ArrSizeV>
  constexpr void Array<T, ArrSizeV>::swap(this_type& other) noexcept
  {
    using std::swap;
    for (usize i = 0; i < ArrSizeV; i++)
    {
      swap(list[i], other.list[i]);
    }
  }

  template <typeset T, usize ArrSizeV>
  [[nodiscard]]
  constexpr auto Array<T, ArrSizeV>::clone() const -> this_type
    requires(ts_clone<T> && ArrSizeV <= MAX_BRACE_INIT_SIZE)
  {
    const auto create_array =
      []<usize... idx>(std::index_sequence<idx...>, const this_type& original) -> this_type
    {
      return {original.list[idx].clone()...};
    };
    return create_array(std::make_index_sequence<ArrSizeV>(), *this);
  }

  template <typeset T, usize ArrSizeV>
  constexpr void Array<T, ArrSizeV>::clone_from(const this_type& other)
    requires ts_clone<T>
  {
    for (usize i = 0; i < ArrSizeV; i++)
    {
      list[i].clone_from(other.buffer[i]);
    }
  }

  template <typeset T, usize ArrSizeV>
  constexpr auto Array<T, ArrSizeV>::data() noexcept -> pointer
  {
    return list;
  }

  template <typeset T, usize ArrSizeV>
  constexpr auto Array<T, ArrSizeV>::data() const noexcept -> const_pointer
  {
    return list;
  }

  template <typeset T, usize ArrSizeV>
  template <ts_unsigned_integral SpanSizeT>
  auto Array<T, ArrSizeV>::span() -> Span<pointer, SpanSizeT>
  {
    return {data(), cast<SpanSizeT>(size())};
  }

  template <typeset T, usize ArrSizeV>
  template <ts_unsigned_integral SpanSizeT>
  auto Array<T, ArrSizeV>::span() const -> Span<const_pointer, SpanSizeT>
  {
    return {data(), cast<SpanSizeT>(size())};
  }

  template <typeset T, usize ArrSizeV>
  template <ts_unsigned_integral SpanSizeT>
  auto Array<T, ArrSizeV>::cspan() const -> Span<const_pointer, SpanSizeT>
  {
    return {data(), cast<SpanSizeT>(size())};
  }

  template <typeset T, usize ArrSizeV>
  constexpr auto Array<T, ArrSizeV>::front() -> reference
  {
    return list[0];
  }

  template <typeset T, usize ArrSizeV>
  constexpr auto Array<T, ArrSizeV>::front() const -> const_reference
  {
    return list[0];
  }

  template <typeset T, usize ArrSizeV>
  constexpr auto Array<T, ArrSizeV>::back() noexcept -> reference
  {
    return list[ArrSizeV - 1];
  }

  template <typeset T, usize ArrSizeV>
  constexpr auto Array<T, ArrSizeV>::back() const noexcept -> const_reference
  {
    return list[ArrSizeV - 1];
  }

  template <typeset T, usize ArrSizeV>
  constexpr auto Array<T, ArrSizeV>::operator[](usize idx) -> reference
  {
    SOUL_ASSERT_UPPER_BOUND_CHECK(idx, ArrSizeV);
    return list[idx];
  }

  template <typeset T, usize ArrSizeV>
  constexpr auto Array<T, ArrSizeV>::operator[](usize idx) const -> const_reference
  {
    SOUL_ASSERT_UPPER_BOUND_CHECK(idx, ArrSizeV);
    return list[idx];
  }

  template <typeset T, usize ArrSizeV>
  [[nodiscard]]
  constexpr auto Array<T, ArrSizeV>::size() const -> usize
  {
    return ArrSizeV;
  }

  template <typeset T, usize ArrSizeV>
  [[nodiscard]]
  constexpr auto Array<T, ArrSizeV>::empty() const -> b8
  {
    return ArrSizeV == 0;
  }

  template <typeset T, usize ArrSizeV>
  constexpr auto Array<T, ArrSizeV>::begin() -> iterator
  {
    return list;
  }

  template <typeset T, usize ArrSizeV>
  constexpr auto Array<T, ArrSizeV>::begin() const -> const_iterator
  {
    return list;
  }

  template <typeset T, usize ArrSizeV>
  constexpr auto Array<T, ArrSizeV>::cbegin() const -> const_iterator
  {
    return list;
  }

  template <typeset T, usize ArrSizeV>
  constexpr auto Array<T, ArrSizeV>::end() -> iterator
  {
    return list + ArrSizeV;
  }

  template <typeset T, usize ArrSizeV>
  constexpr auto Array<T, ArrSizeV>::end() const -> const_iterator
  {
    return list + ArrSizeV;
  }

  template <typeset T, usize ArrSizeV>
  constexpr auto Array<T, ArrSizeV>::cend() const -> const_iterator
  {
    return list + ArrSizeV;
  }

  template <typeset T, usize ArrSizeV>
  constexpr auto Array<T, ArrSizeV>::rbegin() -> reverse_iterator
  {
    return reverse_iterator(end());
  }

  template <typeset T, usize ArrSizeV>
  constexpr auto Array<T, ArrSizeV>::rbegin() const -> const_reverse_iterator
  {
    return const_reverse_iterator(cend());
  }

  template <typeset T, usize ArrSizeV>
  constexpr auto Array<T, ArrSizeV>::crbegin() const -> const_reverse_iterator
  {
    return const_reverse_iterator(cend());
  }

  template <typeset T, usize ArrSizeV>
  constexpr auto Array<T, ArrSizeV>::rend() -> reverse_iterator
  {
    return reverse_iterator(begin());
  }

  template <typeset T, usize ArrSizeV>
  constexpr auto Array<T, ArrSizeV>::rend() const -> const_reverse_iterator
  {
    return const_reverse_iterator(cbegin());
  }

  template <typeset T, usize ArrSizeV>
  constexpr auto Array<T, ArrSizeV>::crend() const -> const_reverse_iterator
  {
    return const_reverse_iterator(cbegin());
  }

  template <typeset T>
  struct Array<T, 0>
  {

    using this_type              = Array<T, 0>;
    using value_type             = T;
    using pointer                = T*;
    using const_pointer          = const T*;
    using reference              = T&;
    using const_reference        = const T&;
    using iterator               = T*;
    using const_iterator         = const T*;
    using reverse_iterator       = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    [[nodiscard]]
    static constexpr auto Fill(const T& /* val */) -> this_type
    {
      return this_type();
    }

    template <ts_generate_fn<T> Fn>
    [[nodiscard]]
    static constexpr auto Generate(Fn /* fn */) -> this_type
    {
      return this_type();
    }

    template <ts_fn<T, usize> Fn>
    [[nodiscard]]
    static constexpr auto TransformIndex(Fn /* fn */) -> this_type
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

  template <typeset T, typeset... U>
  Array(T, U...) -> Array<T, 1 + sizeof...(U)>;

  template <typeset T, usize ArrSizeV>
  constexpr auto operator==(const Array<T, ArrSizeV>& lhs, const Array<T, ArrSizeV>& rhs) -> b8
  {
    return std::ranges::equal(lhs, rhs);
  }

} // namespace soul
