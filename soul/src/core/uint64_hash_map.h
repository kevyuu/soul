#pragma once

#include "core/config.h"
#include "core/dev_util.h"
#include "core/type.h"

namespace soul {

	template <typename T>
	class UInt64HashMap {

	public:

		explicit UInt64HashMap(memory::Allocator* allocator = GetDefaultAllocator()) : _allocator(allocator) {}
		UInt64HashMap(const UInt64HashMap& other);
		UInt64HashMap& operator=(const UInt64HashMap& other);
		UInt64HashMap(UInt64HashMap&& other) noexcept;
		UInt64HashMap& operator=(UInt64HashMap&& other) noexcept;
		~UInt64HashMap() { cleanup();}

		void swap(UInt64HashMap<T>& other) noexcept;
		friend void swap(UInt64HashMap<T>& a, UInt64HashMap<T>& b) noexcept { a.swap(b); }

		void clear();
		void cleanup();

		void reserve(soul_size capacity);
		void add(uint64 key, const T& value);
		void add(uint64 key, T&& value);

		void remove(uint64 key);

		SOUL_NODISCARD bool isExist(uint64 key) const noexcept
		{
			if (_size == 0) return false;
			uint32 index = _findIndex(key);
			return (_indexes[index].key == key && _indexes[index].dib != 0);
		}

		SOUL_NODISCARD T& operator[](uint64 key);
		SOUL_NODISCARD const T& operator[](uint64 key) const;

		SOUL_NODISCARD soul_size size() const noexcept { return _size; }
		SOUL_NODISCARD soul_size capacity() const { return _capacity; }
		SOUL_NODISCARD bool empty() const noexcept { return _size == 0; }

	private:
		memory::Allocator* _allocator = nullptr;
		struct Index {
			uint64 key;
			soul_size dib;
		};
		Index * _indexes = nullptr;
		T* _values = nullptr;

		soul_size _size = 0;
		soul_size _capacity = 0;
		soul_size _maxDib = 0;


		inline void _initAssert() {
			SOUL_ASSERT(0, _indexes == nullptr, "");
			SOUL_ASSERT(0, _values == nullptr, "");
			SOUL_ASSERT(0, _size == 0, "");
			SOUL_ASSERT(0, _capacity == 0, "");
			SOUL_ASSERT(0, _maxDib == 0, "");
		}

		soul_size _findIndex(uint64 key) const {
			uint32 baseIndex = key % _capacity;
			uint32 iterIndex = baseIndex;
			uint32 dib = 0;
			while ((_indexes[iterIndex].key != key) && (_indexes[iterIndex].dib != 0) && (dib < _maxDib)) {
				dib++;
				iterIndex++;
				iterIndex %= _capacity;
			}
			return iterIndex;
		}

		void _removeByIndex(soul_size index) {
			uint32 nextIndex = index + 1;
			nextIndex %= _capacity;
			while(_indexes[nextIndex].dib > 1) {
				_indexes[index].key = _indexes[nextIndex].key;
				_indexes[index].dib = _indexes[nextIndex].dib - 1;
				_values[index] = std::move(_values[nextIndex]);
				index = (index + 1) % _capacity;
				nextIndex = (nextIndex + 1) % _capacity;
			}
			_indexes[index].dib = 0;
			_size--;
		}

		template <
			typename U = T
		>
		requires is_trivially_move_constructible_v<U>
		void _moveValues(const UInt64HashMap<U>& other) {
			memcpy(_values, other._values, sizeof(U) * other._capacity);
		}

		template <
			typename U = T
		>
		requires (!is_trivially_move_constructible_v<U>)
		void _moveValues(const UInt64HashMap<U>& other) {
			for (soul_size i = 0; i < other._capacity; ++i) {
				if (other._indexes[i].dib == 0) continue;
				new (_values + i) U(std::move(other._values[i]));
			}
		}

		template <
			typename U = T
		>
		requires is_trivially_copyable_v<U>
		void _copyValues(const UInt64HashMap<T>& other) {
			memcpy(_values, other._values, sizeof(T) * other._capacity);
		}

		template <
			typename U = T
		>
		requires (!is_trivially_copyable_v<U>)
		void _copyValues(const UInt64HashMap<T>& other) {
			for (int i = 0; i < other._capacity; i++) {
				if (other._indexes[i].dib == 0) continue;
				new (_values + i) T(other._values[i]);
			}
		}

		template <
			typename U = T>
		void _destructValues()
		{
			if constexpr (!is_trivially_destructible_v<U>) {
				for (int i = 0; i < _capacity; i++) {
					if (_indexes[i].dib != 0) {
						_values[i].~U();
					}
				}
			}
		}
		
	};

	template <typename T>
	UInt64HashMap<T>::UInt64HashMap(const UInt64HashMap& other) :  _allocator(other._allocator) {
		_capacity = other.capacity();
		_size = other.size();
		_maxDib = other._maxDib;

		_indexes = (Index*) _allocator->allocate(_capacity * sizeof(Index), alignof(Index));
		memcpy(_indexes, other._indexes, sizeof(Index) * _capacity);

		_values = (T*) _allocator->allocate(_capacity * sizeof(T), alignof(T));
		_copyValues(other);
	}

	template <typename T>
	UInt64HashMap<T>& UInt64HashMap<T>::operator=(const UInt64HashMap& other) {  // NOLINT(bugprone-unhandled-self-assignment)
		UInt64HashMap<T>(other).swap(*this);
		return *this;
	}

	template<typename T>
	UInt64HashMap<T>::UInt64HashMap(UInt64HashMap&& other) noexcept {
		_indexes = std::move(other._indexes);
		_values = std::move(other._values);
		_size = std::move(other._size);
		_capacity = std::move(other._capacity);
		_maxDib = std::move(other._maxDib);
		_allocator = std::move(other._allocator);

		other._indexes = nullptr;
		other._values = nullptr;
		other._size = 0;
		other._capacity = 0;
		other._maxDib = 0;
	}

	template<typename T>
	UInt64HashMap<T>& UInt64HashMap<T>::operator=(UInt64HashMap&& other) noexcept {
		UInt64HashMap<T>(std::move(other)).swap(*this);
		return *this;
	}

	template<typename T>
	void UInt64HashMap<T>::swap(UInt64HashMap<T>& other) noexcept
	{
		using std::swap;
		swap(_allocator, other._allocator);
		swap(_indexes, other._indexes);
		swap(_size, other._size);
		swap(_capacity, other._capacity);
		swap(_maxDib, other._maxDib);
	}

	template <typename T>
	void UInt64HashMap<T>::clear() {
		_destructValues();
		memset(_indexes, 0, sizeof(Index) * _capacity);
		_maxDib = 0;
		_size = 0;
	}

	template <typename T>
	void UInt64HashMap<T>::cleanup() {
		_destructValues();
		_allocator->deallocate(_indexes);
		_allocator->deallocate(_values);  // NOLINT(bugprone-sizeof-expression)
		_maxDib = 0;
		_size = 0;
		_capacity = 0;
	}

	template <typename T>
	void UInt64HashMap<T>::reserve(soul_size capacity) {
		SOUL_ASSERT(0, _size == _capacity, "");
		Index* oldIndexes = _indexes;
		_indexes = (Index*) _allocator->allocate(capacity * sizeof(Index), alignof(Index));
		memset(_indexes, 0, sizeof(Index) * capacity);
		T* oldValues = _values;
		_values = (T*) _allocator->allocate(capacity * sizeof(T), alignof(T));
		const soul_size oldCapacity = _capacity;
		_capacity = capacity;
		_maxDib = 0;
		_size = 0;

		if (oldCapacity != 0) {
			SOUL_ASSERT(0, oldIndexes != nullptr, "");
			SOUL_ASSERT(0, oldValues != nullptr, "");
			for (soul_size i = 0; i < oldCapacity; ++i) {
				if (oldIndexes[i].dib != 0) {
					add(oldIndexes[i].key, std::move(oldValues[i]));
				}
			}
			_allocator->deallocate(oldIndexes);
			_allocator->deallocate(oldValues);
		}
	}

	template <typename T>
	void UInt64HashMap<T>::add(uint64 key, const T& value) {
		T valueToInsert = value;
		add(key, std::move(valueToInsert));
	}

	template <typename T>
	void UInt64HashMap<T>::add(uint64 key, T&& value) {
		if (_size == _capacity) {
			reserve(_capacity * 2 + 8);
		}
		const uint32 baseIndex = uint32(key % _capacity);
		uint32 iterIndex = baseIndex;
		T valueToInsert = std::move(value);
		uint64 keyToInsert = key;
		uint32 dib = 1;
		while (_indexes[iterIndex].dib != 0) {
			if (_indexes[iterIndex].dib < dib) {
				const uint64 tmpKey = _indexes[iterIndex].key;
				T tmpValue = std::move(_values[iterIndex]);
				const uint32 tmpDIB = _indexes[iterIndex].dib;
				_indexes[iterIndex].key = keyToInsert;
				_indexes[iterIndex].dib = dib;
				if (_maxDib < dib) {
					_maxDib = dib;
				}
				new (_values + iterIndex) T(std::move(valueToInsert));
				keyToInsert = tmpKey;
				valueToInsert = std::move(tmpValue);
				dib = tmpDIB;
			}
			dib++;
			iterIndex++;
			iterIndex %= _capacity;
		}

		_indexes[iterIndex].key = keyToInsert;
		_indexes[iterIndex].dib = dib;
		if (_maxDib < dib) {
			_maxDib = dib;
		}
		new (_values + iterIndex) T(std::move(valueToInsert));
		_size++;
	}

	template <typename T>
	void UInt64HashMap<T>::remove(uint64 key) {
		uint32 index = _findIndex(key);
		SOUL_ASSERT(0, _indexes[index].key == key && _indexes[index].dib != 0, "Does not found any key : %d in the hash map", key);
		_removeByIndex(index);
	}

	template <typename T>
	T& UInt64HashMap<T>::operator[](uint64 key) {
		uint32 index = _findIndex(key);
		SOUL_ASSERT(0, _indexes[index].key == key && _indexes[index].dib != 0, "");
		return _values[index];
	}

	template <typename T>
	const T& UInt64HashMap<T>::operator[](uint64 key) const {
		uint32 index = _findIndex(key);
		SOUL_ASSERT(0, _indexes[index].key == key && _indexes[index].dib != 0, "");
		return _values[index];
	}

}