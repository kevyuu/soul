#pragma once

#include "memory/allocator.h"

namespace soul::memory {

	class LinearAllocator final : public Allocator {
	public:
		LinearAllocator() = delete;
		LinearAllocator(const char* name, const soul_size size, Allocator* backing_allocator);
		LinearAllocator(const LinearAllocator& other) = delete;
		LinearAllocator& operator=(const LinearAllocator& other) = delete;
		LinearAllocator(LinearAllocator&& other) = delete;
		LinearAllocator& operator=(LinearAllocator&& other) = delete;
		~LinearAllocator() override;

		void reset() final;
		Allocation try_allocate(soul_size size, soul_size alignment, const char* tag) override;
		soul_size get_allocation_size(void* addr) const override;
		void deallocate(void* addr) override;
		[[nodiscard]] void* get_marker() const noexcept;
		void rewind(void* addr) noexcept;

	private:
		Allocator* backing_allocator_;
		void* base_addr_ = nullptr;
		void* current_addr_ = nullptr;
		uint64 size_ = 0;
	};

}
