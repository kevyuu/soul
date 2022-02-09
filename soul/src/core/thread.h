#pragma once
#include <atomic>

namespace soul
{
	class RWSpinLock {
	public:
		enum { Reader = 2, Writer = 1 };
		RWSpinLock() {
			counter.store(0);
		}

		inline void lockRead() {
			unsigned v = counter.fetch_add(Reader, std::memory_order_acquire);
			while ((v & Writer) != 0) {
				v = counter.load(std::memory_order_acquire);
			}
		}

		inline void unlockRead() {
			counter.fetch_sub(Reader, std::memory_order_release);
		}

		inline void lockWrite() {
			uint32_t expected = 0;
			while (!counter.compare_exchange_weak(expected, Writer,
				std::memory_order_acquire,
				std::memory_order_relaxed)) {
				expected = 0;
			}
		}

		inline void unlockWrite() {
			counter.fetch_and(~Writer, std::memory_order_release);
		}

	private:
		std::atomic<uint32_t>counter;
	};
}
