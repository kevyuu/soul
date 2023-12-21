#pragma once

#include "core/string_view.h"
#include "memory/allocator.h"

#include <cstring>

namespace soul
{

  template <memory::allocator_type AllocatorT>
  auto get_or_create_cstr(StringView str_view, NotNull<AllocatorT*> allocator) -> const char*
  {
    if (str_view.is_null_terminated()) {
      return str_view.data();
    }
    char* array = allocator->template allocate_array<char>(str_view.size() + 1);
    std::memcpy(array, str_view.data(), str_view.size());
    array[str_view.size()] = '\0';
    return array;
  }
} // namespace soul
