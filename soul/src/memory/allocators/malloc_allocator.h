#pragma once

#include "memory/allocator.h"

namespace Soul { namespace Memory {
	class MallocAllocator: public Allocator
	{
	public:

		MallocAllocator() = delete;
		MallocAllocator(const char* name);
		virtual ~MallocAllocator() = default;

		MallocAllocator(const MallocAllocator& other) = delete;
		MallocAllocator& operator=(const MallocAllocator& other) = delete;

		MallocAllocator(MallocAllocator&& other) = delete;
		MallocAllocator& operator=(MallocAllocator&& other) = delete;

		virtual void reset() final override;
		virtual Allocation allocate(uint32 size, uint32 alignment, const char* tag) final override;
		virtual void deallocate(void* addr, uint32 size) final override;

	};
}}

