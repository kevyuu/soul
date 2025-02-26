#include <thread>

#include "core/architecture.h"
#include "core/panic_format.h"
#include "core/profile.h"
#include "core/string.h"
#include "core/util.h"
#include "memory/allocators/linear_allocator.h"
#include "memory/allocators/proxy_allocator.h"
#include "runtime/data.h"
#include "runtime/system.h"

namespace soul::runtime
{

  thread_local ThreadContext* Database::g_thread_context = nullptr; // NOLINT

  void System::execute(TaskID task_id)
  {
    {
      std::lock_guard<std::mutex> lock(db_.loop_mutex);
      db_.active_task_count -= 1;
    }
    Task* task = get_task_ptr(task_id);
    task->func(task_id, task->storage);
    finish_task(task);
  }

  void System::loop(ThreadContext* thread_context)
  {
    Database::g_thread_context = thread_context;

    const auto thread_name = String::Format("Worker Thread = {}", get_thread_id());
    SOUL_PROFILE_THREAD_SET_NAME(thread_name.data());

    memory::LinearAllocator linear_allocator(
      "runtime::System::loop"_str, 20 * ONE_MEGABYTE, get_context_allocator());
    TempAllocator temp_allocator(&linear_allocator, TempProxy::Config());
    thread_context->temp_allocator = &temp_allocator;

    while (true)
    {
      TaskID task_id = thread_context->task_deque.pop();
      while (task_id == TaskID::NULLVAL())
      {

        {
          std::unique_lock<std::mutex> lock(db_.loop_mutex);
          while (db_.active_task_count == 0 && !db_.is_terminated.load(std::memory_order_relaxed))
          {
            db_.loop_cond_var.wait(lock);
          }
        }

        if (db_.is_terminated.load(std::memory_order_relaxed))
        {
          break;
        }

        const usize thread_index = (util::get_random_u32() % db_.thread_count);
        task_id                  = db_.thread_contexts[thread_index].task_deque.steal();
      }

      if (db_.is_terminated.load(std::memory_order_relaxed))
      {
        break;
      }
      execute(task_id);
    }
  }

  void System::terminate()
  {
    db_.is_terminated.store(true);
    {
      std::lock_guard<std::mutex> lock(db_.loop_mutex);
    }
    db_.loop_cond_var.notify_all();
  }

  void System::begin_frame()
  {
    SOUL_PROFILE_ZONE();
    SOUL_ASSERT_MAIN_THREAD();

    wait_task(TaskID::NULLVAL());

    init_root_task();

    db_.thread_contexts[0].temp_allocator->reset();

    for (usize i = 1; i < db_.thread_count; i++)
    {
      db_.thread_contexts[i].task_count = 0;
      db_.thread_contexts[i].task_deque.reset();
      db_.thread_contexts[i].temp_allocator->reset();
    }
  }

  auto System::create_task(TaskID parent, TaskFunc func) -> TaskID
  {
    ThreadContext* thread_state = Database::g_thread_context;

    const auto thread_index = thread_state->thread_index;
    const auto task_index   = thread_state->task_count;
    const auto task_id      = TaskID(thread_index, task_index);

    thread_state->task_count += 1;
    Task& task     = thread_state->task_pool[task_index];
    task.parent_id = parent;
    task.unfinished_count.store(1, std::memory_order_relaxed);
    task.func = func;

    Task* parent_task = get_task_ptr(parent);
    parent_task->unfinished_count.fetch_add(1, std::memory_order_relaxed);

    return task_id;
  }

  auto System::get_task_ptr(TaskID task_id) -> Task*
  {
    const usize thread_index = task_id.get_thread_index();
    const usize task_index   = task_id.get_task_index();

    return &db_.thread_contexts[thread_index].task_pool[task_index];
  }

  auto System::is_task_complete(Task* task) -> b8
  {
    // NOTE(kevinyu): Synchronize with fetch_sub in _finishTask() to make sure the task is executed
    // before we return true.
    return task->unfinished_count.load(std::memory_order_acquire) == 0;
  }

  // TODO(kevinyu) : Implement stealing from other deque when waiting for task.
  // Currently we only try to pop task from our thread local deque.
  void System::wait_task(TaskID task_id)
  {
    ThreadContext* thread_state = Database::g_thread_context;
    Task* task_to_wait          = get_task_ptr(task_id);
    while (!is_task_complete(task_to_wait))
    {

      TaskID task_to_do = thread_state->task_deque.pop();

      if (!task_to_do.is_null())
      {
        execute(task_to_do);
      } else
      {
        std::unique_lock<std::mutex> lock(db_.wait_mutex);
        while (!is_task_complete(task_to_wait))
        {
          db_.wait_cond_var.wait(lock);
        }
      }
    }
  }

  void System::init_root_task()
  {
    // NOTE(kevinyu): TaskID 0 is ROOT. TaskID 0 is used as parent for all task
    db_.thread_contexts[0].task_pool[0].unfinished_count.store(0, std::memory_order_relaxed);
    db_.thread_contexts[0].task_count = 1;
    db_.thread_contexts[0].task_deque.reset();
  }

  auto System::is_worker_thread() const -> b8
  {
    return db_.g_thread_context != nullptr;
  }

  void System::init(const Config& config)
  {
    db_.default_allocator   = config.defaultAllocator;
    db_.temp_allocator_size = config.workerTempAllocatorSize;

    const auto thread_count = config.threadCount != 0
                                ? config.threadCount
                                : soul::cast<ThreadCount>(get_hardware_thread_count());

    SOUL_ASSERT_FORMAT(
      0,
      thread_count <= Constant::MAX_THREAD_COUNT,
      "Thread count : {} is more than MAX_THREAD_COUNT : {}",
      thread_count,
      Constant::MAX_THREAD_COUNT);
    db_.thread_count = thread_count;

    db_.thread_contexts.init_generate(
      config.defaultAllocator,
      thread_count,
      [&config](usize idx) -> ThreadContext
      {
        return ThreadContext{
          .task_count      = 0,
          .thread_index    = static_cast<u16>(idx),
          .allocator_stack = Vector<memory::Allocator*>(config.defaultAllocator),
        };
      });

    // NOTE(kevinyu): i == 0 is for main thread
    Database::g_thread_context = db_.thread_contexts.data();

    for (auto& thread_context : db_.thread_contexts)
    {
      thread_context.task_deque.init();
    }

    db_.is_terminated.store(false, std::memory_order_relaxed);
    for (u16 i = 1; i < thread_count; ++i)
    {
      db_.threads[i] = std::thread(&System::loop, this, &db_.thread_contexts[i]);
    }
    db_.active_task_count = 0;

    get_thread_context().temp_allocator = config.mainThreadTempAllocator;

    init_root_task();
  }

  void System::task_run(TaskID task_id)
  {
    Database::g_thread_context->task_deque.push(task_id);
    {
      std::lock_guard<std::mutex> lock(db_.loop_mutex);
      db_.active_task_count += 1;
    }
    db_.loop_cond_var.notify_all();
  }

  void System::finish_task(Task* task)
  {
    // NOTE(kevinyu): make sure _isCompleteTask() return true only after the task truly finish.
    // Without std::memory_order_release, this operation can be reorder before the task is executed.
    const auto unfinished_count = task->unfinished_count.fetch_sub(1, std::memory_order_release);

    if (unfinished_count == 1)
    {
      // NOTE(kevinyu): This empty lock prevent bug where waitTask() check _isTaskComplete() and
      // then get preempted by OS. For example if waitTask() check _isTaskComplete() and it return
      // false, then before it execute wait() on the condition variable, it get preempted by OS. In
      // the mean time, another thread can call _finishTask(), without this lock _finishTask() will
      // be able to notify. When the preempted waitTask() run again, the task is already completed
      // but we still sleep, and because notify has been called on _finishTask, the thread might
      // sleep forever.
      {
        std::lock_guard<std::mutex> lock(db_.wait_mutex);
      }
      db_.wait_cond_var.notify_all();

      if (task != get_task_ptr(TaskID::NULLVAL()))
      {
        finish_task(get_task_ptr(task->parent_id));
      }
    }
  }

  void System::shutdown()
  {
    SOUL_ASSERT_MAIN_THREAD();

    SOUL_ASSERT_FORMAT(
      0,
      db_.active_task_count == 0,
      "There is still pending task in work deque! Active Task Count = {}.",
      db_.active_task_count);
    terminate();
    for (u64 i = 1; i < db_.thread_count; i++)
    {
      db_.threads[i].join();
    }
    db_.thread_contexts.cleanup();
  }

  auto System::get_thread_count() const -> u16
  {
    return db_.thread_count;
  }

  auto System::get_thread_id() -> u16
  {
    return Database::g_thread_context->thread_index;
  }

  auto System::get_thread_context() -> ThreadContext&
  {
    return db_.thread_contexts[get_thread_id()];
  }

  void System::push_allocator(memory::Allocator* allocator)
  {
    SOUL_ASSERT(0, db_.default_allocator != nullptr);
    get_thread_context().allocator_stack.push_back(allocator);
  }

  void System::pop_allocator()
  {
    SOUL_ASSERT(0, !get_thread_context().allocator_stack.empty());
    get_thread_context().allocator_stack.pop_back();
  }

  auto System::get_context_allocator() -> memory::Allocator*
  {
    if (get_thread_context().allocator_stack.empty())
    {
      return db_.default_allocator;
    }
    return get_thread_context().allocator_stack.back();
  }

  auto System::allocate(u32 size, u32 alignment) -> void*
  {
    return get_context_allocator()->allocate(size, alignment);
  }

  void System::deallocate(void* addr, u32 /* size */)
  {
    get_context_allocator()->deallocate(addr);
  }

  auto System::get_temp_allocator() -> TempAllocator*
  {
    SOUL_ASSERT(0, get_thread_context().temp_allocator != nullptr);
    return get_thread_context().temp_allocator;
  }

} // namespace soul::runtime
