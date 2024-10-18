#pragma once

#include "core/string.h"
#include "memory/allocators/malloc_allocator.h"

using namespace soul;

namespace khaos
{
  class StreamingBuffer
  {
  public:
    StreamingBuffer();

    void push(StringView message);
    void consume(NotNull<String*> dst);
    auto size() const -> usize;

  private:
    mutable std::mutex mutex_;
    memory::MallocAllocator allocator_;
    String buffer_;
  };
} // namespace khaos
