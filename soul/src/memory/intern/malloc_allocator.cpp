#include "memory/allocators/malloc_allocator.h"

namespace soul::memory {

	MallocAllocator::MallocAllocator(const char* name) noexcept : Allocator(name) {}

	void MallocAllocator::reset()
	{
		SOUL_NOT_IMPLEMENTED();
	}

	Allocation MallocAllocator::try_allocate(soul_size size, soul_size alignment, const char* tag) {
		return {malloc(size), size};
	}

	void MallocAllocator::deallocate(void *addr, soul_size size) {
		free(addr);
	}

}
