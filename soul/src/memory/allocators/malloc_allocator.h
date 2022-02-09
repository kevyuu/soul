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
		Allocation tryAllocate(soul_size size, soul_size alignment, const char* tag) override;
		void deallocate(void* addr, soul_size size) override;

	};

}

