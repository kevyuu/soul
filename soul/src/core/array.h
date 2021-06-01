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

		Array() noexcept;
		explicit Array(Memory::Allocator* allocator);
		Array(const Array& other);
		Array& operator=(const Array& rhs);
		Array(Array&& other) noexcept;
		Array& operator=(Array&& other) noexcept;
		~Array();

		void swap(Array<T>& other) noexcept;
		static void swap(Array<T>& a, Array<T>& b) noexcept { a.swap(b); }

		void init(Memory::Allocator* allocator) noexcept;
		void reserve(soul_size capacity);
		void resize(soul_size size);

		void clear() noexcept;
		void cleanup();

		soul_size add(const T& item);
		soul_size add(T&& item);

		void append(const Array<T>& other);

		[[nodiscard]] inline T& back() noexcept { return _buffer[_size - 1]; }
		[[nodiscard]] inline const T& back() const noexcept { return _buffer[_size - 1]; }

		inline void pop() {
			SOUL_ASSERT(0, _size != 0, "Cannot pop an empty array.");
			_size--;
			((T*) _buffer + _size)->~T();
		}

		[[nodiscard]] inline T* ptr(int idx) { return &_buffer[idx]; }
		[[nodiscard]] inline T* data() noexcept { return &_buffer[0]; }
		[[nodiscard]] inline const T* data() const noexcept { return &_buffer[0]; }

		[[nodiscard]] inline T& operator[](soul_size idx) {
			SOUL_ASSERT(0, idx < _size, "Out of bound access to array detected. idx = %d, _size = %d", idx, _size);
			return _buffer[idx];
		}

		[[nodiscard]] inline const T& operator[](soul_size idx) const {
			SOUL_ASSERT(0, idx < _size, "Out of bound access to array detected. idx = %d, _size=%d", idx, _size);
			return _buffer[idx];
		}

		[[nodiscard]] inline soul_size capacity() const noexcept { return _capacity; }
		[[nodiscard]] inline soul_size size() const noexcept { return _size; }
		[[nodiscard]] inline bool empty() const noexcept {return _size == 0; }

		[[nodiscard]] const T* begin() const { return _buffer; }
		[[nodiscard]] const T* end() const { return _buffer + _size; }

		T* begin() { return _buffer; }
		T* end() {return _buffer + _size; }

	private:
		Memory::Allocator* _allocator = nullptr;
		T* _buffer = nullptr;
		soul_size _size = 0;
		soul_size _capacity = 0;

	};

	template <typename T>
	Array<T>::Array() noexcept :
		_allocator(Runtime::GetContextAllocator()),
		_buffer(nullptr) {}

	template<typename T>
	Array<T>::Array(Memory::Allocator* const allocator) :
		_allocator(allocator),
		_buffer(nullptr) {}

	template <typename T>
	Array<T>::Array(const Array<T>& other) : _allocator(other._allocator), _buffer(nullptr) {
		reserve(other._capacity);
		Copy(other._buffer, other._buffer + other._size, _buffer);
		_size = other._size;
	}

	template <typename T>
	Array<T>& Array<T>::operator=(const Array<T>& rhs) {  // NOLINT(bugprone-unhandled-self-assignment)
		Array<T>(rhs).swap(*this);
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
		Array<T>(std::move(other)).swap(*this);
		return *this;
	}

	template<typename T>
	Array<T>::~Array() {
		cleanup();
	}

	template<typename T>
	void Array<T>::swap(Array<T>& other) noexcept {
		using std::swap;
		swap(_allocator, other._allocator);
		swap(_buffer, other._buffer);
		swap(_size, other._size);
		swap(_capacity, other.capacity);
	}

	template<typename T>
	void Array<T>::init(Memory::Allocator* allocator) noexcept
	{
		SOUL_ASSERT(0, allocator != nullptr, "");
		_allocator = allocator;
	}

	template<typename T>
	void Array<T>::reserve(soul_size capacity) {
		T* oldBuffer = _buffer;
		_buffer = (T*) _allocator->allocate(sizeof(T) * capacity, alignof(T));  // NOLINT(bugprone-sizeof-expression)
		if (oldBuffer != nullptr) {
			Move(oldBuffer, oldBuffer + _capacity, _buffer);
			Destruct(oldBuffer, oldBuffer + _capacity);
			_allocator->deallocate(oldBuffer, sizeof(T) * _capacity);  // NOLINT(bugprone-sizeof-expression)
		}
		_capacity = capacity;
	}

	template<typename T>
	void Array<T>::resize(soul_size size) {
		if (size > _capacity) {
			reserve(size);
		}
		for (soul_size i = _size; i < size; i++) {
			new (_buffer + i) T();
		}
		_size = size;
	}

	template<typename T>
	void Array<T>::clear() noexcept
	{
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
		_allocator->deallocate(_buffer, _capacity * sizeof(T));  // NOLINT(bugprone-sizeof-expression)
		_buffer = nullptr;
		_capacity = 0;
	}

	template <typename T>
	soul_size Array<T>::add(const T& item) {
		if (_size == _capacity) {
			reserve((_capacity * 2) + 8);
		}
		new (_buffer + _size) T(item);
		++_size;
		return _size-1;
	}

	template <typename T>
	void Array<T>::append(const Array<T>& other) {
		for (const T& datum: other) {
			add(datum);
		}
	}

	template <typename T>
	soul_size Array<T>::add(T&& item) {
		if (_size == _capacity) {
			reserve((_capacity * 2) + 8);
		}
		new (_buffer + _size) T(std::move(item));
		++_size;
		return _size - 1;
	}

}
