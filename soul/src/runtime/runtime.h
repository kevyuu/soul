#pragma once

#include "core/type_traits.h"
#include "runtime/data.h"
#include "runtime/system.h"

namespace Soul::Runtime {

	inline void Init(const Config& config) {
		System::Get().init(config);
	}

	inline void Shutdown() {
		System::Get().shutdown();
	}

	inline void BeginFrame() {
		System::Get().beginFrame();
	}

	template<
		typename Execute,
		typename = require<is_lambda_v<Execute, void(TaskID)>>
	>
	inline TaskID CreateTask(TaskID parent, Execute&& lambda) {
		return System::Get().taskCreate(parent, std::forward<Execute>(lambda));
	}

	inline void WaitTask(TaskID taskID) {
		System::Get().taskWait(taskID);
	}

	inline void RunTask(TaskID taskID) {
		System::Get().taskRun(taskID);
	}

	template<
		typename Execute,
		typename = require<is_lambda_v<Execute, void(TaskID)>>
	>
	TaskID CreateAndRunTask(TaskID parent, Execute&& lambda) {
		const TaskID taskID = CreateTask(parent, std::forward<Execute>(lambda));
		RunTask(taskID);
		return taskID;
	}

	template<
		typename Func,
		typename = require<is_lambda_v<Func, void(int)>>
	>
	inline TaskID ParallelForTaskCreate(TaskID parent, uint32 count, uint32 blockSize, Func&& func) {
		return System::Get()._parallelForTaskCreateRecursive(parent, 0, count, blockSize, std::forward<Func>(func));
	}

	inline uint16 GetThreadId() {
		return System::Get().getThreadID();
	}

	inline uint16 GetThreadCount() {
		return System::Get().getThreadCount();
	}

	inline void PushAllocator(Memory::Allocator* allocator) {
		System::Get().pushAllocator(allocator);
	}

	inline void PopAllocator() {
		System::Get().popAllocator();
	}

	inline Memory::Allocator* GetContextAllocator() {
		return System::Get().getContextAllocator();
	}

	inline TempAllocator* GetTempAllocator() {
		return System::Get().getTempAllocator();
	}

	inline void* Allocate(uint32 size, uint32 alignment) {
		return System::Get().allocate(size, alignment);
	}

	inline void Deallocate(void* addr, uint32 size) {
		return System::Get().deallocate(addr, size);
	}

	struct AllocatorInitializer {
		AllocatorInitializer() = delete;
		explicit AllocatorInitializer(Memory::Allocator* allocator) {
			PushAllocator(allocator);
		}

		void end() {
			PopAllocator();
		}
	};

	struct AllocatorZone{

		AllocatorZone() = delete;

		explicit AllocatorZone(Memory::Allocator* allocator) {
			PushAllocator(allocator);
		}

		~AllocatorZone() {
			PopAllocator();
		}
	};

#define STRING_JOIN2(arg1, arg2) DO_STRING_JOIN2(arg1, arg2)
#define DO_STRING_JOIN2(arg1, arg2) arg1 ## arg2
#define SOUL_MEMORY_ALLOCATOR_ZONE(allocator) \
                Soul::Runtime::AllocatorZone STRING_JOIN2(allocatorZone, __LINE__)(allocator)
}
