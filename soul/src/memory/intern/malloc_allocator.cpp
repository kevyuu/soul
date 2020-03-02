#include "memory/allocators/malloc_allocator.h"

namespace Soul { namespace Memory {

	MallocAllocator::MallocAllocator(const char* name): Allocator(name) {}

	void MallocAllocator::reset() {}

	Allocation MallocAllocator::allocate(uint32 size, uint32 alignment, const char* tag) {
		return {malloc(size), size};
	}

	void MallocAllocator::deallocate(void *addr, uint32 size) {
		free(addr);
	}

}}