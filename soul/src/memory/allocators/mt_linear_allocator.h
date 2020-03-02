#pragma once

#include "memory/allocator.h"
#include "core/array.h"

namespace Soul { namespace Memory {

	class MTLinearAllocator: public Allocator {

	public:
		MTLinearAllocator() = delete;
		MTLinearAllocator(const char* name, uint32 size, Allocator* backingAllocator);
		virtual ~MTLinearAllocator();
		MTLinearAllocator(const MTLinearAllocator& other) = delete;
		MTLinearAllocator& operator=(const MTLinearAllocator& other) = delete;
		MTLinearAllocator(MTLinearAllocator&& other) = delete;
		MTLinearAllocator& operator=(MTLinearAllocator&& other) = delete;

		void reset() final;
		Allocation allocate(uint32 size, uint32 alignment, const char* tag) final;
		void deallocate(void* addr, uint32 size) final;

		void* getMarker();
		void rewind(void* addr);

	private:

		struct PerThread {
			void* baseAddr;
			void* currentAddr;
		};
		uint64 _threadCount;
		PerThread* _perThreads;
		uint32 _sizePerThread;
		uint32 _totalSize;
		Allocator* _backingAllocator;

	};
}}