#pragma once

#include "core/type.h"

namespace Soul {

    namespace Runtime {
        Memory::Allocator* GetContextAllocator();
    }

	template<typename T>
	class StaticArray {
	public:

		StaticArray() : _allocator((Memory::Allocator*) Runtime::GetContextAllocator()) {};
		explicit StaticArray(Memory::Allocator* allocator) : _allocator(allocator), _size(0), _buffer(nullptr) {}
		~StaticArray() { cleanup(); }

		StaticArray(const StaticArray& other);
		StaticArray& operator=(const StaticArray& other);

		StaticArray(StaticArray&& other) noexcept;
		StaticArray& operator=(StaticArray&& other) noexcept;

		template<typename... ARGS>
		void init(Memory::Allocator* allocator, uint32 size, ARGS&&... args);

		template<typename... ARGS>
		void init(uint32 size, ARGS&&... args);
		void cleanup();

		inline T* ptr(int idx) { SOUL_ASSERT(0, idx < _size, ""); return &_buffer[idx]; }
		inline T* data() { return &_buffer[0]; }
		inline const T* data() const { return &_buffer[0];}

		inline T& operator[](int idx) {
			SOUL_ASSERT(0, idx < _size, "Out of bound access to array detected. idx = %d, _size = %d", idx, _size);
			return _buffer[idx];
		}

		inline const T& operator[](int idx) const {
			SOUL_ASSERT(0, idx < _size, "Out of bound access to array detected. idx = %d, _size=%d", idx, _size);
			return _buffer[idx];
		}

		inline int size() const { return _size; }

		const T* begin() const { return _buffer; }
		const T* end() const { return _buffer + _size; }

		T* begin() { return _buffer; }
		T* end() {return _buffer + _size; }

	private:
		Memory::Allocator* _allocator;
		T* _buffer = nullptr;
		uint32 _size = 0;
	};

	template <typename T>
	StaticArray<T>::StaticArray(const StaticArray<T>& other) : _allocator(other._allocator), _size(other._size) {
		_buffer = (T*) _allocator->allocate(other._size * sizeof(T), alignof(T));
		Copy(other._buffer, other._buffer + _size, _buffer);
	}

	template <typename T>
	StaticArray<T>& StaticArray<T>::operator=(const StaticArray<T>& other) {
		if (this == &other) return *this;
		cleanup();
		new (this) StaticArray<T>(other);
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
		cleanup();
		_allocator = std::move(other._allocator);
		_buffer = std::move(other._buffer);
		_size = std::move(other._size);
		other._buffer = nullptr;
		return *this;
	}

    template <typename T>
    template <typename... ARGS>
    void StaticArray<T>::init(Memory::Allocator* allocator, uint32 size, ARGS&&... args) {
        SOUL_ASSERT(0, size != 0, "");
        SOUL_ASSERT(0, _buffer == nullptr, "Array have been initialized before");
        SOUL_ASSERT(0, _allocator == nullptr, "");
        _allocator = allocator;
        init(size, std::forward<ARGS>(args) ...);
    }

	template <typename T>
	template <typename... ARGS>
	void StaticArray<T>::init(uint32 size, ARGS&&... args) {
		SOUL_ASSERT(0, size != 0, "");
		SOUL_ASSERT(0, _buffer == nullptr, "Array have been initialized before");
		_size = size;
		_buffer = (T*) _allocator->allocate(_size * sizeof(T), alignof(T));
		for (int i = 0; i < _size; i++) {
			new (_buffer + i) T(std::forward<ARGS>(args) ... );
		}
	}

	template <typename T>
	void StaticArray<T>::cleanup() {
		_allocator->deallocate(_buffer, sizeof(T) * _size);
		Destruct(_buffer, _buffer + _size);
		_buffer = nullptr;
	}

}