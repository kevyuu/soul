#pragma once

#include "core/type.h"

#include <thread>

constexpr uint64 SOUL_CACHELINE_SIZE = 64;
constexpr uint64 SOUL_DEFAULT_HARDWARE_THREAD_COUNT = 8;

inline soul_size get_hardware_thread_count()
{
    uint64 count = std::thread::hardware_concurrency();
    return count != 0 ? count : SOUL_DEFAULT_HARDWARE_THREAD_COUNT;
}