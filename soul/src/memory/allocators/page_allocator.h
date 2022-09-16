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
		[[nodiscard]] soul_size get_allocation_size(void* addr) const override;
		void deallocate(void* addr) override;
		
		
	private:
		soul_size page_size_ = 0;
	};

}
