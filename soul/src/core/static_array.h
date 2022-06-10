#pragma once

#include "core/compiler.h"
#include "core/config.h"
#include "core/type.h"

namespace soul {

	template<typename T>
	class StaticArray {
	public:

		explicit StaticArray(memory::Allocator* allocator = GetDefaultAllocator()) : _allocator(allocator) {}
		StaticArray(const StaticArray& other);
		StaticArray& operator=(const StaticArray& other);
		StaticArray(StaticArray&& other) noexcept;
		StaticArray& operator=(StaticArray&& other) noexcept;
		~StaticArray() { cleanup(); }

		void swap(StaticArray& other) noexcept;
		friend void swap(StaticArray& a, StaticArray& b) noexcept { a.swap(b); }

		template<typename... ARGS>
		void init(memory::Allocator* allocator, soul_size size, ARGS&&... args);

		template<typename... ARGS>
		void init(soul_size size, ARGS&&... args);

		template<typename Construct>
		void init_construct(soul_size size, Construct func)
		{
			_size = size;
			_buffer = (T*)_allocator->allocate(_size * sizeof(T), alignof(T));
			for (soul_size i = 0; i < _size; i++) {
				func(i, _buffer + i);
			}
		}

		void cleanup();

		SOUL_NODISCARD T* ptr(soul_size idx) { SOUL_ASSERT(0, idx < _size, ""); return &_buffer[idx]; }
		SOUL_NODISCARD T* data() { return &_buffer[0]; }
		SOUL_NODISCARD const T* data() const { return &_buffer[0];}

		SOUL_NODISCARD T& operator[](soul_size idx) {
			SOUL_ASSERT(0, idx < _size, "Out of bound access to array detected. idx = %d, _size = %d", idx, _size);
			return _buffer[idx];
		}

		SOUL_NODISCARD const T& operator[](soul_size idx) const {
			SOUL_ASSERT(0, idx < _size, "Out of bound access to array detected. idx = %d, _size=%d", idx, _size);
			return _buffer[idx];
		}

		SOUL_NODISCARD soul_size size() const { return _size; }

		SOUL_NODISCARD const T* begin() const { return _buffer; }
		SOUL_NODISCARD const T* end() const { return _buffer + _size; }

		SOUL_NODISCARD T* begin() { return _buffer; }
		SOUL_NODISCARD T* end() {return _buffer + _size; }

	private:
		memory::Allocator* _allocator;
		T* _buffer = nullptr;
		soul_size _size = 0;
	};

	template <typename T>
	StaticArray<T>::StaticArray(const StaticArray<T>& other) : _allocator(other._allocator), _size(other._size) {
		_buffer = (T*) _allocator->allocate(other._size * sizeof(T), alignof(T));
		Copy(other._buffer, other._buffer + _size, _buffer);
	}

	template <typename T>
	StaticArray<T>& StaticArray<T>::operator=(const StaticArray<T>& other) {  // NOLINT(bugprone-unhandled-self-assignment)
		StaticArray<T>(other).swap(*this);
		return *this;
	}

	template <typename T>
	StaticArray<T>::StaticArray(StaticArray&& other) noexcept {
		_allocator = std::move(other._allocator);
		_buffer = std::move(other._buffer);
		_size = std::move(other._size);
		other._buffer = nullptr;
	}

	template <typename T>
	StaticArray<T>& StaticArray<T>::operator=(StaticArray&& other) noexcept {
		StaticArray<T>(std::move(other)).swap(*this);
		return *this;
	}

	template<typename T>
	void StaticArray<T>::swap(StaticArray& other) noexcept {
		using std::swap;
		swap(_allocator, other._allocator);
		swap(_buffer, other._buffer);
		swap(_size, other._size);
	}

    template <typename T>
    template <typename... ARGS>
    void StaticArray<T>::init(memory::Allocator* allocator, soul_size size, ARGS&&... args) {
        SOUL_ASSERT(0, size != 0, "");
        SOUL_ASSERT(0, _buffer == nullptr, "Array have been initialized before");
        SOUL_ASSERT(0, _allocator == nullptr, "");
        _allocator = allocator;
        init(size, std::forward<ARGS>(args) ...);
    }

	template <typename T>
	template <typename... ARGS>
	void StaticArray<T>::init(soul_size size, ARGS&&... args) {
		SOUL_ASSERT(0, size != 0, "");
		SOUL_ASSERT(0, _buffer == nullptr, "Array have been initialized before");
		_size = size;
		_buffer = (T*) _allocator->allocate(_size * sizeof(T), alignof(T));
		for (soul_size i = 0; i < _size; i++) {
			new (_buffer + i) T(std::forward<ARGS>(args) ... );
		}
	}

	template <typename T>
	void StaticArray<T>::cleanup() {
		_allocator->deallocate(_buffer);
		Destruct(_buffer, _buffer + _size);
		_buffer = nullptr;
	}

}