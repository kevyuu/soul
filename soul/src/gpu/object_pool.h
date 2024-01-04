#pragma once

#include "core/config.h"
#include "core/mutex.h"
#include "core/vector.h"
#include "memory/allocator.h"

namespace soul::gpu
{

  template <typename T, is_lockable_v Lockable = Mutex, size_t BLOCK_SIZE = 512>
  class ConcurrentObjectPool
  {
  public:
    struct ID
    {
      T* obj                            = nullptr;
      u64 cookie                        = 0;
      auto operator<=>(const ID&) const = default;
    };

    static constexpr ID NULLVAL = {nullptr, 0};

    explicit ConcurrentObjectPool(memory::Allocator* allocator = get_default_allocator()) noexcept
        : allocator_(allocator)
    {
    }

    ConcurrentObjectPool(const ConcurrentObjectPool&)                        = delete;
    auto operator=(const ConcurrentObjectPool& rhs) -> ConcurrentObjectPool& = delete;
    ConcurrentObjectPool(ConcurrentObjectPool&&)                             = delete;
    auto operator=(ConcurrentObjectPool&& rhs) -> ConcurrentObjectPool&      = delete;

    ~ConcurrentObjectPool()
    {
      auto num_objects = BLOCK_SIZE / sizeof(T);
      for (T* memory : memories_)
      {
        allocator_->deallocate(memory);
      }
    }

    template <typename... ARGS>
    auto create(ARGS&&... args) -> ID
    {
      std::lock_guard guard(mutex_);
      if (vacants_.empty())
      {
        auto num_objects = BLOCK_SIZE / sizeof(T);
        T* memory = static_cast<T*>(allocator_->allocate(num_objects * sizeof(T), alignof(T), ""));
        if (!memory)
        {
          return NULLVAL;
        }
        for (usize object_idx = 0; object_idx < num_objects; object_idx++)
        {
          vacants_.push_back(&memory[object_idx]);
        }
        memories_.push_back(memory);
      }
      T* ptr = vacants_.back();
      vacants_.pop_back();
      new (ptr) T(std::forward<ARGS>(args)...);
      return {ptr, cookie++};
    }

    auto destroy(ID id) -> void
    {
      id.obj->~T();
      std::lock_guard guard(mutex_);
      vacants_.push_back(id.obj);
    }

    auto get(ID id) const -> T*
    {
      return id.obj;
    }

  private:
    mutable Lockable mutex_;
    Vector<T*> vacants_;
    Vector<T*> memories_;
    memory::Allocator* allocator_;
    u64 cookie = 0;
  };

  template <typename T, size_t BLOCK_SIZE = 512>
  using ObjectPool = ConcurrentObjectPool<T, NullMutex, BLOCK_SIZE>;

} // namespace soul::gpu
