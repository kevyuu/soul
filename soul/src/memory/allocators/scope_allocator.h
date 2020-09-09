#pragma once

// TODO(kevinyu) : Move this to runtime namespace

#include "memory/allocator.h"
#include "runtime/runtime.h"

#include "core/array.h"

namespace Soul {namespace Memory {

	template <typename BACKING_ALLOCATOR = Runtime::TempAllocator>
	class ScopeAllocator: public Allocator {

	public:
		ScopeAllocator() = delete;
		ScopeAllocator(const char* name, BACKING_ALLOCATOR* backingAllocator = Runtime::GetTempAllocator(), Allocator* fallbackAllocator = (Allocator*) Runtime::GetContextAllocator());
		virtual ~ScopeAllocator();
		ScopeAllocator(const ScopeAllocator& other) = delete;
		ScopeAllocator& operator=(const ScopeAllocator& other) = delete;
		ScopeAllocator(ScopeAllocator&& other) = delete;
		ScopeAllocator& operator=(ScopeAllocator&& other) = delete;

		void reset() final;
		Allocation allocate(uint32 size, uint32 alignment, const char* tag) final;
		void deallocate(void* addr, uint32 size) final;
		 
	private:
		BACKING_ALLOCATOR* _backingAllocator;
		Allocator* _fallbackAllocator;
		void* _scopeBaseAddr;
		Array<Allocation> _fallbackAllocations;

		void* getMarker();
		void rewind(void* addr);

	};

	template<typename BACKING_ALLOCATOR>
	ScopeAllocator<BACKING_ALLOCATOR>::ScopeAllocator(const char* name, BACKING_ALLOCATOR* backingAllocator, Allocator* fallbackAllocator) :
		Allocator(name), _backingAllocator(backingAllocator), _fallbackAllocator(fallbackAllocator), _fallbackAllocations(fallbackAllocator) {
		_scopeBaseAddr = getMarker();
	}

	template<typename BACKING_ALLOCATOR>
	ScopeAllocator<BACKING_ALLOCATOR>::~ScopeAllocator() {
		rewind(_scopeBaseAddr);
		for (Allocation allocation : _fallbackAllocations) {
			_fallbackAllocator->deallocate(allocation.addr, allocation.size);
		}
	}

	template<typename BACKING_ALLOCATOR>
	void ScopeAllocator<BACKING_ALLOCATOR>::reset() {
		rewind(_scopeBaseAddr);
	}

	template<typename BACKING_ALLOCATOR>
	Allocation ScopeAllocator<BACKING_ALLOCATOR>::allocate(uint32 size, uint32 alignment, const char* tag) {
		Allocation allocation = _backingAllocator->allocate(size, alignment, tag);
		if (allocation.addr == nullptr) {
			allocation = _fallbackAllocator->allocate(size, alignment, tag);
			_fallbackAllocations.add(allocation);
		}
		return allocation;
	}

	template<typename BACKING_ALLOCATOR>
	void ScopeAllocator<BACKING_ALLOCATOR>::deallocate(void* addr, uint32 size) {}

}}