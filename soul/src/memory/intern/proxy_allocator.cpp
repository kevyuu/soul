#include "memory/allocator.h"


namespace Soul {namespace Memory {

	template<typename BACKING_ALLOCATOR,
	        typename THREADING_PROXY,
	        typename TRACKING_PROXY,
	        typename TAGGING_PROXY>
	void ProxyAllocator<BACKING_ALLOCATOR, THREADING_PROXY, TRACKING_PROXY, TAGGING_PROXY>::init() {
		allocator.init();
	}

	template<typename BACKING_ALLOCATOR,
			typename THREADING_PROXY,
			typename TRACKING_PROXY,
			typename TAGGING_PROXY>
	void ProxyAllocator<BACKING_ALLOCATOR, THREADING_PROXY, TRACKING_PROXY, TAGGING_PROXY>::cleanup() {
		allocator.cleanup();
	}

	template<typename BACKING_ALLOCATOR,
			typename THREADING_PROXY,
			typename TRACKING_PROXY,
			typename TAGGING_PROXY>
	void* ProxyAllocator<BACKING_ALLOCATOR, THREADING_PROXY, TRACKING_PROXY, TAGGING_PROXY>::allocate(uint32 size, uint32 alignment, const char* tag) {
		_threadingProxy.lock();

		void* address = allocator.allocate(size, alignment, tag);
		_trackingProxy.onAllocate(address, size);
		_taggingProxy.onAllocate(address, size);
		_threadingProxy.unlock();
		return address;
	}

	template<typename BACKING_ALLOCATOR,
			typename THREADING_PROXY,
			typename TRACKING_PROXY,
			typename TAGGING_PROXY>
	void ProxyAllocator<BACKING_ALLOCATOR, THREADING_PROXY, TRACKING_PROXY, TAGGING_PROXY>::deallocate(void* addr) {
		_threadingProxy.lock();
		_trackingProxy.onDeallocate(addr);
		_taggingProxy.onDeallocate(addr);
		allocator.deallocate(addr);
		_threadingProxy.unlock();
	}

}}