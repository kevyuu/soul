#include "streaming_buffer.h"
#include <mutex>
#include "core/string_view.h"
#include "memory/allocators/malloc_allocator.h"

namespace khaos
{
  StreamingBuffer::StreamingBuffer() : allocator_("Streaming buffer"_str), buffer_(&allocator_) {}

  void StreamingBuffer::push(StringView message)
  {
    std::lock_guard guard(mutex_);
    buffer_.append(message);
  }

  void StreamingBuffer::consume(NotNull<String*> dst)
  {
    std::lock_guard guard(mutex_);
    dst->append(buffer_);
    buffer_.clear();
  }

  void StreamingBuffer::clear()
  {
    return buffer_.clear();
  }

  auto StreamingBuffer::cview() const -> StringView
  {
    return buffer_.cview(); 
  }

  auto StreamingBuffer::size() const -> usize
  {
    std::lock_guard guard(mutex_);
    return buffer_.size();
  }
} // namespace khaos
