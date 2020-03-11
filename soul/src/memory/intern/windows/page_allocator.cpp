#include "memory/allocators/page_allocator.h"
#include "memory/util.h"
#include <windows.h>

namespace Soul { namespace Memory {

	PageAllocator::PageAllocator(const char* name) : Allocator(name) {
		SYSTEM_INFO sSysInfo;
		GetSystemInfo(&sSysInfo);
		_pageSize = sSysInfo.dwPageSize;
	}

	void PageAllocator::reset() {};

	Allocation PageAllocator::allocate(uint32 size, uint32 alignment, const char* tag) {
		uint32 newSize = Util::PageSizeRound(size, _pageSize);
		void* addr = VirtualAlloc(NULL, newSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
		SOUL_ASSERT(0, addr != nullptr, "");
		return {addr, newSize};
	}

	void PageAllocator::deallocate(void* addr, uint32 size) {
		SOUL_ASSERT(0, Util::PageSizeRound(size, _pageSize) == size, "");
		bool isSuccess = VirtualFree(addr, size, MEM_RELEASE);
		SOUL_ASSERT(0, isSuccess, "");
	}

}}