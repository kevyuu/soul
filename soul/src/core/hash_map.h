#pragma once

#include "core/type.h"
#include "core/dev_util.h"
#include "runtime/runtime.h"

namespace Soul {

	template <typename KEYTYPE, typename VALTYPE>
	class HashMap {

	public:

		HashMap() : _allocator((Memory::Allocator*) Runtime::GetContextAllocator()),
			  _capacity(0), _size(0), _maxDIB(0),
			  _indexes(nullptr), _values(nullptr) {};
		explicit HashMap(Memory::Allocator* allocator) : _allocator(allocator) {};
		HashMap(const HashMap& other);
		HashMap& operator=(const HashMap& other);
		HashMap(HashMap&& other) noexcept;
		HashMap& operator=(HashMap&& other) noexcept;
		~HashMap() {
			cleanup();
		}

		void clear();
		void cleanup();

		void reserve(uint32 capacity);
		void add(KEYTYPE key, const VALTYPE& value);
		void add(KEYTYPE key, VALTYPE&& value);

		void remove(KEYTYPE key);

		inline bool isExist(KEYTYPE key) const {
			if (_capacity == 0) return false;
			uint32 index = _findIndex(key);
			return (_indexes[index].key == key && _indexes[index].dib != 0);
		}

		VALTYPE& operator[](KEYTYPE key);
		const VALTYPE& operator[](KEYTYPE key) const;
		const VALTYPE& get(KEYTYPE key, const VALTYPE& defaultValue) const;

		inline int size() const { return _size; }
		inline int capacity() const { return _capacity; }
		inline bool empty() const { return _size == 0; }

	private:
		struct Index {
			KEYTYPE key;
			uint8 dib;
		};
		Index * _indexes = nullptr;
		VALTYPE* _values = nullptr;

		int32 _size = 0;
		int32 _capacity = 0;
		uint8 _maxDIB = 0;

		Memory::Allocator* _allocator = nullptr;

		inline void _initAssert() {
			SOUL_ASSERT(0, _indexes == nullptr, "");
			SOUL_ASSERT(0, _values == nullptr, "");
			SOUL_ASSERT(0, _size == 0, "");
			SOUL_ASSERT(0, _capacity == 0, "");
			SOUL_ASSERT(0, _maxDIB == 0, "");
		}

		uint32 _findIndex(KEYTYPE key) const {
			uint64 baseIndex = key.hash() % _capacity;
			uint64 iterIndex = baseIndex;
			uint32 dib = 0;
			while ((_indexes[iterIndex].key != key) && (_indexes[iterIndex].dib != 0) && (dib < _maxDIB)) {
				dib++;
				iterIndex++;
				iterIndex %= _capacity;
			}
			return iterIndex;
		}

		void _removeByIndex(uint32 index) {
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

		template <typename U = VALTYPE, typename std::enable_if_t<std::is_trivially_copyable<U>::value>* = nullptr>
		inline void _copyValues(const HashMap<KEYTYPE, VALTYPE>& other) {
			memcpy(_values, other._values, sizeof(VALTYPE) * other._capacity);
		}

		template <typename U = VALTYPE, typename std::enable_if_t<!std::is_trivially_copyable<U>::value>* = nullptr>
		inline void _copyValues(const HashMap<KEYTYPE, VALTYPE>& other) {
			for (int i = 0; i < other._capacity; i++) {
				if (other._indexes[i].dib == 0) continue;
				new (_values + i) VALTYPE(other._values[i]);
			}
		}

		template <typename U = VALTYPE, typename std::enable_if_t<std::is_trivially_destructible<U>::value>* = nullptr>
		inline void _destructValues() {}

		template <typename U = VALTYPE, typename std::enable_if_t<!std::is_trivially_destructible<U>::value>* = nullptr>
		inline void _destructValues() {
			for (int i = 0; i < _capacity; i++) {
				if (_indexes[i].dib != 0) {
					_values[i].~U();
				}
			}
		}
	};

	template <typename KEYTYPE, typename VALTYPE>
	HashMap<KEYTYPE, VALTYPE>::HashMap(const HashMap& other) :  _allocator(other._allocator) {
		_capacity = other.capacity();
		_size = other.size();
		_maxDIB = other._maxDIB;

		_indexes = (Index*) _allocator->allocate(_capacity * sizeof(Index), alignof(Index));
		memcpy(_indexes, other._indexes, sizeof(Index) * _capacity);

		_values = (VALTYPE*) _allocator->allocate(_capacity * sizeof(VALTYPE), alignof(VALTYPE));
		_copyValues(other);
	}

	template <typename KEYTYPE, typename VALTYPE>
	HashMap<KEYTYPE, VALTYPE>& HashMap<KEYTYPE, VALTYPE>::operator=(const HashMap& other) {
		if (this == &other) return *this;
		cleanup();
		new (this) HashMap<KEYTYPE, VALTYPE>(other);
		return *this;
	}

	template<typename KEYTYPE, typename VALTYPE>
	HashMap<KEYTYPE, VALTYPE>::HashMap(HashMap&& other) noexcept {
		_indexes = std::move(other._indexes);
		_values = std::move(other._values);
		_size = std::move(other._size);
		_capacity = std::move(other._capacity);
		_maxDIB = std::move(other._maxDIB);
		_allocator = std::move(other._allocator);

		other._indexes = nullptr;
		other._values = nullptr;
		other._size = 0;
		other._capacity = 0;
		other._maxDIB = 0;
	}

	template<typename KEYTYPE, typename VALTYPE>
	HashMap<KEYTYPE, VALTYPE>& HashMap<KEYTYPE, VALTYPE>::operator=(HashMap&& other) noexcept {

		_destructValues();
		_allocator->deallocate(_indexes, sizeof(Index) * _capacity);
		_allocator->deallocate(_values, sizeof(VALTYPE) * _capacity);

		_indexes = std::move(other._indexes);
		_values = std::move(other._values);
		_size = std::move(other._size);
		_capacity = std::move(other._capacity);
		_maxDIB = std::move(other._maxDIB);
		_allocator = std::move(other._allocator);

		other._indexes = nullptr;
		other._values = nullptr;
		other._size = 0;
		other._capacity = 0;
		other._maxDIB = 0;
	}

	template <typename KEYTYPE, typename VALTYPE>
	void HashMap<KEYTYPE, VALTYPE>::clear() {
		_destructValues();
		memset(_indexes, 0, sizeof(Index) * _capacity);
		_maxDIB = 0;
		_size = 0;
	}

	template <typename KEYTYPE, typename VALTYPE>
	void HashMap<KEYTYPE, VALTYPE>::cleanup() {
		_destructValues();
		_allocator->deallocate(_indexes, sizeof(Index) * _capacity);
		_allocator->deallocate(_values, sizeof(VALTYPE) * _capacity);
		_maxDIB = 0;
		_size = 0;
		_capacity = 0;
	}

	template <typename KEYTYPE, typename VALTYPE>
	void HashMap<KEYTYPE, VALTYPE>::reserve(uint32 capacity) {
		SOUL_ASSERT(0, _size == _capacity, "");
		Index* oldIndexes = _indexes;
		_indexes = (Index*) _allocator->allocate(capacity * sizeof(Index), alignof(Index));
		memset(_indexes, 0, sizeof(Index) * capacity);
		VALTYPE* oldValues = _values;
		_values = (VALTYPE*) _allocator->allocate(capacity * sizeof(VALTYPE), alignof(VALTYPE));
		uint32 oldCapacity = _capacity;
		_capacity = capacity;
		_maxDIB = 0;
		_size = 0;

		if (oldCapacity != 0) {
			SOUL_ASSERT(0, oldIndexes != nullptr, "");
			SOUL_ASSERT(0, oldValues != nullptr, "");
			for (int i = 0; i < oldCapacity; i++) {
				if (oldIndexes[i].dib != 0) {
					add(oldIndexes[i].key, std::move(oldValues[i]));
				}
			}
			_allocator->deallocate(oldIndexes, sizeof(Index) * oldCapacity);
			_allocator->deallocate(oldValues, sizeof(VALTYPE) * oldCapacity);
		}
	}

	template <typename KEYTYPE, typename VALTYPE>
	void HashMap<KEYTYPE, VALTYPE>::add(KEYTYPE key, const VALTYPE& value) {
		VALTYPE valueToInsert = value;
		add(key, std::move(valueToInsert));
	}

	template <typename KEYTYPE, typename VALTYPE>
	void HashMap<KEYTYPE, VALTYPE>::add(KEYTYPE key, VALTYPE&& value) {
		if (_size == _capacity) {
			reserve(_capacity * 2 + 8);
		}
		uint32 baseIndex = uint32(key.hash() % _capacity);
		uint32 iterIndex = baseIndex;
		VALTYPE valueToInsert = std::move(value);
		KEYTYPE keyToInsert = key;
		uint32 dib = 1;
		while (_indexes[iterIndex].dib != 0) {
			if (_indexes[iterIndex].dib < dib) {
				KEYTYPE tmpKey = _indexes[iterIndex].key;
				VALTYPE tmpValue = std::move(_values[iterIndex]);
				uint32 tmpDIB = _indexes[iterIndex].dib;
				_indexes[iterIndex].key = keyToInsert;
				_indexes[iterIndex].dib = dib;
				if (_maxDIB < dib) {
					_maxDIB = dib;
				}
				new (_values + iterIndex) VALTYPE(std::move(valueToInsert));
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
		if (_maxDIB < dib) {
			_maxDIB = dib;
		}
		new (_values + iterIndex) VALTYPE(std::move(valueToInsert));
		_size++;
	}

	template <typename KEYTYPE, typename VALTYPE>
	void HashMap<KEYTYPE, VALTYPE>::remove(KEYTYPE key) {
		uint32 index = _findIndex(key);
		SOUL_ASSERT(0, _indexes[index].key == key && _indexes[index].dib != 0, "Does not found any key : %d in the hash map", key);
		_removeByIndex(index);
	}

	template <typename KEYTYPE, typename VALTYPE>
	VALTYPE& HashMap<KEYTYPE, VALTYPE>::operator[](KEYTYPE key) {
		uint32 index = _findIndex(key);
		if (_indexes[index].key == key && _indexes[index].dib != 0) {
			return _values[index];
		}
		SOUL_PANIC(0, "Key not found");
	}

	template <typename KEYTYPE, typename VALTYPE>
	const VALTYPE& HashMap<KEYTYPE, VALTYPE>::operator[](KEYTYPE key) const {
		uint32 index = _findIndex(key);
		if (_indexes[index].key == key && _indexes[index].dib != 0) {
			return _values[index];
		}
		SOUL_PANIC(0, "Key not found");
	}

	template <typename KEYTYPE, typename VALTYPE>
	const VALTYPE& HashMap<KEYTYPE, VALTYPE>::get(KEYTYPE key, const VALTYPE& defaultValue) const {
		uint32 index = _findIndex(key);
		if (_indexes[index].key == key && _indexes[index].dib != 0) {
			return _values[index];
		} else {
			return defaultValue;
		}
	}

}