#include "core/dev_util.h"

#include "memory/allocator.h"
#include "memory/profiler.h"
#include "memory/allocators/proxy_allocator.h"

namespace Soul {
	namespace Memory {

		void ProfileProxy::onPreInit(const char* name) {
			SOUL_MEMPROFILE_REGISTER_ALLOCATOR(name);
			_name = name;
		}

		AllocateParam ProfileProxy::onPreAllocate(const AllocateParam& allocParam) {
			_currentAlloc = allocParam;
			return allocParam;
		}

		Allocation ProfileProxy::onPostAllocate(Allocation allocation) {
			SOUL_MEMPROFILE_REGISTER_ALLOCATION(_name, _currentAlloc.tag, allocation.addr, _currentAlloc.size);
			return allocation;
		}

		DeallocateParam ProfileProxy::onPreDeallocate(const DeallocateParam& deallocParam) {
			if (deallocParam.addr == nullptr) return deallocParam;
			SOUL_MEMPROFILE_REGISTER_DEALLOCATION(_name, deallocParam.addr, deallocParam.size);
			return deallocParam;
		}

		void ProfileProxy::onPreCleanup() {
			SOUL_MEMPROFILE_DEREGISTER_ALLOCATOR(_name);
		}

	}
}