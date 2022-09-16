#include "memory/allocators/malloc_allocator.h"

namespace soul::memory {

	MallocAllocator::MallocAllocator(const char* name) noexcept : Allocator(name) {}

	void MallocAllocator::reset()
	{
		SOUL_NOT_IMPLEMENTED();
	}

	Allocation MallocAllocator::try_allocate(soul_size size, soul_size alignment, const char* tag) {
		void* addr = malloc(size);
		return {addr, size};
	}

	soul_size MallocAllocator::get_allocation_size(void* addr) const
	{
		if (addr == nullptr) return 0;
		return _msize(addr);
	}

	void MallocAllocator::deallocate(void *addr) {
		free(addr);
	}

}
