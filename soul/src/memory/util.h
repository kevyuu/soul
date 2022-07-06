#pragma once
#include "core/type.h"
namespace soul::memory::util
{
	[[nodiscard]] static soul_size pointer_page_size_round(const soul_size size, const soul_size page_size)
	{
		SOUL_ASSERT(0, page_size != 0, "");
		return ((size + page_size) & ~(page_size - 1));
	}

	[[nodiscard]] static void* pointer_add(const void* address, const soul_size size)
	{
		return soul::cast<byte*>(address) + size;
	}

	[[nodiscard]] static void* pointer_align_forward(const void* address, const soul_size alignment)
	{
		return (void*)((reinterpret_cast<uintptr>(address) + alignment) & ~(alignment - 1));  // NOLINT(performance-no-int-to-ptr)
	}

	[[nodiscard]] static void* pointer_align_backward(const void* address, const soul_size alignment)
	{
		return (void*)(reinterpret_cast<uintptr>(address) & ~(alignment - 1));  // NOLINT(performance-no-int-to-ptr)
	}
	
	[[nodiscard]] static void* pointer_sub(const void* address, const soul_size size)
	{
		return soul::cast<byte*>(address) - size;
	}
}
