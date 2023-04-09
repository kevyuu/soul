#pragma once

#include "core/type.h"

namespace soul::memory::util
{
  [[nodiscard]] static auto pointer_page_size_round(const soul_size size, const soul_size page_size)
    -> soul_size
  {
    SOUL_ASSERT(0, page_size != 0, "");
    return ((size + page_size) & ~(page_size - 1));
  }

  [[nodiscard]] static auto pointer_add(const void* address, const soul_size size) -> void*
  {
    return soul::cast<byte*>(address) + size;
  }

  [[nodiscard]] static auto pointer_align_forward(const void* address, const soul_size alignment)
    -> void*
  {
    return (void*)((reinterpret_cast<uintptr>(address) + alignment) & ~(alignment - 1));
    // NOLINT(performance-no-int-to-ptr)
  }

  [[nodiscard]] static auto pointer_align_backward(const void* address, const soul_size alignment)
    -> void*
  {
    return (void*)(reinterpret_cast<uintptr>(address) &
                   ~(alignment - 1)); // NOLINT(performance-no-int-to-ptr)
  }

  [[nodiscard]] static auto pointer_sub(const void* address, const soul_size size) -> void*
  {
    return soul::cast<byte*>(address) - size;
  }
} // namespace soul::memory::util
