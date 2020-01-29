#include "memory/allocator.h"

namespace Soul { namespace Memory {

	MallocAllocator::MallocAllocator(const char* name): Allocator(name) {}

	void MallocAllocator::init() {}
	void MallocAllocator::cleanup() {}

	void* MallocAllocator::allocate(uint32 size, uint32 alignment, const char* tag) {
		return malloc(size);
	}

	void MallocAllocator::deallocate(void *addr) {
		free(addr);
	}

}}