#pragma once

#include "core/type.h"
#include "core/type_traits.h"
#include "runtime/data.h"

#define SOUL_ASSERT_MAIN_THREAD()                                                                  \
  SOUL_ASSERT(                                                                                     \
    0,                                                                                             \
    soul::runtime::System::get().get_thread_id() == 0,                                             \
    "This method is not thread safe. Please only call it only from main thread!")

namespace soul::runtime
{

  class System
  {
  public:
    System() = default;

    System(const System&) = delete;

    auto operator=(const System&) -> System& = delete;

    System(System&&) = delete;

    auto operator=(System&&) -> System& = delete;

    ~System() = default;

    /*
      Initialize the Runtime System. Only call this from the main thread.
    */
    void init(const Config& config);

    /*
      Cleanup all runtime system resource. Only call this from the main thread.
    */
    void shutdown();

    void begin_frame();

    template <execution Execute>
    auto create_task(TaskID parent, Execute&& lambda) -> TaskID
    {
      static_assert(
        sizeof lambda <= sizeof(Task::storage),
        "Lambda size is too big."
        "Consider increase the storage size of the"
        "task or dynamically allocate the memory.");

      auto call = [](TaskID taskID, void* data) {
        Execute& lambda = *static_cast<Execute*>(data);
        lambda(taskID);
        lambda.~Execute();
      };

      const TaskID task_id = create_task(parent, call);
      Task* task = get_task_ptr(task_id);
      new (task->storage) Execute(std::forward<Execute>(lambda));
      return task_id;
    }

    template <typename Func>
    auto create_parallel_for_task_recursive(
      TaskID parent, u32 start, u32 data_count, u32 block_size, Func&& func) -> TaskID
    {
      using TaskData = ParallelForTaskData<Func>;

      static_assert(
        sizeof(TaskData) <= sizeof(Task::storage),
        "ParallelForTaskData size is too big. TaskData = %d"
        "Consider to increase the storage size of the task.");

      auto parallel_func = [](TaskID taskID, void* data) {
        TaskData& task_data = (*static_cast<TaskData*>(data));
        if (task_data.count > task_data.min_count) {
          const u32 left_count = task_data.count / 2;
          const TaskID left_task_id = get().create_parallel_for_task_recursive(
            taskID, task_data.start, left_count, task_data.min_count, task_data.func);
          get().task_run(left_task_id);

          const u32 right_count = task_data.count - left_count;
          const TaskID right_task_id = get().create_parallel_for_task_recursive(
            taskID, task_data.start + left_count, right_count, task_data.min_count, task_data.func);
          get().task_run(right_task_id);
        } else {
          for (usize i = 0; i < task_data.count; i++) {
            task_data.func(task_data.start + i);
          }
        }
      };

      const TaskID task_id = create_task(parent, std::move(parallel_func));
      Task* task = get_task_ptr(task_id);
      new (task->storage) TaskData(start, data_count, block_size, std::forward<Func>(func));
      return task_id;
    }

    template <ts_fn<void, int> Fn>
    auto create_parallel_for_task(TaskID parent, u32 count, u32 block_size, Fn&& func) -> TaskID
    {
      return create_parallel_for_task_recursive(
        parent, 0, count, block_size, std::forward<Fn>(func));
    }

    void wait_task(TaskID task_id);

    void task_run(TaskID task_id);

    auto get_thread_count() const -> u16;

    static auto get_thread_id() -> u16;

    auto get_thread_context() -> ThreadContext&;

    static auto get() -> System&
    {
      [[clang::no_destroy]] static System instance;
      return instance;
    }

    void push_allocator(memory::Allocator* allocator);

    void pop_allocator();

    auto get_context_allocator() -> memory::Allocator*;

    auto allocate(u32 size, u32 alignment) -> void*;

    void deallocate(void* addr, u32 size);

    auto get_temp_allocator() -> TempAllocator*;

  private:
    auto create_task(TaskID parent, TaskFunc func) -> TaskID;

    auto get_task_ptr(TaskID task_id) -> Task*;

    void finish_task(Task* task);

    static auto is_task_complete(Task* task) -> b8;

    void loop(ThreadContext* thread_context);

    void execute(TaskID task);

    void terminate();

    void init_root_task();

    Database db_;
  };
} // namespace soul::runtime
