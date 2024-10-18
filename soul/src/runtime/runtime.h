#pragma once

#include "core/type_traits.h"
#include "runtime/data.h"
#include "runtime/system.h"

namespace soul::runtime
{

  inline void init(const Config& config)
  {
    System::get().init(config);
  }

  inline void shutdown()
  {
    System::get().shutdown();
  }

  inline void begin_frame()
  {
    System::get().begin_frame();
  }

  inline auto create_task(const TaskID parent = TaskID::ROOT()) -> TaskID
  {
    return System::get().create_task(parent, [](TaskID) {});
  }

  template <execution Execute>
  auto create_task(const TaskID parent, Execute&& lambda) -> TaskID
  {
    return System::get().create_task(parent, std::forward<Execute>(lambda));
  }

  inline void wait_task(TaskID taskID)
  {
    System::get().wait_task(taskID);
  }

  inline void run_task(TaskID taskID)
  {
    System::get().task_run(taskID);
  }

  inline void run_and_wait_task(TaskID task_id)
  {
    run_task(task_id);
    wait_task(task_id);
  }

  template <execution Execute>
  auto create_and_run_task(TaskID parent, Execute&& lambda) -> TaskID
  {
    const TaskID taskID = create_task(parent, std::forward<Execute>(lambda));
    run_task(taskID);
    return taskID;
  }

  template <ts_fn<void, int> Fn>
  auto parallel_for_task_create(TaskID parent, u32 count, u32 blockSize, Fn&& func) -> TaskID
  {
    return System::get().create_parallel_for_task_recursive(
      parent, 0, count, blockSize, std::forward<Fn>(func));
  }

  inline auto get_thread_id() -> u16
  {
    return System::get().get_thread_id();
  }

  inline auto get_thread_count() -> u16
  {
    return System::get().get_thread_count();
  }

  inline void push_allocator(memory::Allocator* allocator)
  {
    System::get().push_allocator(allocator);
  }

  inline void pop_allocator()
  {
    System::get().pop_allocator();
  }

  inline auto get_context_allocator() -> memory::Allocator*
  {
    return System::get().get_context_allocator();
  }

  inline auto get_temp_allocator() -> TempAllocator*
  {
    return System::get().get_temp_allocator();
  }

  inline auto allocate(u32 size, u32 alignment) -> void*
  {
    return System::get().allocate(size, alignment);
  }

  inline auto is_worker_thread() -> b8
  {
    return System::get().is_worker_thread();
  }

  inline void deallocate(void* addr, u32 size)
  {
    return System::get().deallocate(addr, size);
  }

  struct AllocatorInitializer
  {
    AllocatorInitializer() = delete;

    explicit AllocatorInitializer(memory::Allocator* allocator)
    {
      push_allocator(allocator);
    }

    void end()
    {
      pop_allocator();
    }
  };

  struct AllocatorZone
  {
    AllocatorZone() = delete;

    explicit AllocatorZone(memory::Allocator* allocator)
    {
      push_allocator(allocator);
    }

    AllocatorZone(const AllocatorZone& other) = delete;

    AllocatorZone(AllocatorZone&& other) = delete;

    auto operator=(const AllocatorZone&) = delete;

    auto operator=(AllocatorZone&&) = delete;

    ~AllocatorZone()
    {
      pop_allocator();
    }
  };

#define STRING_JOIN2(arg1, arg2)    DO_STRING_JOIN2(arg1, arg2)
#define DO_STRING_JOIN2(arg1, arg2) arg1##arg2
#define SOUL_MEMORY_ALLOCATOR_ZONE(allocator)                                                      \
  soul::runtime::AllocatorZone STRING_JOIN2(allocatorZone, __LINE__)(allocator)
} // namespace soul::runtime
