#pragma once

namespace soul::memory
{
  class Allocator;
}

namespace soul
{
  auto get_default_allocator() -> memory::Allocator*;

#ifndef SOUL_BIT_BLOCK_TYPE_DEFAULT
#  define SOUL_BIT_BLOCK_TYPE_DEFAULT usize
#endif

} // namespace soul
