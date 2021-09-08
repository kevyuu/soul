#pragma once

#include "core/config.h"
#include "core/type.h"
#include "core/dev_util.h"
#include "runtime/runtime.h"

namespace Soul {
	
	template <typename KEYTYPE, typename VALTYPE>
	class HashMap {

	public:
		explicit HashMap(Memory::Allocator* allocator = GetDefaultAllocator()) : _allocator(allocator) {}
		HashMap(const HashMap& other);
		HashMap& operator=(const HashMap& other);
		HashMap(HashMap&& other) noexcept;
		HashMap& operator=(HashMap&& other) noexcept;
		~HashMap();

		void swap(HashMap<KEYTYPE, VALTYPE>& other) noexcept;
		friend void swap(HashMap<KEYTYPE, VALTYPE>& a, HashMap<KEYTYPE, VALTYPE>& b) noexcept { a.swap(b); }

		void clear();
		void cleanup();

		void reserve(soul_size capacity);
		void add(const KEYTYPE& key, const VALTYPE& value);
		void add(const KEYTYPE& key, VALTYPE&& value);

		void remove(const KEYTYPE& key);

		SOUL_NODISCARD bool isExist(const KEYTYPE& key) const {
			if (_capacity == 0) return false;
			soul_size index = findIndex(key);
			return (_indexes[index].key == key && _indexes[index].dib != 0);
		}

		SOUL_NODISCARD VALTYPE& operator[](const KEYTYPE& key);
		SOUL_NODISCARD const VALTYPE& operator[](const KEYTYPE& key) const;

		SOUL_NODISCARD soul_size size() const { return _size; }
		SOUL_NODISCARD soul_size capacity() const { return _capacity; }
		SOUL_NODISCARD bool empty() const { return _size == 0; }

	private:
		struct Index {
			KEYTYPE key;
			soul_size dib;
		};

		static constexpr soul_size VALTYPE_SIZE = sizeof(VALTYPE);  // NOLINT(bugprone-sizeof-expression)

		Memory::Allocator* _allocator = nullptr;
		Index * _indexes = nullptr;
		VALTYPE* _values = nullptr;

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

		[[nodiscard]] soul_size findIndex(const KEYTYPE& key) const {
			const soul_size baseIndex = key.hash() % _capacity;
			soul_size iterIndex = baseIndex;
			soul_size dib = 0;
			while ((_indexes[iterIndex].key != key) && (_indexes[iterIndex].dib != 0) && (dib < _maxDib)) {
				++dib;
				++iterIndex;
				iterIndex %= _capacity;
			}
			return iterIndex;
		}

		void removeByIndex(soul_size index) {
			soul_size nextIndex = index + 1;
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
			typename U = VALTYPE,
			SOUL_REQUIRE(is_trivially_copyable_v<U>)>
		void copyValues(const HashMap<KEYTYPE, VALTYPE>& other) {
			memcpy(_values, other._values, sizeof(VALTYPE) * other._capacity);
		}

		template <
			typename U = VALTYPE,
			SOUL_REQUIRE(!is_trivially_copyable_v<U>)>
		void copyValues(const HashMap<KEYTYPE, VALTYPE>& other) {
			for (soul_size i = 0; i < other._capacity; ++i) {
				if (other._indexes[i].dib == 0) continue;
				new (_values + i) VALTYPE(other._values[i]);
			}
		}

		template <typename U = VALTYPE>
		void destructValues() {
			if constexpr (!is_trivially_destructible_v<U>)
			{
				for (soul_size i = 0; i < _capacity; ++i) {
					if (_indexes[i].dib != 0) {
						_values[i].~U();
					}
				}
			}
		}
	};

	template <typename KEYTYPE, typename VALTYPE>
	HashMap<KEYTYPE, VALTYPE>::HashMap(const HashMap& other) :  _allocator(other._allocator) {
		_capacity = other.capacity();
		_size = other.size();
		_maxDib = other._maxDib;

		_indexes = (Index*) _allocator->allocate(_capacity * sizeof(Index), alignof(Index));
		memcpy(_indexes, other._indexes, sizeof(Index) * _capacity);

		_values = (VALTYPE*) _allocator->allocate(_capacity * sizeof(VALTYPE), alignof(VALTYPE));
		copyValues(other);
	}

	template <typename KEYTYPE, typename VALTYPE>
	HashMap<KEYTYPE, VALTYPE>& HashMap<KEYTYPE, VALTYPE>::operator=(const HashMap& other) {  // NOLINT(bugprone-unhandled-self-assignment)
		HashMap<KEYTYPE, VALTYPE>(other).swap(*this);
		return *this;
	}

	template<typename KEYTYPE, typename VALTYPE>
	HashMap<KEYTYPE, VALTYPE>::HashMap(HashMap&& other) noexcept {
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

	template<typename KEYTYPE, typename VALTYPE>
	HashMap<KEYTYPE, VALTYPE>& HashMap<KEYTYPE, VALTYPE>::operator=(HashMap&& other) noexcept {
		HashMap<KEYTYPE, VALTYPE>(std::move(other)).swap(*this);
		return *this;
	}

	template<typename KEYTYPE, typename VALTYPE>
	HashMap<KEYTYPE, VALTYPE>::~HashMap()
	{
		destructValues<VALTYPE>();
		_allocator->deallocate(_indexes, sizeof(Index) * _capacity);
		_allocator->deallocate(_values, VALTYPE_SIZE * _capacity);
	}

	template<typename KEYTYPE, typename VALTYPE>
	void HashMap<KEYTYPE, VALTYPE>::swap(HashMap<KEYTYPE, VALTYPE>& other) noexcept
	{
		using std::swap;
		swap(_allocator, other._allocator);
		swap(_indexes, other._indexes);
		swap(_values, other._values);
		swap(_indexes, other._indexes);
		swap(_values, other._values);
	}

	template <typename KEYTYPE, typename VALTYPE>
	void HashMap<KEYTYPE, VALTYPE>::clear() {
		destructValues<VALTYPE>();
		memset(_indexes, 0, sizeof(Index) * _capacity);
		_maxDib = 0;
		_size = 0;
	}

	template <typename KEYTYPE, typename VALTYPE>
	void HashMap<KEYTYPE, VALTYPE>::cleanup() {
		destructValues<VALTYPE>();
		_allocator->deallocate(_indexes, sizeof(Index) * _capacity);
		_allocator->deallocate(_values, VALTYPE_SIZE * _capacity);
		_maxDib = 0;
		_size = 0;
		_capacity = 0;
	}

	template <typename KEYTYPE, typename VALTYPE>
	void HashMap<KEYTYPE, VALTYPE>::reserve(soul_size capacity) {
		SOUL_ASSERT(0, _size == _capacity, "");
		Index* oldIndexes = _indexes;
		_indexes = (Index*) _allocator->allocate(capacity * sizeof(Index), alignof(Index));
		memset(_indexes, 0, sizeof(Index) * capacity);
		VALTYPE* oldValues = _values;
		_values = (VALTYPE*) _allocator->allocate(capacity * VALTYPE_SIZE, alignof(VALTYPE));
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
			_allocator->deallocate(oldIndexes, sizeof(Index) * oldCapacity);
			_allocator->deallocate(oldValues, VALTYPE_SIZE * oldCapacity);
		}
	}

	template <typename KEYTYPE, typename VALTYPE>
	void HashMap<KEYTYPE, VALTYPE>::add(const KEYTYPE& key, const VALTYPE& value) {
		VALTYPE valueToInsert = value;
		add(key, std::move(valueToInsert));
	}

	template <typename KEYTYPE, typename VALTYPE>
	void HashMap<KEYTYPE, VALTYPE>::add(const KEYTYPE& key, VALTYPE&& value) {
		if (_size == _capacity) {
			reserve((_capacity * 2) + 8);
		}
		const soul_size baseIndex = key.hash() % _capacity;
		soul_size iterIndex = baseIndex;
		VALTYPE valueToInsert = std::move(value);
		KEYTYPE keyToInsert = key;
		soul_size dib = 1;
		while (_indexes[iterIndex].dib != 0) {
			if (_indexes[iterIndex].dib < dib) {
				KEYTYPE tmpKey = _indexes[iterIndex].key;
				VALTYPE tmpValue = std::move(_values[iterIndex]);
				const soul_size tmpDib = _indexes[iterIndex].dib;
				_indexes[iterIndex].key = keyToInsert;
				_indexes[iterIndex].dib = dib;
				if (_maxDib < dib) {
					_maxDib = dib;
				}
				new (_values + iterIndex) VALTYPE(std::move(valueToInsert));
				keyToInsert = tmpKey;
				valueToInsert = std::move(tmpValue);
				dib = tmpDib;
			}
			++dib;
			++iterIndex;
			iterIndex %= _capacity;
		}

		_indexes[iterIndex].key = keyToInsert;
		_indexes[iterIndex].dib = dib;
		if (_maxDib < dib) {
			_maxDib = dib;
		}
		new (_values + iterIndex) VALTYPE(std::move(valueToInsert));
		_size++;
	}

	template <typename KEYTYPE, typename VALTYPE>
	void HashMap<KEYTYPE, VALTYPE>::remove(const KEYTYPE& key) {
		soul_size index = findIndex(key);
		SOUL_ASSERT(0, _indexes[index].key == key && _indexes[index].dib != 0, "Does not found any key : %d in the hash map", key);
		removeByIndex(index);
	}

	template <typename KEYTYPE, typename VALTYPE>
	VALTYPE& HashMap<KEYTYPE, VALTYPE>::operator[](const KEYTYPE& key) {
		soul_size index = findIndex(key);
		SOUL_ASSERT(0, _indexes[index].key == key && _indexes[index].dib != 0, "Hashmap key not found");
		return _values[index];
	}

	template <typename KEYTYPE, typename VALTYPE>
	const VALTYPE& HashMap<KEYTYPE, VALTYPE>::operator[](const KEYTYPE& key) const {
		soul_size index = findIndex(key);
		SOUL_ASSERT(0, _indexes[index].key == key && _indexes[index].dib != 0, "Hashmap key not found");
		return _values[index];
	}

}