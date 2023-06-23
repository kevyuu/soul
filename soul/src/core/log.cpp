#include <iostream>

#include "core/cstring.h"
#include "core/log.h"
#include "core/mutex.h"
#include "memory/allocator.h"
#include "memory/allocators/malloc_allocator.h"

namespace soul::impl
{
  namespace
  {
    struct LogBuffer {
      static constexpr soul_size CAPACITY = 8192;
      Mutex lock;
      CString buffer;

      explicit LogBuffer(memory::Allocator* allocator) : buffer(allocator) { buffer.reserve(8192); }
    };

    using LogBuffers = FlagMap<LogLevel, LogBuffer>;

    auto get_log_buffers() -> LogBuffers&
    {
      static memory::MallocAllocator malloc_allocator("malloc allocator");
      static auto log_buffers =
        LogBuffers::init_generate([] { return LogBuffer(&malloc_allocator); });
      return log_buffers;
    }

    auto get_output_stream(LogLevel log_level) -> std::ostream&
    {
      if (log_level == LogLevel::FATAL && log_level == LogLevel::ERROR) {
        return std::cerr;
      } else {
        return std::cout;
      }
    }

    auto log_flush_no_lock(LogLevel log_level) -> void
    {
      auto& log_buffers = get_log_buffers();
      get_output_stream(log_level) << log_buffers[log_level].buffer.data();
    }
  } // namespace

  auto log(LogLevel log_level, const CString& message) -> void
  {
    auto& log_buffers = get_log_buffers();
    std::lock_guard guard(log_buffers[log_level].lock);
    auto& log_buffer = log_buffers[log_level];
    if (log_buffer.buffer.size() + message.size() + 1 > LogBuffer::CAPACITY) {
      log_flush_no_lock(log_level);
    }
    if (message.size() >= log_buffer.buffer.capacity()) {
      get_output_stream(log_level) << message.data() << std::endl;
      return;
    }
    log_buffer.buffer.append(message);
    log_buffer.buffer.push_back('\n');
  }

  auto flush_log(LogLevel log_level) -> void
  {
    auto& log_buffers = impl::get_log_buffers();
    std::lock_guard guard(log_buffers[log_level].lock);
    impl::log_flush_no_lock(log_level);
  }

  auto flush_logs() -> void
  {
    for (auto log_level : FlagIter<LogLevel>()) {
      flush_log(log_level);
    }
  }
} // namespace soul::impl
