#pragma once

#include "core/debug.h"
#include <cstdlib>
#include <cstring>

namespace Soul {

	template <typename T>
	struct Array
	{
		Array() {
			_buffer = nullptr;
			_capacity = 0;
			_size = 0;
		}

		~Array() {
			SOUL_ASSERT(0, _capacity == 0, "");
			SOUL_ASSERT(0, _size == 0, "");
			SOUL_ASSERT(0, _buffer == 0, "");
		}

		void reserve(int capacity) {
			T* oldBuffer = _buffer;
			_buffer = new T[capacity];
			if (oldBuffer != nullptr) {
				memcpy(_buffer, oldBuffer, _capacity * sizeof(T));
				delete[] oldBuffer;
			}
			_capacity = capacity;
		}

		void resize(int size) {
			if (size > _capacity) {
				reserve(size);
			}
			_size = size;
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

		inline const T* constdata() const {
			return &_buffer[0];
		}

		inline T& operator[](int idx) {
			SOUL_ASSERT(0, idx < _size, "Out of bound access to array detected.");
			return _buffer[idx];
		}

		inline const T& get(int idx) const {
			return _buffer[idx];
		}

		inline int size() const {
			return _size;
		}

		T* _buffer = nullptr;

		int _size = 0;
		int _capacity = 0;
	};

}

