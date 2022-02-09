#include "memory/allocators/linear_allocator.h"
#include "memory/util.h"

namespace soul::memory {

	LinearAllocator::LinearAllocator(const char* name, uint32 size, Allocator* backingAllocator): Allocator(name), _backingAllocator(backingAllocator) {
		SOUL_ASSERT(0, _backingAllocator != nullptr, "");
		const Allocation allocation = _backingAllocator->tryAllocate(size, 0, name);
		_baseAddr = allocation.addr;
		_currentAddr = _baseAddr;
		_size = allocation.size;
	}

	LinearAllocator::~LinearAllocator() {
		_backingAllocator->deallocate(_baseAddr, _size);
	}

	void LinearAllocator::reset() { _currentAddr = _baseAddr; }

	Allocation LinearAllocator::tryAllocate(soul_size size, soul_size alignment, const char* tag) {
		void *addr = Util::AlignForward(_currentAddr, alignment);
		if (Util::Add(_baseAddr, _size) < Util::Add(addr, size)) {
			return {nullptr, 0};
		}
		_currentAddr = Util::Add(addr, size);
		return Allocation(addr, size);
	}

	void LinearAllocator::deallocate(void* addr, soul_size size) {}

	void* LinearAllocator::getMarker() const noexcept {return _currentAddr;}
	void LinearAllocator::rewind(void* addr) noexcept
	{
		SOUL_ASSERT(0, addr >= _baseAddr && addr <= _currentAddr, "");
		_currentAddr = addr;
	}

}
