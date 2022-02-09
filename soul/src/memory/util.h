#pragma once

#include "core/math.h"

namespace soul::memory::Util
{
	inline static uint64 PageSizeRound(uint64 size, uint64 pageSize)
	{
		SOUL_ASSERT(0, pageSize != 0, "");
		SOUL_ASSERT(0, isPowerOfTwo(pageSize), "");
		return ((size + pageSize) & ~(pageSize - 1));
	}

	inline static void* AlignForward(void* address, uint64 alignment)
	{
		return (void*)((uintptr(address) + alignment) & ~(alignment - 1));
	}

	inline static void* Add(void* address, uint64 size)
	{
		return (void*)(uintptr(address) + size);
	}

	inline static void* Sub(void* address, uint64 size)
	{
		return (void*)(uintptr(address) - size);
	}
}
