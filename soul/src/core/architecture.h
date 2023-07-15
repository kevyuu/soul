#pragma once

#include <thread>

#include "core/type.h"

inline constexpr ui64 SOUL_CACHELINE_SIZE = 64;
inline constexpr ui64 SOUL_DEFAULT_HARDWARE_THREAD_COUNT = 8;

inline auto get_hardware_thread_count() -> usize
{
  const ui64 count = std::thread::hardware_concurrency();
  return count != 0 ? count : SOUL_DEFAULT_HARDWARE_THREAD_COUNT;
}
