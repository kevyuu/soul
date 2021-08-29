#pragma once

// TODO(kevinyu) : Move this to runtime namespace, Use double ended stack allocator

#include "core/array.h"

#include "memory/allocator.h"
#include "runtime/runtime.h"

namespace Soul::Runtime{

	template <typename BackingAllocator = TempAllocator>
	class ScopeAllocator final: public Memory::Allocator {

	public:
		ScopeAllocator() = delete;
		explicit ScopeAllocator(const char* name, BackingAllocator* backingAllocator = Runtime::GetTempAllocator(), Allocator* fallbackAllocator = (Allocator*) Runtime::GetContextAllocator()) noexcept;
		ScopeAllocator(const ScopeAllocator& other) = delete;
		ScopeAllocator& operator=(const ScopeAllocator& other) = delete;
		ScopeAllocator(ScopeAllocator&& other) = delete;
		ScopeAllocator& operator=(ScopeAllocator&& other) = delete;
		~ScopeAllocator() override;

		void reset() override;
		Memory::Allocation tryAllocate(soul_size size, soul_size alignment, const char* tag) override;
		void deallocate(void* addr, soul_size size) override;

	private:
		BackingAllocator* _backingAllocator = nullptr;
		void* _scopeBaseAddr = nullptr;
		Allocator* _fallbackAllocator = nullptr;
		Array<Memory::Allocation> _fallbackAllocations;

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
	Memory::Allocation ScopeAllocator<BACKING_ALLOCATOR>::tryAllocate(soul_size size, soul_size alignment, const char* tag) {
		Memory::Allocation allocation = _backingAllocator->tryAllocate(size, alignment, tag);
		if (allocation.addr == nullptr) {
			allocation = _fallbackAllocator->tryAllocate(size, alignment, tag);
			_fallbackAllocations.add(allocation);
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
