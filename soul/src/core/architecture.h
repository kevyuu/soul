#pragma once

#include <thread>

#include "core/type.h"

inline constexpr uint64 SOUL_CACHELINE_SIZE = 64;
inline constexpr uint64 SOUL_DEFAULT_HARDWARE_THREAD_COUNT = 8;

inline auto get_hardware_thread_count() -> soul_size
{
  const uint64 count = std::thread::hardware_concurrency();
  return count != 0 ? count : SOUL_DEFAULT_HARDWARE_THREAD_COUNT;
}
