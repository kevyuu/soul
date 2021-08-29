#include "core/config.h"
#include "runtime/runtime.h"

namespace Soul {
	Memory::Allocator* GetDefaultAllocator()
	{
		return Runtime::GetContextAllocator();
	}
}