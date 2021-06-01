#pragma once

#include "memory/allocator.h"

namespace Soul::Memory {

	class LinearAllocator final : public Allocator {
	public:
		LinearAllocator() = delete;
		LinearAllocator(const char* name, uint32 size, Allocator* backingAllocator);
		LinearAllocator(const LinearAllocator& other) = delete;
		LinearAllocator& operator=(const LinearAllocator& other) = delete;
		LinearAllocator(LinearAllocator&& other) = delete;
		LinearAllocator& operator=(LinearAllocator&& other) = delete;
		~LinearAllocator() override;

		void reset() final;
		Allocation tryAllocate(soul_size size, soul_size alignment, const char* tag) override;
		void deallocate(void* addr, soul_size size) override;
		[[nodiscard]] void* getMarker() const noexcept;
		void rewind(void* addr) noexcept;

	private:
		Allocator* _backingAllocator;
		void* _baseAddr = nullptr;
		void* _currentAddr = nullptr;
		uint64 _size = 0;
	};

}
