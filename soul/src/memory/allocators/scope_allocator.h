#pragma once

// TODO(kevinyu) : Move this to runtime namespace, Use double ended stack allocator

#include "core/array.h"

#include "memory/allocator.h"
#include "runtime/runtime.h"

namespace Soul::Memory{

	template <typename BACKING_ALLOCATOR = Runtime::TempAllocator>
	class ScopeAllocator final: public Allocator {

	public:
		ScopeAllocator() = delete;
		explicit ScopeAllocator(const char* name, BACKING_ALLOCATOR* backingAllocator = Runtime::GetTempAllocator(), Allocator* fallbackAllocator = (Allocator*) Runtime::GetContextAllocator()) noexcept;
		ScopeAllocator(const ScopeAllocator& other) = delete;
		ScopeAllocator& operator=(const ScopeAllocator& other) = delete;
		ScopeAllocator(ScopeAllocator&& other) = delete;
		ScopeAllocator& operator=(ScopeAllocator&& other) = delete;
		~ScopeAllocator() override;

		void reset() override;
		Allocation tryAllocate(soul_size size, soul_size alignment, const char* tag) override;
		void deallocate(void* addr, soul_size size) override;

	private:
		BACKING_ALLOCATOR* _backingAllocator = nullptr;
		void* _scopeBaseAddr = nullptr;
		Allocator* _fallbackAllocator = nullptr;
		Array<Allocation> _fallbackAllocations;

		void* getMarker();
		void rewind(void* addr);

	};

	template<typename BACKING_ALLOCATOR>
	ScopeAllocator<BACKING_ALLOCATOR>::ScopeAllocator(const char* name, BACKING_ALLOCATOR* backingAllocator, Allocator* fallbackAllocator) noexcept :
		Allocator(name), _backingAllocator(backingAllocator),
		_fallbackAllocator(fallbackAllocator), _fallbackAllocations(fallbackAllocator)
	{
		_scopeBaseAddr = getMarker();
	}

	template<typename BACKING_ALLOCATOR>
	ScopeAllocator<BACKING_ALLOCATOR>::~ScopeAllocator() {
		rewind(_scopeBaseAddr);
		for (const auto [addr, size] : _fallbackAllocations) {
			_fallbackAllocator->deallocate(addr, size);
		}
	}

	template<typename BACKING_ALLOCATOR>
	void ScopeAllocator<BACKING_ALLOCATOR>::reset()
	{
		rewind(_scopeBaseAddr);
	}

	template<typename BACKING_ALLOCATOR>
	Allocation ScopeAllocator<BACKING_ALLOCATOR>::tryAllocate(soul_size size, soul_size alignment, const char* tag) {
		Allocation allocation = _backingAllocator->tryAllocate(size, alignment, tag);
		if (allocation.addr == nullptr) {
			allocation = _fallbackAllocator->tryAllocate(size, alignment, tag);
			_fallbackAllocations.add(allocation);
		}
		return allocation;
	}

	template<typename BACKING_ALLOCATOR>
	void ScopeAllocator<BACKING_ALLOCATOR>::deallocate(void* addr, soul_size size) {}

}
