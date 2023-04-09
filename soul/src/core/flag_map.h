#pragma once

#include <array>
#include "core/type.h"

namespace soul
{

  template <flag EnumType, typename ValType>
  class FlagMap
  {
  public:
    static constexpr uint64 COUNT = to_underlying(EnumType::COUNT);

    constexpr FlagMap<EnumType, ValType>() : buffer_() {}

    constexpr explicit FlagMap<EnumType, ValType>(ValType val) noexcept;

    template <std::size_t N>
    static consteval auto build_from_list(const ValType (&list)[N]) -> FlagMap<EnumType, ValType>;

    constexpr FlagMap<EnumType, ValType>(
      std::initializer_list<std::pair<EnumType, ValType>> init_list) noexcept;

    constexpr auto operator[](EnumType index) noexcept -> ValType&
    {
      SOUL_ASSERT(0, to_underlying(index) < to_underlying(EnumType::COUNT), "");
      return buffer_[to_underlying(index)];
    }

    constexpr auto operator[](EnumType index) const -> const ValType&
    {
      SOUL_ASSERT(0, to_underlying(index) < to_underlying(EnumType::COUNT), "");
      return buffer_[to_underlying(index)];
    }

    [[nodiscard]] constexpr auto size() const -> int { return to_underlying(EnumType::COUNT); }

    [[nodiscard]] constexpr auto begin() const -> const ValType* { return buffer_; }
    [[nodiscard]] constexpr auto end() const -> const ValType*
    {
      return buffer_ + to_underlying(EnumType::COUNT);
    }

    [[nodiscard]] constexpr auto begin() -> ValType* { return buffer_; }
    [[nodiscard]] constexpr auto end() -> ValType*
    {
      return buffer_ + to_underlying(EnumType::COUNT);
    }

  private:
    ValType buffer_[to_underlying(EnumType::COUNT)];
  };

  template <flag EnumType, typename ValType>
  constexpr FlagMap<EnumType, ValType>::FlagMap(
    std::initializer_list<std::pair<EnumType, ValType>> init_list) noexcept
      : buffer_()
  {
    for (auto item : init_list) {
      buffer_[to_underlying(item.first)] = item.second;
    }
  }

#pragma warning(disable : 26495)
  template <flag EnumType, typename ValType>
  constexpr FlagMap<EnumType, ValType>::FlagMap(ValType val) noexcept
  {
    for (ValType& dst_val : buffer_) {
      std::construct_at(&dst_val, val);
    }
  }
#pragma warning(default : 26495)

  template <flag EnumType, typename ValType>
  template <std::size_t N>
  consteval auto FlagMap<EnumType, ValType>::build_from_list(const ValType (&list)[N])
    -> FlagMap<EnumType, ValType>
  {
    static_assert(N == COUNT);
    FlagMap<EnumType, ValType> ret;
    for (soul_size i = 0; i < N; i++) {
      ret.buffer_[i] = list[i];
    }
    return ret;
  }

} // namespace soul
