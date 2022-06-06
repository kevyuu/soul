#pragma once

// TODO(kevinyu) : Move this to runtime namespace, Use double ended stack allocator

#include "core/vector.h"

#include "memory/allocator.h"
#include "runtime/runtime.h"

namespace soul::runtime{

	template <typename BackingAllocator = TempAllocator>
	class ScopeAllocator final: public memory::Allocator {

	public:
		ScopeAllocator() = delete;
		explicit ScopeAllocator(const char* name, BackingAllocator* backingAllocator = runtime::get_temp_allocator(), Allocator* fallbackAllocator = (Allocator*) runtime::get_context_allocator()) noexcept;
		ScopeAllocator(const ScopeAllocator& other) = delete;
		ScopeAllocator& operator=(const ScopeAllocator& other) = delete;
		ScopeAllocator(ScopeAllocator&& other) = delete;
		ScopeAllocator& operator=(ScopeAllocator&& other) = delete;
		~ScopeAllocator() override;

		void reset() override;
		memory::Allocation try_allocate(soul_size size, soul_size alignment, const char* tag) override;
		void deallocate(void* addr, soul_size size) override;

	private:
		BackingAllocator* _backingAllocator = nullptr;
		void* _scopeBaseAddr = nullptr;
		Allocator* _fallbackAllocator = nullptr;
		Vector<memory::Allocation> _fallbackAllocations;

		SOUL_NODISCARD void* getMarker() const noexcept;
		void rewind(void* addr) noexcept;

	};

	template<typename BackingAllocator>
	ScopeAllocator<BackingAllocator>::ScopeAllocator(const char* name, BackingAllocator* backingAllocator, Allocator* fallbackAllocator) noexcept :
		Allocator(name), _backingAllocator(backingAllocator),
		_fallbackAllocator(fallbackAllocator), _fallbackAllocations(fallbackAllocator)
	{
		_scopeBaseAddr = getMarker();
	}

	template<typename BackingAllocator>
	ScopeAllocator<BackingAllocator>::~ScopeAllocator() {
		rewind(_scopeBaseAddr);
		for (const auto [addr, size] : _fallbackAllocations) {
			_fallbackAllocator->deallocate(addr, size);
		}
	}

	template<typename BackingAllocator>
	void ScopeAllocator<BackingAllocator>::reset()
	{
		rewind(_scopeBaseAddr);
	}

	template<typename BACKING_ALLOCATOR>
	memory::Allocation ScopeAllocator<BACKING_ALLOCATOR>::try_allocate(soul_size size, soul_size alignment, const char* tag) {
		memory::Allocation allocation = _backingAllocator->try_allocate(size, alignment, tag);
		if (allocation.addr == nullptr) {
			allocation = _fallbackAllocator->try_allocate(size, alignment, tag);
			_fallbackAllocations.push_back(allocation);
		}
		return allocation;
	}

	template<typename BackingAllocator>
	void ScopeAllocator<BackingAllocator>::deallocate(void* addr, soul_size size) {}

	template<typename BackingAllocator>
	void* ScopeAllocator<BackingAllocator>::getMarker() const noexcept
	{
		return _backingAllocator->getMarker();
	}

	template<typename BackingAllocator>
	void ScopeAllocator<BackingAllocator>::rewind(void* addr) noexcept
	{
		_backingAllocator->rewind(addr);
	}

}
