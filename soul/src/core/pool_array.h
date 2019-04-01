#pragma once

#include "core/type.h"
#include "core/debug.h"
#include <cstdlib>

namespace Soul {

	typedef uint32 PoolID;

	template <typename T>
	struct PoolArray {

		PoolArray() {
			_capacity = 0;
			_count = 0;
			_freelist = 0;
			_buffer = nullptr;
		}

		~PoolArray() {
			SOUL_ASSERT(0, _capacity == 0, "Pool array capacity is not zero. You might forget to call cleanup()");
			SOUL_ASSERT(0, _count == 0, "Pool array count is not zero. You might forget to call cleanup()");
			SOUL_ASSERT(0, _buffer == nullptr, "Pool array buffer is not nullptr. You might forget to call cleanup()");
		}

		inline void reserve(uint32 capacity) {
			Unit* oldBuffer = _buffer;
			_buffer = (Unit*) malloc(capacity * sizeof(Unit));
			if (oldBuffer != nullptr) {
				memcpy(_buffer, oldBuffer, _capacity * sizeof(Unit));
				free(oldBuffer);
			}
			for (int i = _count; i < capacity; i++) {
				_buffer[i].next = i + 1;
			}
			_freelist = _count;
			_capacity = capacity;
		}

		inline PoolID add(const T& datum) {
			if (_count == _capacity) {
				reserve(_capacity * 2 + 1);
			}
			PoolID id = _freelist;
			_freelist = _buffer[_freelist].next;
			_buffer[id].datum = datum;
			_count++;
			return id;
		}

		inline void remove(PoolID id) {
			_count--;
			_buffer[id].next = _freelist;
			_freelist = id;
		}

		T& operator[](PoolID id) {
			SOUL_ASSERT(0, id < _capacity, "Pool Array access violation");
			return _buffer[id].datum;
		}

		const T& get(PoolID id) const {
			SOUL_ASSERT(0, id < _capacity, "Pool Array access violation");
			return _buffer[id].datum;
		}

		T* ptr(PoolID id) const {
			SOUL_ASSERT(0, id < _capacity, "Pool Array access violation");
			return &(_buffer[id].datum);
		}

		uint32 count() const {
			return _count;
		}

		inline void cleanup() {
			_capacity = 0;
			_count = 0;
			free(_buffer);
			_buffer = nullptr;
		}

		union Unit {
			T datum;
			uint32 next;
		};

		Unit* _buffer;
		uint32 _capacity;
		uint32 _count;
		uint32 _freelist;

	};
}