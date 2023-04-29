#pragma once

#include <format>

#include "core/cstring.h"

namespace soul
{
  template <typename... Args>
  void appendf(CString& string, std::format_string<Args...> fmt, Args&&... args)
  {
    std::format_to(std::back_inserter(string), std::move(fmt), std::forward<Args>(args)...);
  }
} // namespace soul
