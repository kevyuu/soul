#include "memory/allocators/scope_allocator.h"
#include "memory/allocators/proxy_allocator.h"
#include "memory/allocators/linear_allocator.h"

namespace Soul {namespace Memory {

	template<>
	void* ScopeAllocator<ProxyAllocator<LinearAllocator>>::getMarker(){
		return _backingAllocator->allocator->getMarker();
	}

	template<>
	void ScopeAllocator<ProxyAllocator<LinearAllocator>>::rewind(void* addr) {
		_backingAllocator->allocator->rewind(addr);
	}

	template<>
	void* ScopeAllocator<LinearAllocator>::getMarker() {
		return _backingAllocator->getMarker();
	}

	template<>
	void ScopeAllocator<LinearAllocator>::rewind(void* addr) {
		_backingAllocator->rewind(addr);
	}
}}