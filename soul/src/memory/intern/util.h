#pragma once

namespace Soul { namespace Memory {

	namespace Util
	{
		inline static void* AlignForward(void* address, uint32 alignment)
		{
			return (void*)((uintptr(address) + alignment) & ~(alignment - 1));
		}

		inline static void* Add(void* address, uint32 size)
		{
			return (void*)(uintptr(address) + size);
		}
	}

}}