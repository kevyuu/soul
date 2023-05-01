#pragma once

#include "memory/allocator.h"

namespace soul
{
  auto get_default_allocator() -> memory::Allocator*;

#ifndef SOUL_BIT_BLOCK_TYPE_DEFAULT
#  define SOUL_BIT_BLOCK_TYPE_DEFAULT soul_size
#endif

} // namespace soul
