#pragma once

#include "memory/allocator.h"
#include "memory/allocators/linear_allocator.h"
#include "memory/allocators/proxy_allocator.h"
#include "memory/allocators/malloc_allocator.h"

#define ONE_KILOBYTE 1024
#define ONE_MEGABYTE 1024 * ONE_KILOBYTE
#define ONE_GIGABYTE 1024 * ONE_MEGABYTE

namespace Soul { namespace Memory {

	using TempProxy = NoOpProxy;
	using TempAllocator = ProxyAllocator<LinearAllocator, TempProxy>;

	using DefaultAllocatorProxy = MultiProxy<CounterProxy, ClearValuesProxy, BoundGuardProxy, NoOpProxy, NoOpProxy>;
	using DefaultAllocator = ProxyAllocator<MallocAllocator, DefaultAllocatorProxy>;

	void Init(Allocator* defaultAllocator);
	void SetTempAllocator(TempAllocator* tempAllocator);

	void PushAllocator(Allocator* allocator);
	void PopAllocator();
	Allocator* GetContextAllocator();
	TempAllocator* GetTempAllocator();

	void* Allocate(uint32 size, uint32 alignment);
	void Deallocate(void* addr, uint32 size);

	struct AllocatorInitializer {
		AllocatorInitializer() = delete;
		explicit AllocatorInitializer(Allocator* allocator) {
			PushAllocator(allocator);
		}

		void end() {
			PopAllocator();
		}
	};

	struct AllocatorZone{

		AllocatorZone() = delete;

		explicit AllocatorZone(Allocator* allocator) {
			PushAllocator(allocator);
		}

		~AllocatorZone() {
			PopAllocator();
		}
	};

	#define STRING_JOIN2(arg1, arg2) DO_STRING_JOIN2(arg1, arg2)
	#define DO_STRING_JOIN2(arg1, arg2) arg1 ## arg2
	#define SOUL_MEMORY_ALLOCATOR_ZONE(allocator) \
		Soul::Memory::AllocatorZone STRING_JOIN2(allocatorZone, __LINE__)(allocator)
}}