#pragma once
#include <atomic>
#include "dev_util.h"

#include <mutex>
#include <shared_mutex>

namespace soul
{
	template <typename T>
	concept is_lockable_v = requires (T lock)
	{
		lock.lock();
		lock.unlock();
		{ lock.try_lock() } -> std::same_as<bool>;
	};

	template <typename T>
	concept is_shared_lockable_v = requires(T lock)
	{
		lock.lock_shared();
		lock.unlock_shared();
		lock.lock();
		lock.unlock();
	};

	class Mutex
	{
	public:
		void lock() { mutex_.lock(); }
		bool try_lock() { return mutex_.try_lock(); }
		void unlock() { return mutex_.unlock(); }

	private:
		SOUL_LOCKABLE(std::mutex, mutex_);
	};
	static_assert(is_lockable_v<Mutex>);

	class NullMutex
	{
	public:
		void lock() {}
		bool try_lock() { return true; }
		void unlock() {}
	};
	static_assert(is_lockable_v<NullMutex>);

	class SharedMutex
	{
	public:
		void lock() { mutex_.lock(); }
		void unlock() { mutex_.unlock(); }
		void lock_shared() { mutex_.lock_shared(); }
		void unlock_shared() { mutex_.unlock_shared(); }
	private:
		SOUL_SHARED_LOCKABLE(std::shared_mutex, mutex_);
	};
	static_assert(is_shared_lockable_v<SharedMutex>);

	class NullSharedMutex
	{
	public:
		void lock() {}
		void unlock() {}
		void lock_shared() {}
		void unlock_shared() {}
	};
	static_assert(is_shared_lockable_v<NullSharedMutex>);

	class RWSpinMutex
	{
	public:
		void lock() { mutex_.lock(); }
		void unlock() { mutex_.unlock(); }
		void lock_shared() { mutex_.lock_shared(); }
		void unlock_shared() { mutex_.unlock_shared(); }
	private:
		class InternalMutex {
		public:
			enum { Reader = 2, Writer = 1 };
			InternalMutex() {
				counter.store(0);
			}

			void lock_shared() {
				unsigned v = counter.fetch_add(Reader, std::memory_order_acquire);
				while ((v & Writer) != 0) {
					v = counter.load(std::memory_order_acquire);
				}
			}

			void unlock_shared() {
				counter.fetch_sub(Reader, std::memory_order_release);
			}

			void lock() {
				uint32_t expected = 0;
				while (!counter.compare_exchange_weak(expected, Writer,
					std::memory_order_acquire,
					std::memory_order_relaxed)) {
					expected = 0;
				}
			}

			void unlock() {
				counter.fetch_and(~Writer, std::memory_order_release);
			}

		private:
			std::atomic<uint32_t> counter;
		};

		SOUL_SHARED_LOCKABLE(InternalMutex, mutex_);
	};
	
	static_assert(is_shared_lockable_v<RWSpinMutex>);
}
