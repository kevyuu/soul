#pragma once

#include <atomic>
#include <thread>
#include <condition_variable>
#include <mutex>

#include "core/type.h"
#include "core/architecture.h"

#include "core/array.h"
#include "core/pool.h"
#include "core/static_array.h"

namespace Soul {
	namespace Job {
		struct Constant {

			static constexpr uint16 MAX_THREAD_COUNT = 16;
			static constexpr uint16 MAX_TASK_PER_THREAD = 4096;

			static constexpr uint16 TASK_ID_THREAD_INDEX_MASK = 0xF000;
			static constexpr uint16 TASK_ID_THREAD_INDEX_SHIFT = 12;
			static constexpr uint16 TASK_ID_TASK_INDEX_MASK = 0x0FFF;
			static constexpr uint16 TASK_ID_TASK_INDEX_SHIFT = 0;

		};

		using TaskID = uint16;

		struct Task;
		using TaskFunc = void(*)(TaskID taskID, void* data);

		struct alignas(SOUL_CACHELINE_SIZE) Task {

			static constexpr uint32 STORAGE_SIZE_BYTE = SOUL_CACHELINE_SIZE
				- sizeof(void*) // func size
				- 2 // parentID size
				- 2; // unfinishedCount size
				
			void* storage[STORAGE_SIZE_BYTE / sizeof(void*)];
			TaskFunc func;
			TaskID parentID;
			std::atomic<uint16> unfinishedCount;
		
		};
		static_assert(sizeof(Task) == SOUL_CACHELINE_SIZE, "Task must be the same size as cache line size.");

		class TaskDeque {
		public:
			TaskID _tasks[Constant::MAX_TASK_PER_THREAD];
			std::atomic<int32> _bottom;
			std::atomic<int32> _top;
		
			void init();
			void shutdown();

			void reset();
			void push(TaskID task);
			TaskID pop(); // return nullptr if empty
			TaskID steal(); // return nullptr if empty or fail (other thread do steal operation concurrently)

		};

		struct alignas(SOUL_CACHELINE_SIZE) _ThreadContext {
			TaskDeque taskDeque;

			Task taskPool[Constant::MAX_TASK_PER_THREAD];
			uint16 taskCount;

			uint16 threadIndex;
		};

		struct Database {
			thread_local static _ThreadContext* gThreadContext;
			StaticArray<_ThreadContext> threadContexts;
			std::thread threads[Constant::MAX_THREAD_COUNT];
			
			std::condition_variable waitCondVar;
			std::mutex waitMutex;

			std::condition_variable loopCondVar;
			std::mutex loopMutex;

			std::atomic<bool> isTerminated;
			
			uint16 activeTaskCount;
			uint16 threadCount;
		};

		template<typename Func>
		struct ParallelForTaskData {
			using ParallelForFunc = Func;

			explicit ParallelForTaskData(
				uint32 start, 
				uint32 count, 
				uint32 minCount, 
				ParallelForFunc&& func): 
				start(start), count(count), minCount(minCount), func(std::forward<ParallelForFunc>(func)) {}

			uint32 start;
			uint32 count;
			uint32 minCount;
			ParallelForFunc func;
		};

	}
}