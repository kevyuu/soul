#pragma once

#include <format>

#include "core/span.h"

namespace soul
{
  using StringView = Span<const char*>;

} // namespace soul

template <>
struct std::formatter<soul::StringView> : std::formatter<std::string_view>
{
  auto format(soul::StringView string_view, std::format_context& ctx) const
  {
    return std::formatter<std::string_view>::format(
      std::string_view(string_view.data(), string_view.size()), ctx);
  }
};
