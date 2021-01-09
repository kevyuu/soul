#pragma once

#include "core/dev_util.h"
#include "core/util.h"
#include "memory/allocator.h"

namespace Soul {

    namespace Runtime {
        Memory::Allocator* GetContextAllocator();
    };

	template <typename T>
	class Array
	{
	public:

		Array();
		explicit Array(Memory::Allocator* allocator);
		Array(const Array& other);
		Array& operator=(const Array& other);
		Array(Array&& other) noexcept ;
		Array& operator=(Array&& other) noexcept ;
		~Array();

		void init(Memory::Allocator* allocator);
		void reserve(uint32 capacity);
		void resize(uint32 size);

		void clear();
		void cleanup();

		uint32 add(const T& item);
		uint32 add(T&& item);

		void append(const Array<T>& others);

		inline T& back() { return _buffer[_size - 1]; }
		inline const T& back() const { return _buffer[_size - 1]; }

		inline void pop() {
			SOUL_ASSERT(0, _size != 0, "Cannot pop an empty array.");
			_size--;
			((T*) _buffer + _size)->~T();
		}

		inline T* ptr(int idx) { return &_buffer[idx]; }
		inline T* data() { return &_buffer[0]; }
		inline const T* data() const { return &_buffer[0]; }

		inline T& operator[](int idx) {
			SOUL_ASSERT(0, idx < _size, "Out of bound access to array detected. idx = %d, _size = %d", idx, _size);
			return _buffer[idx];
		}

		inline const T& operator[](int idx) const {
			SOUL_ASSERT(0, idx < _size, "Out of bound access to array detected. idx = %d, _size=%d", idx, _size);
			return _buffer[idx];
		}

		inline int capacity() const { return _capacity; }

		inline int size() const { return _size; }
		inline bool empty() const {return _size == 0; }

		const T* begin() const { return _buffer; }
		const T* end() const { return _buffer + _size; }

		T* begin() { return _buffer; }
		T* end() {return _buffer + _size; }

	private:
		T* _buffer = nullptr;
		int _size = 0;
		int _capacity = 0;
		Memory::Allocator* _allocator = nullptr;

	};

	template <typename T>
	Array<T>::Array() :
		_buffer(nullptr),
		_capacity(0),
		_size(0),
		_allocator((Memory::Allocator*) Runtime::GetContextAllocator()) {}

	template<typename T>
	Array<T>::Array(Memory::Allocator* const allocator) :
		_buffer(nullptr),
		_capacity(0),
		_size(0),
		_allocator(allocator) {}

	template<typename T>
	Array<T>::~Array() {
		cleanup();
	}

	template <typename T>
	Array<T>::Array(const Array<T>& other) : _buffer(nullptr), _capacity(0), _size(0), _allocator(other._allocator) {
		reserve(other._capacity);
		Copy(other._buffer, other._buffer + other._size, _buffer);
		_size = other._size;
	}

	template <typename T>
	Array<T>& Array<T>::operator=(const Array<T>& other) {
		cleanup();
		_allocator = other._allocator;
		reserve(other._capacity);
		Copy(other._buffer, other._buffer + other._size, _buffer);
		_size = other._size;
		return *this;
	}

	template<typename T>
	Array<T>::Array(Array<T>&& other) noexcept {
		_allocator = std::move(other._allocator);
		_buffer = std::move(other._buffer);
		_size = std::move(other._size);
		_capacity = std::move(other._capacity);
		other._buffer = nullptr;
		other._size = 0;
		other._capacity = 0;
	}

	template<typename T>
	Array<T>& Array<T>::operator=(Array<T>&& other) noexcept {
		Destruct(_buffer, _buffer + _size);
		_allocator->deallocate(_buffer, sizeof(T) * _capacity);
		new (this) Array<T>(std::move(other));
		return *this;
	}

	template<typename T>
	void Array<T>::init(Memory::Allocator* allocator)
	{
		SOUL_ASSERT(0, allocator != nullptr, "");
		_allocator = allocator;
	}

	template<typename T>
	void Array<T>::reserve(uint32 capacity) {
		T* oldBuffer = _buffer;
		_buffer = (T*) _allocator->allocate(sizeof(T) * capacity, alignof(T));
		if (oldBuffer != nullptr) {
			Move(oldBuffer, oldBuffer + _capacity, _buffer);
			Destruct(oldBuffer, oldBuffer + _capacity);
			_allocator->deallocate(oldBuffer, sizeof(T) * _capacity);
		}
		_capacity = capacity;
	}

	template<typename T>
	void Array<T>::resize(uint32 size) {
		if (size > _capacity) {
			reserve(size);
		}
		for (int i = _size; i < size; i++) {
			new (_buffer + i) T();
		}
		_size = size;
	}

	template<typename T>
	void Array<T>::clear() {
		Destruct(_buffer, _buffer + _size);
		_size = 0;
	}

	template <typename T>
	void Array<T>::cleanup() {
		if (_buffer == nullptr) {
			SOUL_ASSERT(0, _size == 0, "");
			SOUL_ASSERT(0, _capacity == 0, "");
			return;
		}
		clear();
		_allocator->deallocate(_buffer, sizeof(T) * _capacity);
		_buffer = nullptr;
		_capacity = 0;
	}

	template <typename T>
	uint32 Array<T>::add(const T& item) {
		if (_size == _capacity) {
			reserve((_capacity * 2) + 8);
		}
		_buffer[_size] = item;
		_size++;
		return _size-1;
	}

	template <typename T>
	void Array<T>::append(const Array<T>& other) {
		for (const T& datum: other) {
			add(datum);
		}
	}

	template <typename T>
	uint32 Array<T>::add(T&& item) {
		if (_size == _capacity) {
			reserve((_capacity * 2) + 8);
		}
		new (_buffer + _size) T(std::move(item));
		_size++;
		return _size - 1;
	}

}
