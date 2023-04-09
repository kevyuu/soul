#pragma once

#include "memory/allocator.h"

namespace soul
{
  namespace impl
  {
    auto get_default_allocator() -> memory::Allocator*;
  }
#ifndef USE_CUSTOM_DEFAULT_ALLOCATOR
  inline auto get_default_allocator() -> memory::Allocator*
  {
    return impl::get_default_allocator();
  }
#else
  memory::Allocator* get_default_allocator();
#endif

#ifndef SOUL_BIT_BLOCK_TYPE_DEFAULT
#  define SOUL_BIT_BLOCK_TYPE_DEFAULT soul_size
#endif

} // namespace soul
