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
  using ThreadCount = uint16;

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
    uint16 threadCount; // 0 to use hardware thread count
    uint16 taskPoolCount;
    TempAllocator* mainThreadTempAllocator;
    uint64 workerTempAllocatorSize;
    DefaultAllocator* defaultAllocator;
  };

  struct Constant {
    static constexpr uint32 TASK_ID_THREAD_INDEX_MASK = 0xFFFFC000;
    static constexpr uint32 TASK_ID_THREAD_INDEX_SHIFT = 14;
    static constexpr uint32 TASK_ID_TASK_INDEX_MASK = 0x00003FFF;
    static constexpr uint32 TASK_ID_TASK_INDEX_SHIFT = 0;

    static constexpr uint16 MAX_THREAD_COUNT = 16;
    static constexpr soul_size MAX_TASK_PER_THREAD = 2u << (TASK_ID_THREAD_INDEX_SHIFT - 1);
  };

  struct Task;

  // NOTE(kevinyu): We use id == 0 as both root and null value;
  struct TaskID {
    uint32 id;
    static constexpr auto NULLVAL() -> TaskID { return {0, 0}; }
    static constexpr auto ROOT() -> TaskID { return NULLVAL(); }

    constexpr TaskID() : id(NULLVAL().id) {}

    constexpr TaskID(uint32 thread_index, uint32 task_index)
        : id(
            (thread_index << Constant::TASK_ID_THREAD_INDEX_SHIFT) |
            (task_index << Constant::TASK_ID_TASK_INDEX_SHIFT))
    {
      SOUL_ASSERT(0, task_index < Constant::MAX_TASK_PER_THREAD, "Task Index overflow");
    }

    [[nodiscard]] auto get_thread_index() const -> uint32
    {
      return (id & Constant::TASK_ID_THREAD_INDEX_MASK) >> Constant::TASK_ID_THREAD_INDEX_SHIFT;
    }

    [[nodiscard]] auto get_task_index() const -> uint32
    {
      return (id & Constant::TASK_ID_TASK_INDEX_MASK) >> Constant::TASK_ID_TASK_INDEX_SHIFT;
    }

    [[nodiscard]] auto operator==(const TaskID& other) const -> bool { return other.id == id; }
    [[nodiscard]] auto operator!=(const TaskID& other) const -> bool { return other.id != id; }
    [[nodiscard]] auto is_root() const -> bool { return id == ROOT().id; }
    [[nodiscard]] auto is_null() const -> bool { return id == NULLVAL().id; }
  };

  using TaskFunc = void (*)(TaskID taskID, void* data);

  struct alignas(SOUL_CACHELINE_SIZE) Task {
    static constexpr uint32 STORAGE_SIZE_BYTE = SOUL_CACHELINE_SIZE - sizeof(TaskFunc) // func size
                                                - sizeof(TaskID) // parentID size
                                                -
                                                sizeof(std::atomic<uint16>); // unfinishedCount size

    void* storage[STORAGE_SIZE_BYTE / sizeof(void*)] = {};
    TaskFunc func = nullptr;
    TaskID parent_id;
    std::atomic<uint16> unfinished_count = {0};
  };

  static_assert(
    sizeof(Task) == SOUL_CACHELINE_SIZE, "Task must be the same size as cache line size.");

  class TaskDeque
  {
  public:
    TaskID _tasks[Constant::MAX_TASK_PER_THREAD];
    std::atomic<int32> _bottom;
    std::atomic<int32> _top;

    auto init() -> void;
    auto shutdown() -> void;

    auto reset() -> void;
    auto push(TaskID task) -> void;
    auto pop() -> TaskID; // return nullptr if empty
    auto steal()
      -> TaskID; // return nullptr if empty or fail (other thread do steal operation concurrently)
  };

  struct alignas(SOUL_CACHELINE_SIZE) ThreadContext {
    TaskDeque task_deque;

    Task task_pool[Constant::MAX_TASK_PER_THREAD];
    uint16 task_count = 0;

    uint16 thread_index = 0;

    Vector<memory::Allocator*> allocator_stack{nullptr};
    TempAllocator* temp_allocator = nullptr;
  };

  struct Database {
    thread_local static ThreadContext* g_thread_context;
    FixedVector<ThreadContext> thread_contexts;
    std::thread threads[Constant::MAX_THREAD_COUNT];

    std::condition_variable wait_cond_var;
    std::mutex wait_mutex;

    std::condition_variable loop_cond_var;
    std::mutex loop_mutex;

    std::atomic<bool> is_terminated;

    soul_size active_task_count = 0;
    ThreadCount thread_count = 0;

    memory::Allocator* default_allocator = nullptr;
    soul_size temp_allocator_size = 0;

    Database() : thread_contexts(nullptr) {}
  };

  template <typename Func>
  struct ParallelForTaskData {
    using ParallelForFunc = Func;

    explicit ParallelForTaskData(
      uint32 start, uint32 count, uint32 min_count, ParallelForFunc&& func)
        : start(start),
          count(count),
          min_count(min_count),
          func(std::forward<ParallelForFunc>(func))
    {
    }

    uint32 start;
    uint32 count;
    uint32 min_count;
    ParallelForFunc func;
  };

  template <typename T>
  concept execution = ts_fn<T, void, TaskID>;

} // namespace soul::runtime
