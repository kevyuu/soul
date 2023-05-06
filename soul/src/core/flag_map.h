#pragma once

#include <array>
#include <utility>
#include "core/type.h"
#include <type_traits>

namespace soul
{

  template <flag EnumType, typename ValType>
  class FlagMap
  {
  public:
    static constexpr uint64 COUNT = to_underlying(EnumType::COUNT);

    using this_type = FlagMap<EnumType, ValType>;
    using value_type = ValType;
    using pointer = ValType*;
    using const_pointer = const ValType*;
    using reference = ValType&;
    using const_reference = const ValType&;
    using iterator = ValType*;
    using const_iterator = const ValType*;
    using buffer_type = std::array<ValType, COUNT>;

    template <typename... Args>
    static constexpr auto with_value_arguments(Args&&... args) noexcept -> this_type;

    static constexpr auto with_default_value(const ValType& val) noexcept -> this_type;

    template <std::size_t N>
    static consteval auto from_val_list(const ValType (&list)[N]) noexcept -> this_type;

    static constexpr auto from_key_val_list(
      std::initializer_list<std::pair<EnumType, ValType>> init_list) noexcept -> this_type;

    constexpr auto operator[](EnumType index) noexcept -> reference
    {
      SOUL_ASSERT(0, to_underlying(index) < to_underlying(EnumType::COUNT), "");
      return buffer_[to_underlying(index)];
    }

    constexpr auto operator[](EnumType index) const -> const_reference
    {
      SOUL_ASSERT(0, to_underlying(index) < to_underlying(EnumType::COUNT), "");
      return buffer_[to_underlying(index)];
    }

    [[nodiscard]] constexpr auto size() const -> soul_size
    {
      return to_underlying(EnumType::COUNT);
    }

    [[nodiscard]] constexpr auto begin() const -> const_iterator { return buffer_; }
    [[nodiscard]] constexpr auto end() const -> const_iterator { return buffer_ + COUNT; }

    [[nodiscard]] constexpr auto begin() -> iterator { return buffer_; }
    [[nodiscard]] constexpr auto end() -> iterator { return buffer_ + COUNT; }

    ValType buffer_[COUNT];
  };

  template <flag EnumType, typename ValType>
  template <typename... Args>
  constexpr auto FlagMap<EnumType, ValType>::with_value_arguments(Args&&... args) noexcept
    -> this_type
  {
    const auto create_array = [&args...]<soul_size... I>(std::index_sequence<I...>) -> this_type {
      return {(static_cast<void>(I), ValType(args...))...};
    };
    return create_array(std::make_index_sequence<COUNT>());
  }

  template <flag EnumType, typename ValType>
  constexpr auto FlagMap<EnumType, ValType>::with_default_value(const ValType& val) noexcept
    -> this_type
  {
    const auto create_array =
      []<soul_size... I>(std::index_sequence<I...>, const ValType& val) -> this_type {
      return {(static_cast<void>(I), val)...};
    };
    return create_array(std::make_index_sequence<COUNT>(), val);
  }

  template <flag EnumType, typename ValType>
  constexpr auto FlagMap<EnumType, ValType>::from_key_val_list(
    std::initializer_list<std::pair<EnumType, ValType>> init_list) noexcept -> this_type
  {
    FlagMap<EnumType, ValType> ret;
    for (auto item : init_list) {
      ret[item.first] = item.second;
    }
    return ret;
  }

  template <flag EnumType, typename ValType>
  template <std::size_t N>
  consteval auto FlagMap<EnumType, ValType>::from_val_list(const ValType (&list)[N]) noexcept
    -> this_type
  {
    static_assert(N == COUNT);
    const auto create_array = [&]<soul_size... I>(std::index_sequence<I...>) -> this_type {
      return {list[I]...};
    };
    return create_array(std::make_index_sequence<COUNT>());
  }

} // namespace soul
