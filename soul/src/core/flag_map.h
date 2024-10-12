#pragma once

#include "core/array.h"
#include "core/type.h"
#include "core/type_traits.h"

#include <iostream>
#include <string_view>

namespace soul
{

  template <ts_flag EnumT, typename ValT>
  class FlagMap
  {
  public:
    static constexpr u64 COUNT = to_underlying(EnumT::COUNT);

    using this_type              = FlagMap<EnumT, ValT>;
    using value_type             = ValT;
    using pointer                = ValT*;
    using const_pointer          = const ValT*;
    using reference              = ValT&;
    using const_reference        = const ValT&;
    using iterator               = ValT*;
    using const_iterator         = const ValT*;
    using reverse_iterator       = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    template <std::size_t N>
    static consteval auto FromValues(const ValT (&list)[N]) noexcept -> this_type;

    static constexpr auto FromKeyValues(
      std::initializer_list<std::pair<EnumT, ValT>> init_list) noexcept -> this_type;

    [[nodiscard]]
    static constexpr auto Fill(const ValT& val) -> this_type
    {
      return {Array<ValT, COUNT>::Fill(val)};
    }

    template <ts_generate_fn<ValT> Fn>
    [[nodiscard]]
    static constexpr auto Generate(Fn fn) -> this_type
    {
      return {Array<ValT, COUNT>::Generate(fn)};
    }

    template <ts_fn<ValT, usize> Fn>
    [[nodiscard]]
    static constexpr auto TransformIndex(Fn fn) -> this_type
    {
      return {Array<ValT, COUNT>::TransformIndex(fn)};
    }

    [[nodiscard]]
    auto clone() const -> this_type
      requires ts_clone<ValT>
    {
      return {buffer_.clone()};
    }

    void clone_from(const this_type& other) const
      requires ts_clone<ValT>
    {
      buffer_.clone_from(other.buffer_);
    }

    constexpr auto operator[](EnumT index) noexcept -> reference
    {
      SOUL_ASSERT_UPPER_BOUND_CHECK(to_underlying(index), to_underlying(EnumT::COUNT));
      return buffer_[to_underlying(index)];
    }

    constexpr auto operator[](EnumT index) const -> const_reference
    {
      SOUL_ASSERT_UPPER_BOUND_CHECK(to_underlying(index), to_underlying(EnumT::COUNT));
      return buffer_[to_underlying(index)];
    }

    constexpr auto ref(EnumT index) noexcept -> reference
    {
      SOUL_ASSERT_UPPER_BOUND_CHECK(to_underlying(index), to_underlying(EnumT::COUNT));
      return buffer_[to_underlying(index)];
    }

    constexpr auto ref(EnumT index) const -> const_reference
    {
      SOUL_ASSERT_UPPER_BOUND_CHECK(to_underlying(index), to_underlying(EnumT::COUNT));
      return buffer_[to_underlying(index)];
    }

    [[nodiscard]]
    constexpr auto size() const -> usize
    {
      return buffer_.size();
    }

    [[nodiscard]]
    constexpr auto empty() const -> b8
    {
      return buffer_.empty();
    }

    [[nodiscard]]
    constexpr auto begin() -> iterator
    {
      return buffer_.begin();
    }

    [[nodiscard]]
    constexpr auto begin() const -> const_iterator
    {
      return buffer_.begin();
    }

    [[nodiscard]]
    constexpr auto cbegin() const -> const_iterator
    {
      return buffer_.cbegin();
    }

    [[nodiscard]]
    constexpr auto end() -> iterator
    {
      return buffer_.end();
    }

    [[nodiscard]]
    constexpr auto end() const -> const_iterator
    {
      return buffer_.end();
    }

    [[nodiscard]]
    constexpr auto cend() const -> const_iterator
    {
      return buffer_.cend();
    }

    [[nodiscard]]
    constexpr auto rbegin() -> reverse_iterator
    {
      return buffer_.rbegin();
    }

    [[nodiscard]]
    constexpr auto rbegin() const -> const_reverse_iterator
    {
      return buffer_.rbegin();
    }

    [[nodiscard]]
    constexpr auto crbegin() const -> const_reverse_iterator
    {
      return buffer_.crbegin();
    }

    [[nodiscard]]
    constexpr auto rend() -> reverse_iterator
    {
      return buffer_.rend();
    }

    [[nodiscard]]
    constexpr auto rend() const -> const_reverse_iterator
    {
      return buffer_.rend();
    }

    [[nodiscard]]
    constexpr auto crend() const -> const_reverse_iterator
    {
      return buffer_.crend();
    }

    template <typename CompareValT>
    constexpr auto find_first_key_with_val(const CompareValT& val) const -> EnumT
    {
      for (u32 i = 0; i < COUNT; i++)
      {
        std::cout << "find first key" << std::endl;
        std::cout << buffer_[i].data() << std::endl;
        std::cout << std::string_view(val.data(), val.size()) << std::endl;
        if (buffer_[i] == val)
        {
          return EnumT(i);
        }
      }
      return EnumT::COUNT;
    }

    soul::Array<ValT, COUNT> buffer_;
  };

  template <ts_flag EnumT, typename ValT>
  constexpr auto FlagMap<EnumT, ValT>::FromKeyValues(
    std::initializer_list<std::pair<EnumT, ValT>> init_list) noexcept -> this_type
  {
    FlagMap<EnumT, ValT> ret;
    for (auto item : init_list)
    {
      ret[item.first] = item.second;
    }
    return ret;
  }

  template <ts_flag EnumT, typename ValT>
  template <std::size_t N>
  consteval auto FlagMap<EnumT, ValT>::FromValues(const ValT (&list)[N]) noexcept -> this_type
  {
    static_assert(N == COUNT);
    const auto create_array = [&]<usize... I>(std::index_sequence<I...>) -> this_type
    {
      return {list[I]...};
    };
    return create_array(std::make_index_sequence<COUNT>());
  }

} // namespace soul
