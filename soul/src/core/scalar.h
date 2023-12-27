#pragma once

#include "boolean.h"
#include "floating_point.h"
#include "integer.h"

namespace soul
{
  // ----------------------------------------------------------------------------
  // Boolean reductions
  // ----------------------------------------------------------------------------

  template <typename T>
  [[nodiscard]]
  constexpr auto any(T x) -> bool
  {
    return x != T(0);
  }

  template <typename T>
  [[nodiscard]]
  constexpr auto all(T x) -> bool
  {
    return x != T(0);
  }
} // namespace soul
