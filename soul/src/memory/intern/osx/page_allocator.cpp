#include <sys/mman.h>
#include <unistd.h>
#include <errno.h>

#include "memory/allocators/page_allocator.h"
#include "memory/util.h"

namespace Soul::Memory {

	PageAllocator::PageAllocator(const char* name) : Allocator(name) {
		_pageSize = getpagesize();
	}

	void PageAllocator::reset() {};

	Allocation PageAllocator::tryAllocate(uint32 size, uint32 alignment, const char* tag) {
		uint32 newSize = Util::PageSizeRound(size, _pageSize);
		void* addr = mmap(nullptr, newSize, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);
		return {addr, newSize};
	}

	void PageAllocator::deallocate(void* addr, uint32 size) {
		SOUL_ASSERT(0, Util::PageSizeRound(size, _pageSize) == size, "");
		munmap(addr, size);
	}

}