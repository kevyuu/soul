#pragma once
#include <atomic>
#include "dev_util.h"

#include <mutex>
#include <shared_mutex>

namespace soul
{
	class Mutex
	{
	public:
		inline void lock() { mutex_.lock(); }
		inline bool try_lock() { return mutex_.try_lock(); }
		inline void unlock() { return mutex_.unlock(); }

	private:
		SOUL_LOCKABLE(std::mutex, mutex_, "");
	};

	class RWSpinMutex {
	public:
		enum { Reader = 2, Writer = 1 };
		RWSpinMutex() {
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
		std::atomic<uint32_t>counter;
	};
}
