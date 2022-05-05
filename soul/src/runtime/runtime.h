#pragma once

#include "core/type_traits.h"
#include "runtime/data.h"
#include "runtime/system.h"

namespace soul::runtime {

	inline void init(const Config& config) {
		System::Get().init(config);
	}

	inline void shutdown() {
		System::Get().shutdown();
	}

	inline void begin_frame() {
		System::Get().beginFrame();
	}

	inline TaskID create_task(const TaskID parent = TaskID::ROOT()) {
		return System::Get().taskCreate(parent, [](TaskID){});
	}

	template<execution Execute>
	inline TaskID create_task(const TaskID parent, Execute&& lambda) {
		return System::Get().taskCreate(parent, std::forward<Execute>(lambda));
	}

	inline void wait_task(TaskID taskID) {
		System::Get().taskWait(taskID);
	}

	inline void run_task(TaskID taskID) {
		System::Get().taskRun(taskID);
	}

	inline void run_and_wait_task(TaskID task_id)
	{
		run_task(task_id);
		wait_task(task_id);
	}

	template<execution Execute>
	TaskID create_and_run_task(TaskID parent, Execute&& lambda) {
		const TaskID taskID = create_task(parent, std::forward<Execute>(lambda));
		run_task(taskID);
		return taskID;
	}

	template<typename Func> requires is_lambda_v<Func, void(int)>
	inline TaskID parallel_for_task_create(TaskID parent, uint32 count, uint32 blockSize, Func&& func) {
		return System::Get()._parallelForTaskCreateRecursive(parent, 0, count, blockSize, std::forward<Func>(func));
	}

	inline uint16 get_thread_id() {
		return System::Get().getThreadID();
	}

	inline uint16 get_thread_count() {
		return System::Get().getThreadCount();
	}

	inline void push_allocator(memory::Allocator* allocator) {
		System::Get().pushAllocator(allocator);
	}

	inline void pop_allocator() {
		System::Get().popAllocator();
	}

	inline memory::Allocator* get_context_allocator() {
		return System::Get().getContextAllocator();
	}

	inline TempAllocator* get_temp_allocator() {
		return System::Get().getTempAllocator();
	}

	inline void* allocate(uint32 size, uint32 alignment) {
		return System::Get().allocate(size, alignment);
	}

	inline void deallocate(void* addr, uint32 size) {
		return System::Get().deallocate(addr, size);
	}

	struct AllocatorInitializer {
		AllocatorInitializer() = delete;
		explicit AllocatorInitializer(memory::Allocator* allocator) {
			push_allocator(allocator);
		}

		void end() {
			pop_allocator();
		}
	};

	struct AllocatorZone{

		AllocatorZone() = delete;

		explicit AllocatorZone(memory::Allocator* allocator) {
			push_allocator(allocator);
		}

		~AllocatorZone() {
			pop_allocator();
		}
	};

#define STRING_JOIN2(arg1, arg2) DO_STRING_JOIN2(arg1, arg2)
#define DO_STRING_JOIN2(arg1, arg2) arg1 ## arg2
#define SOUL_MEMORY_ALLOCATOR_ZONE(allocator) \
                soul::runtime::AllocatorZone STRING_JOIN2(allocatorZone, __LINE__)(allocator)
}
