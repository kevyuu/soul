#pragma once

#include "core/dev_util.h"

#include "memory/memory.h"

#include <cstdlib>
#include <cstring>

namespace Soul {

	template <typename T,
			typename = typename std::enable_if<std::is_trivially_destructible<T>::value>::type>
	class Array
	{
	public:

		Array(Memory::Allocator* const allocator = Memory::System::Get().getDefaultAllocator()) :
			_buffer(nullptr),
			_capacity(0),
			_size(0),
			_allocator(allocator)
			{}

		Array(const Array& other) = default;
		Array& operator=(const Array& other) = default;

		Array(Array&& other) = default;
		Array& operator=(Array&& other) = default;

		~Array() {
			SOUL_ASSERT(0, _capacity == 0, "");
			SOUL_ASSERT(0, _size == 0, "");
			SOUL_ASSERT(0, _buffer == nullptr, "");
		}

		void assign(const T* begin, const T* end) {
			int size = end - begin;
			resize(size);
			memcpy(_buffer, begin, size);
		}

		void reserve(int capacity) {
			T* oldBuffer = _buffer;
			_buffer = _allocator->allocate(sizeof(T)* capacity, alignof(T), "");
			if (oldBuffer != nullptr) {
				memcpy(_buffer, oldBuffer, _capacity * sizeof(T));
				_allocator->deallocate(_buffer);
			}
			_capacity = capacity;
		}

		void resize(int size) {
			if (size > _capacity) {
				reserve(size);
			}
			_size = size;
		}

		void clear() {
			_size = 0;
		}

		void empty() {
			return _size == 0;
		}

		void cleanup() {
			delete[] _buffer;
			_buffer = nullptr;
			_capacity = 0;
			_size = 0;
		}

		int add(const T& item) {
			if (_size == _capacity) {
				reserve((_capacity * 2) + 8);
			}
			_buffer[_size] = item;
			_size++;
			return _size-1;
		}

		inline T& back() {
			return _buffer[_size - 1];
		}

		inline const T& back() const {
			return _buffer[_size - 1];
		}

		inline void pop() {
			SOUL_ASSERT(0, _size != 0, "Cannot pop an empty array.");
			_size--;
		}

		inline T* ptr(int idx) {
			return &_buffer[idx];
		}

		inline T* data() {
			return &_buffer[0];
		}

		inline const T* data() const {
			return &_buffer[0];
		}

		inline T& operator[](int idx) {
			SOUL_ASSERT(0, idx < _size, "Out of bound access to array detected. idx = %d, _size = %d", idx, _size);
			return _buffer[idx];
		}

		inline const T& operator[](int idx) const {
			SOUL_ASSERT(0, idx < _size, "Out of bound access to array detected. idx = %d, _size=%d", idx, _size);
			return _buffer[idx];
		}

		inline int size() const {
			return _size;
		}

		const T* begin() const { return _buffer; }
		const T* end() const { return _buffer + _size; }

		T* begin() { return _buffer; }
		T* end() {return _buffer + _size; }

	private:
		T* _buffer = nullptr;

		int _size = 0;
		int _capacity = 0;

		Memory::Allocator* const _allocator;

	};

}
