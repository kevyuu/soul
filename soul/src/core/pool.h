#pragma once

#include "core/type.h"
#include "core/dev_util.h"
#include "core/bitset.h"

#include "memory/allocator.h"

#include <cstdlib>

namespace Soul {

    namespace Runtime {
        Memory::Allocator* GetContextAllocator();
    }

	using PoolID = uint32;

	template <typename T>
	class Pool {
	public:

		Pool() : _capacity(0), _size(0), _freelist(0), _buffer(nullptr), _allocator((Memory::Allocator*) Runtime::GetContextAllocator()), _bitSet() {}
		explicit Pool(Memory::Allocator* allocator) : _capacity(0), _size(0), _freelist(0), _buffer(nullptr), _allocator(allocator), _bitSet(_allocator) {}
		Pool(const Pool& other);
		Pool& operator=(const Pool& other);
		Pool(Pool&& other) noexcept;
		Pool& operator=(Pool&& other) noexcept;
		~Pool() { cleanup(); }

		void reserve(uint32 capacity);

		PoolID add(const T& datum);
		PoolID add(T&& datum);

		void remove(PoolID id);

		inline T& operator[](PoolID id) {
			SOUL_ASSERT(0, id < _capacity, "id = %d, _capacity = %d", id , _capacity);
			return _buffer[id].datum;
		}

		inline const T& operator[](PoolID id) const {
			SOUL_ASSERT(0, id < _capacity, "id = %d, _capacity = %d", id, _capacity);
			return _buffer[id].datum;
		}

		inline const T& get(PoolID id) const {
			SOUL_ASSERT(0, id < _capacity, "id = %d, _capacity = %d", id, _capacity);
			return _buffer[id].datum;
		}

		inline T* ptr(PoolID id) const {
			SOUL_ASSERT(0, id < _capacity, "id = %d, _capacity = %d", id, _capacity);
			return &(_buffer[id].datum);
		}

		inline uint32 size() const { return _size; }
		inline bool empty() const { return _size == 0; }

		void clear();
		void cleanup();

	private:

		PoolID _allocate();

		union Unit {
			T datum;
			uint32 next;
		};

		Memory::Allocator* _allocator;
		BitSet _bitSet;
		Unit* _buffer;
		uint32 _capacity;
		uint32 _size;
		uint32 _freelist;

		template <typename U = T,
				typename std::enable_if_t<std::is_trivially_move_constructible<U>::value>* = nullptr>
		inline void _moveUnits(Unit* dst) {
			memcpy((void*)dst, (void*)_buffer, _size * sizeof(Unit));
		}

		template <typename U = T,
				typename std::enable_if_t<!std::is_trivially_move_constructible<U>::value>* = nullptr>
		inline void _moveUnits(Unit* dst) {
			for (int i = 0; i < _size; i++) {
				if (_bitSet.test(i)) {
					new (dst + i) T(std::move(_buffer[i].datum));
				} else {
					dst[i].next = _buffer[i].next;
				}
			}
		}

		template <typename U = T,
				typename std::enable_if_t<std::is_trivially_copyable<U>::value>* = nullptr>
		inline void _copyUnits(const Pool<T>& other) {
			memcpy(_buffer, other._buffer, other._size * sizeof(Unit));
		}

		template <typename U = T,
				typename std::enable_if_t<!std::is_trivially_destructible<U>::value>* = nullptr>
		inline void _copyUnits(const Pool<T>& other) {
			for (int i = 0; i < _capacity; i++) {
				if (_bitSet.test(i)) {
					new(_buffer + i) T(other._buffer[i].datum);
				} else {
					_buffer[i].next = other._buffer[i].next;
				}
			}
		}

		template <typename U = T,
				typename std::enable_if_t<std::is_trivially_destructible<U>::value>* = nullptr>
		inline void _destructUnits() {}

		template <typename U = T,
				typename std::enable_if_t<!std::is_trivially_destructible<U>::value>* = nullptr>
		inline void _destructUnits() {
			for (int i = 0; i < _capacity; i++) {
				if (!_bitSet.test(i)) continue;
				_buffer[i]->datum.~T();
			}
		}

	};

	template <typename T>
	Pool<T>::Pool(const Pool& other) : _allocator(other._allocator), _bitSet(other._bitSet), _buffer(nullptr), _size(0), _capacity(0), _freelist(0) {
		reserve(other._capacity);
		_copyUnits(other);
		_freelist = other._freelist;
		_size = other._size;
	}

	template <typename T>
	Pool<T>& Pool<T>::operator=(const Pool& other) {
		cleanup();
		reserve(other._capacity);
		_copyUnits(other);
		_freelist = other._freelist;
		_size = other._size;
	}

	template <typename T>
	Pool<T>::Pool(Pool&& other) noexcept : _bitSet(std::move(other._bitSet)) {
		_allocator = std::move(other._allocator);
		_buffer = std::move(other._buffer);
		_capacity = std::move(other._capacity);
		_size = std::move(other._size);
		_freelist = std::move(other._freelist);

		other._buffer = nullptr;
		other._capacity = 0;
		other._size = 0;
		other._freelist = 0;
	}

	template <typename T>
	Pool<T>& Pool<T>::operator=(Pool&& other) noexcept {
		this->cleanup();
		new (this) Pool<T>(std::move(other));
		return *this;
	}

	template<typename T>
	void Pool<T>::reserve(uint32 capacity) {
		Unit* newBuffer = (Unit*) _allocator->allocate(capacity * sizeof(Unit), alignof(Unit));
		if (_buffer != nullptr) {
			_moveUnits(newBuffer);
			_destructUnits();
			_allocator->deallocate(_buffer, sizeof(Unit) * _capacity);
		}
		_buffer = newBuffer;
		for (int i = _capacity; i < capacity; i++) {
			_buffer[i].next = i + 1;
		}
		_freelist = _size;
		_capacity = capacity;

		_bitSet.resize(_capacity);
	}

	template<typename T>
	PoolID Pool<T>::_allocate() {
		if (_size == _capacity) {
			reserve(_capacity * 2 + 8);
		}
		PoolID id = _freelist;
		_freelist = _buffer[_freelist].next;
		_size++;
		return id;
	}

	template<typename T>
	PoolID Pool<T>::add(const T& datum) {
		PoolID id = _allocate();
		new (_buffer + id) T(datum);
		_bitSet.set(id);
		return id;
	}

	template<typename T>
	PoolID Pool<T>::add(T&& datum) {
		PoolID id = _allocate();
		new (_buffer + id) T(std::move(datum));
		_bitSet.set(id);
		return id;
	}

	template <typename T>
	void Pool<T>::remove(PoolID id) {
		_size--;
		_buffer[id].datum.~T();
		_buffer[id].next = _freelist;
		_freelist = id;
		_bitSet.unset(id);
	}

	template <typename T>
	void Pool<T>::clear() {
		_destructUnits();
		_size = 0;
		_freelist = 0;
		for (int i = 0; i < _capacity; i++) {
			_buffer[i].next = i + 1;
		}
		_bitSet.clear();
	}

	template <typename T>
	void Pool<T>::cleanup() {
		_destructUnits();
		_capacity = 0;
		_size = 0;
		_allocator->deallocate(_buffer, sizeof(T) * _capacity);
		_buffer = nullptr;
		_bitSet.cleanup();
	}

}