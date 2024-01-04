#pragma once

#include <atomic>
#include <mutex>
#include <shared_mutex>

#include "core/type.h"
#include "profile.h"

namespace soul
{

  template <typename T>
  concept is_lockable_v = requires(T lock) {
    lock.lock();
    lock.unlock();
    {
      lock.try_lock()
    } -> std::same_as<b8>;
  };

  template <typename T>
  concept is_shared_lockable_v = requires(T lock) {
    lock.lock_shared();
    lock.unlock_shared();
    lock.lock();
    lock.unlock();
  };

  class Mutex
  {
  public:
    auto lock() -> void
    {
      mutex_.lock();
    }

    auto try_lock() -> b8
    {
      return mutex_.try_lock();
    }

#pragma warning(push)
#pragma warning(disable : 26110)

    auto unlock() -> void
    {
      mutex_.unlock();
    }

#pragma warning(pop)
  private:
    SOUL_LOCKABLE(std::mutex, mutex_);
  };

  static_assert(is_lockable_v<Mutex>);

  class NullMutex
  {
  public:
    auto lock() -> void {}

    static auto try_lock() -> b8
    {
      return true;
    }

    auto unlock() -> void {}
  };

  static_assert(is_lockable_v<NullMutex>);

  class SharedMutex
  {
  public:
    auto lock() -> void
    {
      mutex_.lock();
    }

    auto unlock() -> void
    {
      mutex_.unlock();
    }

    auto lock_shared() -> void
    {
      mutex_.lock_shared();
    }

    auto unlock_shared() -> void
    {
      mutex_.unlock_shared();
    }

  private:
    SOUL_SHARED_LOCKABLE(std::shared_mutex, mutex_);
  };

  static_assert(is_shared_lockable_v<SharedMutex>);

  class NullSharedMutex
  {
  public:
    auto lock() -> void {}

    auto unlock() -> void {}

    auto lock_shared() -> void {}

    auto unlock_shared() -> void {}
  };

  static_assert(is_shared_lockable_v<NullSharedMutex>);

  class RWSpinMutex
  {
  public:
    auto lock() -> void
    {
      mutex_.lock();
    }

    auto unlock() -> void
    {
      mutex_.unlock();
    }

    auto lock_shared() -> void
    {
      mutex_.lock_shared();
    }

    auto unlock_shared() -> void
    {
      mutex_.unlock_shared();
    }

  private:
    class InternalMutex
    {
    public:
      static constexpr uint32_t READER = 2;
      static constexpr uint32_t WRITER = 1;

      InternalMutex()
      {
        counter.store(0);
      }

      auto lock_shared() -> void
      {
        auto v = counter.fetch_add(READER, std::memory_order_acquire);
        while ((v & WRITER) != 0)
        {
          v = counter.load(std::memory_order_acquire);
        }
      }

      auto unlock_shared() -> void
      {
        counter.fetch_sub(READER, std::memory_order_release);
      }

      auto lock() -> void
      {
        uint32_t expected = 0;
        while (!counter.compare_exchange_weak(
          expected, WRITER, std::memory_order_acquire, std::memory_order_relaxed))
        {
          expected = 0;
        }
      }

      auto unlock() -> void
      {
        counter.fetch_and(~WRITER, std::memory_order_release);
      }

    private:
      std::atomic<uint32_t> counter;
    };

    SOUL_SHARED_LOCKABLE(InternalMutex, mutex_);
  };

  static_assert(is_shared_lockable_v<RWSpinMutex>);

} // namespace soul
