#pragma once

#include "core/type.h"

#include <thread>

inline constexpr uint64 SOUL_CACHELINE_SIZE = 64;
inline constexpr uint64 SOUL_DEFAULT_HARDWARE_THREAD_COUNT = 8;

// testtest
inline soul_size get_hardware_thread_count()
{
    const uint64 count = std::thread::hardware_concurrency();
    return count != 0 ? count : SOUL_DEFAULT_HARDWARE_THREAD_COUNT;
}
