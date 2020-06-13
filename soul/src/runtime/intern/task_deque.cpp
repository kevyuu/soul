#include "core/math.h"
#include "runtime/data.h"
#include "runtime/system.h"

namespace Soul {
	namespace Runtime {

		void TaskDeque::init() {
			SOUL_ASSERT_MAIN_THREAD();
			_bottom.store(0, std::memory_order_relaxed);
			_top.store(0, std::memory_order_relaxed);
		}

		void TaskDeque::shutdown() {
			SOUL_ASSERT_MAIN_THREAD();
			free(_tasks);
		}

		void TaskDeque::reset() {
			SOUL_ASSERT_MAIN_THREAD();

			SOUL_ASSERT(0, _bottom.load(std::memory_order_relaxed) == _top.load(std::memory_order_relaxed),
				"Reset cannot be called when deque is not empty");

			// TODO(kevinyu): Is memory_order_seq_cst here necessary or even enough?
			_bottom.store(0, std::memory_order_seq_cst);
			_top.store(0, std::memory_order_seq_cst);
		}

		void TaskDeque::push(TaskID task) {
			int32 bottom = _bottom.load(std::memory_order_relaxed);
			SOUL_ASSERT(0, bottom < Constant::MAX_TASK_PER_THREAD, "Number of task exceed capacity."
				"You can configure capacity in Runtime::Constant::MAX_TASK_PER_THREAD.");
			_tasks[bottom] = task;

			// NOTE(kevinyu): make sure steal() can see the item that is push to the deque
			_bottom.store(bottom + 1, std::memory_order_release);
		}

		TaskID TaskDeque::pop() {
			// NOTE(kevinyu): bottom must be fetched before top hence memory_order_acquire here
			int32 bottom = _bottom.fetch_sub(1, std::memory_order_acquire);
			bottom = bottom - 1;
			int32 top = _top.load(std::memory_order_acquire);

			if (bottom < top) {
				_bottom.store(top);
				return 0;
			}
			
			if (bottom > top) {
				return _tasks[bottom];
			}

			// NOTE(kevinyu): bottom == top. last element case. pretend to steal
			TaskID task = 0;
			if (_top.compare_exchange_strong(top, top + 1, std::memory_order_acq_rel, std::memory_order_relaxed)) {
				top += 1;
				task = _tasks[bottom];
			}

			_bottom.store(top, std::memory_order_relaxed);
			return task;
		}

		TaskID TaskDeque::steal() {
			// NOTE(kevinyu): top must be fetch before bottom
			int32 top = _top.load(std::memory_order_acquire);

			// NOTE(kevinyu): make sure item that is pushed to the deque from push() is visible after fetching bottom
			int32 bottom = _bottom.load(std::memory_order_acquire);
			
			if (top >= bottom) {
				return 0;
			}

			TaskID task = _tasks[top];
			if (_top.compare_exchange_strong(top, top + 1, std::memory_order_acq_rel, std::memory_order_relaxed)) {
				return task;
			}

			return 0;
		}

	}
}