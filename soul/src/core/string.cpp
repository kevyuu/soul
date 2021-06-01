#include "string.h"
#include "runtime/runtime.h"

#include <algorithm>
#include <stdio.h>
#include <stdarg.h>

namespace Soul {

	String::String(Memory::Allocator* allocator, uint64 initialCapacity) :
		_allocator(allocator), _capacity(0),
		_size(0), _data(nullptr)
	{
		reserve(initialCapacity);
	}

	String::String(Memory::Allocator* allocator, char* str) : String(allocator, strlen(str) + 1) {
		_size = _capacity - 1;
		memcpy(_data, str, _capacity * sizeof(char));
	}

	String::String(Memory::Allocator* allocator, const char* str) : String(allocator, strlen(str) + 1) {
		_size = _capacity - 1;
		memcpy(_data, str, _capacity * sizeof(char));
	}

	String::String() : String(Runtime::GetContextAllocator(), uint64(0)) {}
	String::String(char* str) : String(Runtime::GetContextAllocator(), str) {}
	String::String(const char* str) : String(Runtime::GetContextAllocator(), str) {}

	String::String(const String& rhs) {
		_allocator = rhs._allocator;
		_data = (char*) _allocator->allocate(rhs._capacity, alignof(char));
		_size = rhs._size;
		_capacity = rhs._capacity;
		std::copy(rhs._data, rhs._data + rhs._capacity, _data);
	}

	String::String(String&& rhs) noexcept {
		_allocator = std::exchange(rhs._allocator, nullptr);
		_data = std::exchange(rhs._data, nullptr);
		_capacity = std::exchange(rhs._capacity, 0);
		_size = std::exchange(rhs._size, 0);
	}

	String& String::operator=(String copy) {
		copy.swap(*this);
		return *this;
	}

	String& String::operator=(char* buf) {
		_size = strlen(buf);
		if (_capacity < _size + 1) {
			reserve(_size + 1);
		}
		std::copy(buf, buf + _size + 1, _data);
		return *this;
	}

	String::~String() {
		if (_data != nullptr) {
			SOUL_ASSERT(0, _capacity != 0, "");
			_allocator->deallocate(_data, _capacity);
		}
	}

	void String::swap(String& rhs) noexcept {
		using std::swap;
		swap(_allocator, rhs._allocator);
		swap(_capacity, rhs._capacity);
		swap(_size, rhs._size);
		swap(_data, rhs._data);
	}

	void String::reserve(uint64 newCapacity) {
		char* newData = (char*)_allocator->allocate(newCapacity, alignof(char));
		if (_data != nullptr) {
			memcpy(newData, _data, _capacity * sizeof(char));
			_allocator->deallocate(_data, _capacity);
		}
		_data = newData;
		_capacity = newCapacity;
	}

	String& String::appendf(const char* format, ...) {
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