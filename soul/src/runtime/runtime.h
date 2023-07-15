#pragma once

#include "core/type_traits.h"
#include "runtime/data.h"
#include "runtime/system.h"

namespace soul::runtime
{

  inline auto init(const Config& config) -> void { System::get().init(config); }

  inline auto shutdown() -> void { System::get().shutdown(); }

  inline auto begin_frame() -> void { System::get().begin_frame(); }

  inline auto create_task(const TaskID parent = TaskID::ROOT()) -> TaskID
  {
    return System::get().create_task(parent, [](TaskID) {});
  }

  template <execution Execute>
  auto create_task(const TaskID parent, Execute&& lambda) -> TaskID
  {
    return System::get().create_task(parent, std::forward<Execute>(lambda));
  }

  inline auto wait_task(TaskID taskID) -> void { System::get().wait_task(taskID); }

  inline auto run_task(TaskID taskID) -> void { System::get().task_run(taskID); }

  inline auto run_and_wait_task(TaskID task_id) -> void
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

  template <ts_fn<void,int> Fn>
  auto parallel_for_task_create(TaskID parent, u32 count, u32 blockSize, Fn&& func)
    -> TaskID
  {
    return System::get().create_parallel_for_task_recursive(
      parent, 0, count, blockSize, std::forward<Fn>(func));
  }

  inline auto get_thread_id() -> u16 { return System::get().get_thread_id(); }

  inline auto get_thread_count() -> u16 { return System::get().get_thread_count(); }

  inline auto push_allocator(memory::Allocator* allocator) -> void
  {
    System::get().push_allocator(allocator);
  }

  inline auto pop_allocator() -> void { System::get().pop_allocator(); }

  inline auto get_context_allocator() -> memory::Allocator*
  {
    return System::get().get_context_allocator();
  }

  inline auto get_temp_allocator() -> TempAllocator* { return System::get().get_temp_allocator(); }

  inline auto allocate(u32 size, u32 alignment) -> void*
  {
    return System::get().allocate(size, alignment);
  }

  inline auto deallocate(void* addr, u32 size) -> void
  {
    return System::get().deallocate(addr, size);
  }

  struct AllocatorInitializer {
    AllocatorInitializer() = delete;

    explicit AllocatorInitializer(memory::Allocator* allocator) { push_allocator(allocator); }

    auto end() -> void { pop_allocator(); }
  };

  struct AllocatorZone {
    AllocatorZone() = delete;

    explicit AllocatorZone(memory::Allocator* allocator) { push_allocator(allocator); }

    ~AllocatorZone() { pop_allocator(); }
  };

#define STRING_JOIN2(arg1, arg2) DO_STRING_JOIN2(arg1, arg2)
#define DO_STRING_JOIN2(arg1, arg2) arg1##arg2
#define SOUL_MEMORY_ALLOCATOR_ZONE(allocator)                                                      \
  soul::runtime::AllocatorZone STRING_JOIN2(allocatorZone, __LINE__)(allocator)
} // namespace soul::runtime
