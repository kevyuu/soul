#pragma once

#include <cstdint>

#include "core/boolean.h"
#include "core/floating_point.h"
#include "core/integer.h"
#include "core/matrix.h"
#include "core/quaternion.h"
#include "core/vec.h"

namespace soul
{
  inline namespace builtin
  {
    using iptr = std::intptr_t;
    using uptr = std::uintptr_t;

    using byte = u8;
  } // namespace builtin

} // namespace soul
