#pragma once

#include "memory/allocator.h"

namespace soul
{
	namespace impl
	{
		memory::Allocator* get_default_allocator();
	}
#ifndef USE_CUSTOM_DEFAULT_ALLOCATOR
	inline memory::Allocator* get_default_allocator()
	{
		return impl::get_default_allocator();
	}
#else
    memory::Allocator* get_default_allocator();
#endif

}