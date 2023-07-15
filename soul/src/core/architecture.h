#pragma once

#include <thread>

#include "core/type.h"

namespace soul
{
  inline constexpr u64 SOUL_CACHELINE_SIZE = 64;
  inline constexpr u64 SOUL_DEFAULT_HARDWARE_THREAD_COUNT = 8;

  inline auto get_hardware_thread_count() -> usize
  {
    const u64 count = std::thread::hardware_concurrency();
    return count != 0 ? count : SOUL_DEFAULT_HARDWARE_THREAD_COUNT;
  }
} // namespace soul
