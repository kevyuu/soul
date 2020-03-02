#pragma once
#include "memory/allocator.h"

namespace Soul { namespace Memory {
	class LinearAllocator: public Allocator {
	public:

		LinearAllocator() = delete;
		LinearAllocator(const char* name, uint32 size, Allocator* backingAllocator);
		virtual ~LinearAllocator();
		LinearAllocator(const LinearAllocator& other) = delete;
		LinearAllocator& operator=(const LinearAllocator& other) = delete;
		LinearAllocator(LinearAllocator&& other) = delete;
		LinearAllocator& operator=(LinearAllocator&& other) = delete;

		void reset() final;
		Allocation allocate(uint32 size, uint32 alignment, const char* tag) final;
		void deallocate(void* addr, uint32 size) final;
		void* getMarker();
		void rewind(void* addr);

	private:
		Allocator* _backingAllocator;
		void* _baseAddr;
		void* _currentAddr;
		uint64 _size;
	};
}}