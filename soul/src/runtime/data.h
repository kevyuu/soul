#pragma once

#include <atomic>
#include <mutex>
#include <thread>
#include <condition_variable>

#include "core/architecture.h"
#include "core/fixed_vector.h"
#include "core/type.h"
#include "core/vector.h"
#include "memory/allocator.h"
#include "memory/allocators/linear_allocator.h"
#include "memory/allocators/malloc_allocator.h"
#include "memory/allocators/proxy_allocator.h"

namespace soul::runtime
{
  using ThreadCount = u16;

  using TempProxy = memory::NoOpProxy;
  using TempAllocator = memory::ProxyAllocator<memory::LinearAllocator, TempProxy>;

  using DefaultAllocatorProxy = memory::MultiProxy<
    memory::MutexProxy,
    memory::ProfileProxy,
    memory::CounterProxy,
    memory::ClearValuesProxy,
    memory::BoundGuardProxy>;
  using DefaultAllocator = memory::ProxyAllocator<memory::MallocAllocator, DefaultAllocatorProxy>;

  struct Config {
    u16 threadCount; // 0 to use hardware thread count
    u16 taskPoolCount;
    TempAllocator* mainThreadTempAllocator;
    u64 workerTempAllocatorSize;
    DefaultAllocator* defaultAllocator;
  };

  struct Constant {
    static constexpr u32 TASK_ID_THREAD_INDEX_MASK = 0xFFFFC000;
    static constexpr u32 TASK_ID_THREAD_INDEX_SHIFT = 14;
    static constexpr u32 TASK_ID_TASK_INDEX_MASK = 0x00003FFF;
    static constexpr u32 TASK_ID_TASK_INDEX_SHIFT = 0;

    static constexpr u16 MAX_THREAD_COUNT = 16;
    static constexpr usize MAX_TASK_PER_THREAD = 2u << (TASK_ID_THREAD_INDEX_SHIFT - 1);
  };

  struct Task;

  // NOTE(kevinyu): We use id == 0 as both root and null value;
  struct TaskID {
    u32 id;
    static constexpr auto NULLVAL() -> TaskID { return {0, 0}; }
    static constexpr auto ROOT() -> TaskID { return NULLVAL(); }

    constexpr TaskID() : id(NULLVAL().id) {}

    constexpr TaskID(u32 thread_index, u32 task_index)
        : id(
            (thread_index << Constant::TASK_ID_THREAD_INDEX_SHIFT) |
            (task_index << Constant::TASK_ID_TASK_INDEX_SHIFT))
    {
      SOUL_ASSERT(0, task_index < Constant::MAX_TASK_PER_THREAD, "Task Index overflow");
    }

    [[nodiscard]]
    auto get_thread_index() const -> u32
    {
      return (id & Constant::TASK_ID_THREAD_INDEX_MASK) >> Constant::TASK_ID_THREAD_INDEX_SHIFT;
    }

    [[nodiscard]]
    auto get_task_index() const -> u32
    {
      return (id & Constant::TASK_ID_TASK_INDEX_MASK) >> Constant::TASK_ID_TASK_INDEX_SHIFT;
    }

    [[nodiscard]]
    auto
    operator==(const TaskID& other) const -> b8
    {
      return other.id == id;
    }
    [[nodiscard]]
    auto
    operator!=(const TaskID& other) const -> b8
    {
      return other.id != id;
    }
    [[nodiscard]]
    auto is_root() const -> b8
    {
      return id == ROOT().id;
    }
    [[nodiscard]]
    auto is_null() const -> b8
    {
      return id == NULLVAL().id;
    }
  };

  using TaskFunc = void (*)(TaskID taskID, void* data);

  struct alignas(SOUL_CACHELINE_SIZE) Task {
    static constexpr u32 STORAGE_SIZE_BYTE = SOUL_CACHELINE_SIZE - sizeof(TaskFunc) // func size
                                             - sizeof(TaskID)                       // parentID size
                                             - sizeof(std::atomic<u16>); // unfinishedCount size

    void* storage[STORAGE_SIZE_BYTE / sizeof(void*)] = {};
    TaskFunc func = nullptr;
    TaskID parent_id;
    std::atomic<u16> unfinished_count = {0};
  };

  static_assert(
    sizeof(Task) == SOUL_CACHELINE_SIZE, "Task must be the same size as cache line size.");

  class TaskDeque
  {
  public:
    TaskID _tasks[Constant::MAX_TASK_PER_THREAD];
    std::atomic<i32> _bottom;
    std::atomic<i32> _top;

    void init();

    void shutdown();

    void reset();

    void push(TaskID task);

    auto pop() -> TaskID; // return nullptr if empty

    // return nullptr if empty or fail (other thread do steal operation concurrently)
    auto steal() -> TaskID;
  };

  struct alignas(SOUL_CACHELINE_SIZE) ThreadContext {
    TaskDeque task_deque;

    Task task_pool[Constant::MAX_TASK_PER_THREAD];
    u16 task_count = 0;

    u16 thread_index = 0;

    Vector<memory::Allocator*> allocator_stack{nullptr};
    TempAllocator* temp_allocator = nullptr;
  };

  struct Database {
    thread_local static ThreadContext* g_thread_context; // NOLINT
    FixedVector<ThreadContext> thread_contexts;
    std::thread threads[Constant::MAX_THREAD_COUNT];

    std::condition_variable wait_cond_var;
    std::mutex wait_mutex;

    std::condition_variable loop_cond_var;
    std::mutex loop_mutex;

    std::atomic<b8> is_terminated;

    usize active_task_count = 0;
    ThreadCount thread_count = 0;

    memory::Allocator* default_allocator = nullptr;
    usize temp_allocator_size = 0;

    Database() : thread_contexts(nullptr) {}
  };

  template <typename Func>
  struct ParallelForTaskData {
    using ParallelForFunc = Func;

    explicit ParallelForTaskData(u32 start, u32 count, u32 min_count, ParallelForFunc&& func)
        : start(start),
          count(count),
          min_count(min_count),
          func(std::forward<ParallelForFunc>(func))
    {
    }

    u32 start;
    u32 count;
    u32 min_count;
    ParallelForFunc func;
  };

  template <typename T>
  concept execution = ts_fn<T, void, TaskID>;

} // namespace soul::runtime
