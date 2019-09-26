#pragma once

#include "core/type.h"
#include "core/debug.h"
#include <cstdlib>

namespace Soul {

	using PoolID = uint32;

	template <typename T>
	struct PoolArray {

		PoolArray() {
			_capacity = 0;
			_size = 0;
			_freelist = 0;
			_buffer = nullptr;
		}

		~PoolArray() {
			SOUL_ASSERT(0, _capacity == 0, "Pool array capacity is not zero. You might forget to call cleanup()");
			SOUL_ASSERT(0, _size == 0, "Pool array size is not zero. You might forget to call cleanup()");
			SOUL_ASSERT(0, _buffer == nullptr, "Pool array buffer is not nullptr. You might forget to call cleanup()");
		}

		inline void reserve(uint32 capacity) {
			Unit* oldBuffer = _buffer;
			_buffer = (Unit*) malloc(capacity * sizeof(*_buffer));
			if (oldBuffer != nullptr) {
				memcpy(_buffer, oldBuffer, _capacity * sizeof(*_buffer));
				free(oldBuffer);
			}
			for (int i = _size; i < capacity; i++) {
				_buffer[i].next = i + 1;
			}
			_freelist = _size;
			_capacity = capacity;
		}

		inline PoolID add(const T& datum) {
			if (_size == _capacity) {
				reserve(_capacity * 2 + 1);
			}
			PoolID id = _freelist;
			_freelist = _buffer[_freelist].next;
			_buffer[id].datum = datum;
			_size++;
			return id;
		}

		inline void remove(PoolID id) {
			_size--;
			_buffer[id].next = _freelist;
			_freelist = id;
		}

		inline T& operator[](PoolID id) {
			SOUL_ASSERT(0, id < _capacity, "Pool Array access violation");
			return _buffer[id].datum;
		}

		inline const T& get(PoolID id) const {
			SOUL_ASSERT(0, id < _capacity, "Pool Array access violation");
			return _buffer[id].datum;
		}

		inline T* ptr(PoolID id) const {
			SOUL_ASSERT(0, id < _capacity, "Pool Array access violation");
			return &(_buffer[id].datum);
		}

		inline uint32 size() const {
			return _size;
		}

		inline void clear()
		{
			_size = 0;
			_freelist = 0;
			for (int i = 0; i < _capacity; i++)
			{
				_buffer[i].next = i + 1;
			}
		}

		inline void cleanup() 
		{
			_capacity = 0;
			_size = 0;
			free(_buffer);
			_buffer = nullptr;
		}

		union Unit {
			T datum;
			uint32 next;
		};

		Unit* _buffer;
		uint32 _capacity;
		uint32 _size;
		uint32 _freelist;

	};
}