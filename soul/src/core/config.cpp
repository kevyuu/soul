#include "core/config.h"
#include "runtime/runtime.h"

namespace soul {
	memory::Allocator* GetDefaultAllocator()
	{
		return runtime::get_context_allocator();
	}
}