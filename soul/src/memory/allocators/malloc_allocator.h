#pragma once

#include "memory/allocator.h"

namespace soul::memory {

	class MallocAllocator final : public Allocator
	{
	public:

		MallocAllocator() = delete;
		MallocAllocator(const char* name) noexcept;
		MallocAllocator(const MallocAllocator& other) = delete;
		MallocAllocator& operator=(const MallocAllocator& other) = delete;
		MallocAllocator(MallocAllocator&& other) = delete;
		MallocAllocator& operator=(MallocAllocator&& other) = delete;
		~MallocAllocator() override = default;

		void reset() override;
		Allocation try_allocate(soul_size size, soul_size alignment, const char* tag) override;
		[[nodiscard]] soul_size get_allocation_size(void* addr) const override;
		void deallocate(void* addr) override;

	};

}

