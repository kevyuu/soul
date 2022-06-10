#pragma once

#include "core/config.h"
#include "core/bitset.h"
#include "core/dev_util.h"
#include "core/compiler.h"
#include "core/type.h"

#include "memory/allocator.h"

namespace soul {

    // ReSharper disable once CppInconsistentNaming
    using PoolID = uint32;

	template <typename T>
	class Pool {
	public:
		
		explicit Pool(memory::Allocator* allocator = GetDefaultAllocator()) noexcept;
		Pool(const Pool& other);
		Pool& operator=(const Pool& other);
		Pool(Pool&& other) noexcept;
		Pool& operator=(Pool&& other) noexcept;
		~Pool();

		void swap(Pool& other) noexcept;
		friend void swap(Pool& a, Pool& b) noexcept{ a.swap(b);  }

		void reserve(soul_size capacity);

		PoolID add(const T& datum);
		PoolID add(T&& datum);

		void remove(PoolID id);

		SOUL_NODISCARD T& operator[](PoolID id) {
			SOUL_ASSERT(0, id < _capacity, "Pool access violation");
			return _buffer[id].datum;
		}

		SOUL_NODISCARD const T& operator[](PoolID id) const {
			SOUL_ASSERT(0, id < _capacity, "Pool access violation");
			return _buffer[id].datum;
		}

		SOUL_NODISCARD const T& get(PoolID id) const {
			SOUL_ASSERT(0, id < _capacity, "Pool access violation");
			return _buffer[id].datum;
		}

		SOUL_NODISCARD T* ptr(PoolID id) const {
			SOUL_ASSERT(0, id < _capacity, "Pool access violation");
			return &(_buffer[id].datum);
		}

		SOUL_NODISCARD uint32 size() const { return _size; }
		SOUL_NODISCARD bool empty() const { return _size == 0; }

		void clear();
		void cleanup();

	private:

		PoolID _allocate();

		union Unit {
			T datum;
			soul_size next;
		};

		memory::Allocator* _allocator;
		BitSet _bitSet;
		Unit* _buffer = nullptr;
		soul_size _capacity = 0;
		soul_size _size = 0;
		soul_size _freelist = 0;

		template <typename U = T,
				std::enable_if_t<std::is_trivially_move_constructible_v<U>>* = nullptr>
		inline void _moveUnits(Unit* dst) {
			memcpy((void*)dst, (void*)_buffer, _size * sizeof(Unit));
		}

		template <typename U = T,
				std::enable_if_t<!std::is_trivially_move_constructible_v<U>>* = nullptr>
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
				std::enable_if_t<std::is_trivially_copyable_v<U>>* = nullptr>
		inline void _copyUnits(const Pool<T>& other) {
			memcpy(_buffer, other._buffer, other._size * sizeof(Unit));
		}

		template <typename U = T,
				std::enable_if_t<!std::is_trivially_destructible_v<U>>* = nullptr>
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
				std::enable_if_t<std::is_trivially_destructible_v<U>>* = nullptr>
		inline void _destructUnits() {}

		template <typename U = T,
				std::enable_if_t<!std::is_trivially_destructible_v<U>>* = nullptr>
		inline void _destructUnits() {
			for (int i = 0; i < _capacity; i++) {
				if (!_bitSet.test(i)) continue;
				_buffer[i].datum.~T();
			}
		}

	};

	template<typename T>
	Pool<T>::Pool(memory::Allocator* allocator) noexcept: _allocator(allocator), _bitSet(_allocator) {}

	template <typename T>
	Pool<T>::Pool(const Pool& other) : _allocator(other._allocator), _bitSet(other._bitSet), _buffer(nullptr), _size(0), _capacity(0), _freelist(0) {
		reserve(other._capacity);
		_copyUnits(other);
		_freelist = other._freelist;
		_size = other._size;
	}

	template <typename T>
	Pool<T>& Pool<T>::operator=(const Pool& other) {  // NOLINT(bugprone-unhandled-self-assignment)
		Pool<T>(other).swap(*this);
		return *this;
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
	Pool<T>::~Pool() {
		cleanup();
	}

	template <typename T>
	void Pool<T>::swap(Pool& other) noexcept {
		using std::swap;
		swap(_allocator, other._allocator);
		swap(_bitSet, other._bitSet);
		swap(_buffer, other._buffer);
		swap(_capacity, other._capacity);
		swap(_size, other._size);
		swap(_freelist, other._freelist);
	}

	template <typename T>
	Pool<T>& Pool<T>::operator=(Pool&& other) noexcept {
		Pool<T>(std::move(other)).swap(*this);
		return *this;
	}

	template<typename T>
	void Pool<T>::reserve(soul_size capacity) {
		Unit* newBuffer = (Unit*) _allocator->allocate(capacity * sizeof(Unit), alignof(Unit));
		if (_buffer != nullptr) {
			_moveUnits(newBuffer);
			_destructUnits<T>();
			_allocator->deallocate(_buffer);
		}
		_buffer = newBuffer;
		for (uint64 i = _capacity; i < capacity; i++) {
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
		_destructUnits<T>();
		_size = 0;
		_freelist = 0;
		for (uint64 i = 0; i < _capacity; i++) {
			_buffer[i].next = i + 1;
		}
		_bitSet.clear();
	}

	template <typename T>
	void Pool<T>::cleanup() {
		_destructUnits<T>();
		_capacity = 0;
		_size = 0;
		_allocator->deallocate(_buffer);
		_buffer = nullptr;
		_bitSet.cleanup();
	}

}