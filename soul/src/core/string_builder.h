#include "core/type.h"
#include "memory/allocator.h"

namespace Soul {
	class StringBuilder {

	public:
		StringBuilder(Memory::Allocator* allocator, uint64 initialCapacity);

		~StringBuilder();

		void reserve(uint64 newCapacity);

		StringBuilder& append(const char* format, ...);

		uint64 capacity() { return _capacity; }

		uint64 size() { return _size; }

		char* data() { return _data; }

	private:
		Memory::Allocator* _allocator;
		uint64 _capacity;
		uint64 _size; // string size, not counting NULL
		char* _data;
	};
}
