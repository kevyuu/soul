#pragma once

#include "core/builtins.h"
#include "core/span.h"

namespace soul
{
  class CompStr;

  inline namespace literals
  {
    consteval auto operator""_str(const char* literal, usize length) -> CompStr;
  } // namespace literals

  class CompStr
  {
  private:
    const char* data_;
    usize size_;

    consteval CompStr(const char* literal, usize length) : data_(literal), size_(length) {}

  public:
    [[nodiscard]]
    constexpr auto size() const -> usize
    {
      return size_;
    }

    [[nodiscard]]
    constexpr auto data() const -> const char*
    {
      return data_;
    }

    [[nodiscard]]
    constexpr auto c_str() const -> const char*
    {
      return data_;
    }

    constexpr operator Span<const char*>() const // NOLINT(hicpp-explicit-conversions)
    {
      return {data_, size_};
    }

    friend consteval auto literals::operator""_str(const char* literal, usize length) -> CompStr;
  };

  inline namespace literals
  {
    [[nodiscard]]
    consteval auto operator""_str(const char* literal, usize length) -> CompStr
    {
      return CompStr(literal, length);
    }

  } // namespace literals

} // namespace soul
