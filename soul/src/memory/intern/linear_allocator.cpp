#include "memory/allocators/linear_allocator.h"
#include "memory/util.h"

namespace Soul {namespace Memory {

	LinearAllocator::LinearAllocator(const char* name, uint32 size, Allocator* backingAllocator): Allocator(name), _backingAllocator(backingAllocator) {
		SOUL_ASSERT(0, _backingAllocator != nullptr, "");
		Allocation allocation = _backingAllocator->allocate(size, 0, getName());
		_baseAddr = allocation.addr;
		_currentAddr = _baseAddr;
		_size = allocation.size;
	}

	LinearAllocator::~LinearAllocator() {
		_backingAllocator->deallocate(_baseAddr, _size);
	}

	void LinearAllocator::reset() { _currentAddr = _baseAddr; }

	Allocation LinearAllocator::allocate(uint32 size, uint32 alignment, const char* tag) {
		void *addr = Util::AlignForward(_currentAddr, alignment);
		if (Util::Add(_baseAddr, _size) < Util::Add(addr, size)) {
			return {nullptr, 0};
		}
		_currentAddr = Util::Add(addr, size);
		return {addr, size};
	}

	void LinearAllocator::deallocate(void* addr, uint32 size) {}

	void* LinearAllocator::getMarker() {return _currentAddr;}
	void LinearAllocator::rewind(void* addr) {
		SOUL_ASSERT(0, addr >= _baseAddr && addr <= _currentAddr, "");
		_currentAddr = addr;
	}

}}