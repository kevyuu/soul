#include "core/config.h"
#include "runtime/runtime.h"

namespace soul::impl {
	memory::Allocator* get_default_allocator()
	{
		return runtime::get_context_allocator();
	}
}