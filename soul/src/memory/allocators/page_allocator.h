#pragma once

#include "memory/allocator.h"

namespace soul::memory {

	class PageAllocator final : public Allocator {
	public:
		PageAllocator() = delete;
		explicit PageAllocator(const char* name);
		PageAllocator(const PageAllocator& other) = delete;
		PageAllocator& operator=(const PageAllocator& other) = delete;
		PageAllocator(PageAllocator&& other) = delete;
		PageAllocator& operator=(PageAllocator&& other) = delete;
		~PageAllocator() override = default;

		void reset() override;
		Allocation try_allocate(soul_size size, soul_size alignment, const char* tag) override;
		void deallocate(void* addr, soul_size size) override;
	private:
		soul_size _pageSize = 0;
	};

}
