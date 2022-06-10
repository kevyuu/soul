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
		explicit ScopeAllocator(const char* name, BackingAllocator* backing_allocator = runtime::get_temp_allocator(), Allocator* fallbackAllocator = (Allocator*) runtime::get_context_allocator()) noexcept;
		ScopeAllocator(const ScopeAllocator& other) = delete;
		ScopeAllocator& operator=(const ScopeAllocator& other) = delete;
		ScopeAllocator(ScopeAllocator&& other) = delete;
		ScopeAllocator& operator=(ScopeAllocator&& other) = delete;
		~ScopeAllocator() override;

		void reset() override;
		memory::Allocation try_allocate(soul_size size, soul_size alignment, const char* tag) override;
		soul_size get_allocation_size(void* addr) const override;
		void deallocate(void* addr) override;

	private:
		BackingAllocator* backing_allocator_ = nullptr;
		void* scope_base_addr_ = nullptr;
		Allocator* fallback_allocator_ = nullptr;
		Vector<memory::Allocation> fallback_allocations_;

		[[nodiscard]] void* get_marker() const noexcept;
		void rewind(void* addr) noexcept;

	};

	template<typename BackingAllocator>
	ScopeAllocator<BackingAllocator>::ScopeAllocator(const char* name, BackingAllocator* backing_allocator, Allocator* fallbackAllocator) noexcept :
		Allocator(name), backing_allocator_(backing_allocator), scope_base_addr_(backing_allocator_->get_marker()),
		fallback_allocator_(fallbackAllocator), fallback_allocations_(fallbackAllocator) {}

	template<typename BackingAllocator>
	ScopeAllocator<BackingAllocator>::~ScopeAllocator() {
		rewind(scope_base_addr_);
		for (const auto [addr, size] : fallback_allocations_) {
			fallback_allocator_->deallocate(addr);
		}
	}

	template<typename BackingAllocator>
	void ScopeAllocator<BackingAllocator>::reset()
	{
		rewind(scope_base_addr_);
	}

	template<typename BackingAllocator>
	memory::Allocation ScopeAllocator<BackingAllocator>::try_allocate(soul_size size, soul_size alignment, const char* tag) {
		memory::Allocation allocation = backing_allocator_->try_allocate(size, alignment, tag);
		if (allocation.addr == nullptr) {
			allocation = fallback_allocator_->try_allocate(size, alignment, tag);
			fallback_allocations_.push_back(allocation);
		}
		return allocation;
	}

	template <typename BackingAllocator>
	soul_size ScopeAllocator<BackingAllocator>::get_allocation_size(void* addr) const
	{
		return backing_allocator_->get_allocation_size(addr);
	}

	template<typename BackingAllocator>
	void ScopeAllocator<BackingAllocator>::deallocate(void* addr) {}

	template<typename BackingAllocator>
	void* ScopeAllocator<BackingAllocator>::get_marker() const noexcept
	{
		return backing_allocator_->getMarker();
	}

	template<typename BackingAllocator>
	void ScopeAllocator<BackingAllocator>::rewind(void* addr) noexcept
	{
		backing_allocator_->rewind(addr);
	}

}
