#pragma once

#include <atomic>
#include <thread>
#include <condition_variable>
#include <mutex>

#include "core/architecture.h"
#include "core/array.h"
#include "core/static_array.h"
#include "core/type.h"

#include "memory/allocator.h"
#include "memory/allocators/linear_allocator.h"
#include "memory/allocators/proxy_allocator.h"
#include "memory/allocators/malloc_allocator.h"

namespace Soul::Runtime
{
	using ThreadCount = uint16;

	using TempProxy = Memory::NoOpProxy;
	using TempAllocator = Memory::ProxyAllocator<Memory::LinearAllocator, TempProxy>;

	using DefaultAllocatorProxy = Memory::MultiProxy<
		Memory::MutexProxy,
		Memory::CounterProxy,
		Memory::ClearValuesProxy,
		Memory::BoundGuardProxy>;
	using DefaultAllocator = Memory::ProxyAllocator<
		Memory::MallocAllocator,
		DefaultAllocatorProxy>;

	struct Config {
		uint16 threadCount; // 0 to use hardware thread count
		uint16 taskPoolCount;
		TempAllocator* mainThreadTempAllocator;
		uint64 workerTempAllocatorSize;
		DefaultAllocator* defaultAllocator;
	};

	struct Constant {

		static constexpr uint16 MAX_THREAD_COUNT = 16;
		static constexpr uint16 MAX_TASK_PER_THREAD = 4096;

		static constexpr uint16 TASK_ID_THREAD_INDEX_MASK = 0xF000;
		static constexpr uint16 TASK_ID_THREAD_INDEX_SHIFT = 12;
		static constexpr uint16 TASK_ID_TASK_INDEX_MASK = 0x0FFF;
		static constexpr uint16 TASK_ID_TASK_INDEX_SHIFT = 0;

	};


	struct Task;
	// NOTE(kevinyu): We use id == 0 as both root and null value;
	struct TaskID {
		uint16 id;
		static constexpr TaskID NULLVAL() { return TaskID(0, 0); }
		static constexpr TaskID ROOT() { return NULLVAL(); }
		constexpr TaskID() : id(NULLVAL().id) {}
		constexpr TaskID(uint16 threadIndex, uint16 taskIndex) : 
			id(uint16(threadIndex << Constant::TASK_ID_THREAD_INDEX_SHIFT | (taskIndex << Constant::TASK_ID_TASK_INDEX_SHIFT)))
		{}

		SOUL_NODISCARD uint16 threadIndex() const {
			return (id & Constant::TASK_ID_THREAD_INDEX_MASK) >> Constant::TASK_ID_THREAD_INDEX_SHIFT;
		}

		SOUL_NODISCARD uint16 taskIndex() const {
			return (id & Constant::TASK_ID_TASK_INDEX_MASK) >> Constant::TASK_ID_TASK_INDEX_SHIFT;
		}

		SOUL_NODISCARD bool operator==(const TaskID& other) const { return other.id == id; }
		SOUL_NODISCARD bool operator!=(const TaskID& other) const { return other.id != id; }
		SOUL_NODISCARD bool isRoot() const { return id == ROOT().id; }
		SOUL_NODISCARD bool isNull() const { return id == NULLVAL().id; }
	};
		
		
	using TaskFunc = void(*)(TaskID taskID, void* data);

	struct alignas(SOUL_CACHELINE_SIZE) Task {

		static constexpr uint32 STORAGE_SIZE_BYTE = SOUL_CACHELINE_SIZE
			- sizeof(void*) // func size
			- 2 // parentID size
			- 2; // unfinishedCount size
				
		void* storage[STORAGE_SIZE_BYTE / sizeof(void*)] = {};
		TaskFunc func = nullptr;
		TaskID parentID;
		std::atomic<uint16> unfinishedCount = { 0 };
		
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

	struct alignas(SOUL_CACHELINE_SIZE) ThreadContext {
		TaskDeque taskDeque;

		Task taskPool[Constant::MAX_TASK_PER_THREAD];
		uint16 taskCount = 0;

		uint16 threadIndex = 0;

		Array<Memory::Allocator*> allocatorStack{nullptr};
		TempAllocator* tempAllocator = nullptr;
	};

	struct Database {
		thread_local static ThreadContext* gThreadContext;
		StaticArray<ThreadContext> threadContexts;
		std::thread threads[Constant::MAX_THREAD_COUNT];
			
		std::condition_variable waitCondVar;
		std::mutex waitMutex;

		std::condition_variable loopCondVar;
		std::mutex loopMutex;

		std::atomic<bool> isTerminated;
			
		soul_size activeTaskCount = 0;
		ThreadCount threadCount = 0;

		Memory::Allocator* defaultAllocator = nullptr;
		soul_size tempAllocatorSize = 0;

		Database() : threadContexts(nullptr) {}
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
