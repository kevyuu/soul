#pragma once

#include "memory/allocator.h"

namespace Soul { namespace Memory {
	class PageAllocator: public Allocator {
	public:
		PageAllocator() = delete;
		PageAllocator(const char* name);
		virtual ~PageAllocator() = default;
		PageAllocator(const PageAllocator& other) = delete;
		PageAllocator& operator=(const PageAllocator& other) = delete;
		PageAllocator(PageAllocator&& other) = delete;
		PageAllocator& operator=(PageAllocator&& other) = delete;

		virtual void reset() final override;
		virtual Allocation allocate(uint32 size, uint32 alignment, const char* tag) final override;
		virtual void deallocate(void* addr, uint32 size) final override;
	private:
		uint64 _pageSize;
	};
}}