#pragma once

#include "core/type.h"

namespace Soul {

	class HashMap {
	public:
		
		HashMap();

		void init(uint32 capacity);
		uint32 get(uint32 key);
		void add(uint32 key, uint32 value);
		void cleanup();

		constexpr static uint32 VALUE_EMPTY = 0;

	private:

		uint32 capacity;
		uint32* keys;
		uint32* values;

	};
}