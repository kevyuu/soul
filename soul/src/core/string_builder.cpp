#include "string_builder.h"
#include <stdio.h>
#include <stdarg.h>

namespace Soul{
	StringBuilder::StringBuilder(Memory::Allocator* allocator, uint64 initialCapacity) :
		_allocator(allocator), _capacity(0),
		_size(0), _data(nullptr)
	{
		reserve(initialCapacity);
	}

	StringBuilder::~StringBuilder() {
		if (_data != nullptr) {
			SOUL_ASSERT(0, _capacity != 0, "");
			_allocator->deallocate(_data, _capacity);
		}
	}

	void StringBuilder::reserve(uint64 newCapacity) {
		char* newData = (char*)_allocator->allocate(newCapacity, alignof(char));
		if (_data != nullptr) {
			memcpy(newData, _data, _capacity * sizeof(char));
			_allocator->deallocate(_data, _capacity);
		}
		_data = newData;
		_capacity = newCapacity;
	}

	StringBuilder& StringBuilder::append(const char* format, ...) {
		va_list args;

		va_start(args, format);
		uint64 needed = vsnprintf(nullptr, 0, format, args);
		va_end(args);

		if (needed + 1 > _capacity - _size) {
			uint64 newCapacity = _capacity >= (needed + 1) ? 2 * _capacity : _capacity + (needed + 1);
			reserve(newCapacity);
		}

		va_start(args, format);
		vsnprintf(_data + _size, (needed + 1), format, args);
		va_end(args);

		_size += needed;
		_data[_size] = '\0';

		return *this;
	}
}