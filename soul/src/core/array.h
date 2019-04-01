#pragma once

#include "core/debug.h"
#include <cstdlib>

namespace Soul {
	template <typename T>
	struct Array
	{

		Array() {
			_buffer = nullptr;
			_capacity = 0;
			_count = 0;
		}

		~Array() {
			SOUL_ASSERT(0, _capacity == 0);
			SOUL_ASSERT(0, _count == 0);
			SOUL_ASSERT(0, _buffer == 0);
		}

		void reserve(int capacity) {
			T* oldBuffer = _buffer;
			_buffer = (T*)malloc(capacity * sizeof(T));
			if (oldBuffer != nullptr) {
				memcpy(_buffer, oldBuffer, _capacity * sizeof(T));
				free(oldBuffer);
			}
			_capacity = capacity;
		}

		void resize(int size) {
			if (size > _capacity) {
				reserve(size);
			}
			_count = size;
		}

		void cleanup() {
			free(_buffer);
			_buffer = nullptr;
			_capacity = 0;
			_count = 0;
		}

		void add(const T& item) {

			if (_count == _capacity) {
				T* oldBuffer = _buffer;
				_buffer = (T*)malloc(sizeof(T) * 2 * _capacity);
				memcpy(_buffer, oldBuffer, sizeof(T) * _capacity);
				free(oldBuffer);
				_capacity *= 2;
			}

			_buffer[_count] = item;
			_count++;
		}

		T& back() {
			return _buffer[_count - 1];
		}

		void pop() {
			SOUL_ASSERT(0, _count != 0, "Cannot pop an empty array.");
			_count--;
		}

		T* ptr(int idx) {
			return &_buffer[idx];
		}

		T& operator[](int idx) {
			return _buffer[idx];
		}

		T& get(int idx) const {
			return _buffer[idx];
		}

		int count() const {
			return _count;
		}

		T* _buffer;

		int _count;
		int _capacity;
	};

}

