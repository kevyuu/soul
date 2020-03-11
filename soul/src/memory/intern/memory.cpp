#include "memory/memory.h"
#include "core/array.h"

namespace Soul { namespace Memory {
	static thread_local Array<Allocator*> _gAllocatorStack(nullptr);
	static thread_local TempAllocator* _gTempAllocator;
	static Allocator* _gDefaultAllocator = nullptr;

	void Init(Allocator* defaultAllocator) {
		_gDefaultAllocator = defaultAllocator;
		_gAllocatorStack.init(_gDefaultAllocator);
	}

	void SetTempAllocator(TempAllocator* tempAllocator) {
		_gTempAllocator = tempAllocator;
	}

	void PushAllocator(Allocator* allocator) {
		SOUL_ASSERT(0, _gDefaultAllocator != nullptr, "");
		_gAllocatorStack.add(allocator);
	}

	void PopAllocator() {
		SOUL_ASSERT(0, !_gAllocatorStack.empty(), "");
		_gAllocatorStack.pop();
	}

	Allocator* GetContextAllocator() {
		if (_gAllocatorStack.empty()) return _gDefaultAllocator;
		return _gAllocatorStack.back();
	}

	void* Allocate(uint32 size, uint32 alignment) {
		return GetContextAllocator()->allocate(size, alignment);
	}

	void Deallocate(void* addr, uint32 size) {
		return GetContextAllocator()->deallocate(addr, size);
	}

	TempAllocator* GetTempAllocator() {
		SOUL_ASSERT(0, _gTempAllocator != nullptr, "");
		return _gTempAllocator;
	}

}}