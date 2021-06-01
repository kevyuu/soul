#include <windows.h>

#include "memory/allocators/page_allocator.h"
#include "memory/util.h"

namespace Soul::Memory {

	PageAllocator::PageAllocator(const char* name) : Allocator(name) {
		SYSTEM_INFO sSysInfo;
		GetSystemInfo(&sSysInfo);
		_pageSize = sSysInfo.dwPageSize;
	}

	void PageAllocator::reset() {};

	Allocation PageAllocator::tryAllocate(soul_size size, soul_size alignment, const char* tag) {
		soul_size newSize = Util::PageSizeRound(size, _pageSize);
		void* addr = VirtualAlloc(NULL, newSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
		SOUL_ASSERT(0, addr != nullptr, "");
		return Allocation(addr, newSize);
	}

	void PageAllocator::deallocate(void* addr, soul_size size) {
		SOUL_ASSERT(0, Util::PageSizeRound(size, _pageSize) == size, "");
		bool isSuccess = VirtualFree(addr, 0, MEM_RELEASE);
		SOUL_ASSERT(0, isSuccess, "");
	}

}