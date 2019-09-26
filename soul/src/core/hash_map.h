#pragma once
#include "core/type.h"
#include "core/debug.h"

namespace Soul {

	template <typename Data>
	class HashMap {

	public:

		struct Index {
			uint64 key;
			uint8 dib;
		};
		Index * _indexes;
		Data* _values;

		int32 _size;
		int32 _capacity;
		uint8 _maxDIB;

		HashMap() {
			_indexes = nullptr;
			_values = nullptr;
			_capacity = 0;
			_size = 0;
			_maxDIB = 0;
		}

		~HashMap() {
			SOUL_ASSERT(0, _capacity == 0, "");
			SOUL_ASSERT(0, _size == 0, "");
		}

		void init(int32 capacity) {
			_indexes = (Index*) malloc(sizeof(Index) * capacity);
			memset(_indexes, 0, sizeof(Index) * capacity);

			_values = (Data*) malloc(sizeof(Data) * capacity);
			memset(_values, 0, sizeof(Data) * capacity);

			_capacity = capacity;
		}

		void cleanup() {
			free(_indexes);
			free(_values);
		}

		void add(uint64 key, Data value) {
			SOUL_ASSERT(0, _size < _capacity, "HashMap is full. Please increase the capacity!");
			uint32 baseIndex = (uint32) (key % _capacity);
			uint32 iterIndex = baseIndex;
			Data valueToInsert = value;
			uint64 keyToInsert = key;
			while (_indexes[iterIndex].dib != 0) {
				if (_indexes[iterIndex].dib < (iterIndex - baseIndex) + 1) {
					uint64 tmpKey = _indexes[iterIndex].key;
					Data tmpValue = _values[iterIndex];
					_indexes[iterIndex].key = keyToInsert;
					_indexes[iterIndex].dib = iterIndex - baseIndex + 1;
					_values[iterIndex] = valueToInsert;
					keyToInsert = tmpKey;
					valueToInsert = tmpValue;
					baseIndex = iterIndex;
				}
				iterIndex++;
				iterIndex %= _capacity;
			}

			_indexes[iterIndex].key = keyToInsert;
			_indexes[iterIndex].dib = iterIndex - baseIndex + 1;
			_values[iterIndex] = valueToInsert;
			if (_maxDIB < _indexes[iterIndex].dib) {
				_maxDIB = _indexes[iterIndex].dib;
			}
		}

		void removeByIndex(uint32 index) {
			uint32 nextIndex = index + 1;
			nextIndex %= _capacity;
			while(_indexes[nextIndex].dib > 1) {
				_indexes[index] = _indexes[nextIndex];
				_values[index] = _values[nextIndex];
				index = (index + 1) % _capacity;
				nextIndex = (index + 1) % _capacity;
			}
			_indexes[index].dib = 0;
		}

		uint32 _findIndex(uint64 key) {
			uint32 baseIndex = key % _capacity;
			uint32 iterIndex = baseIndex;
			while ((_indexes[iterIndex].key != key) && (_indexes[iterIndex].dib != 0) && (iterIndex - baseIndex < _maxDIB)) {
				iterIndex++;
			}
			return iterIndex;
		}

		void remove(uint64 key) {
			uint32 index = _findIndex(key);
			SOUL_ASSERT(0, _indexes[index].key == key && _indexes[index].dib != 0, "Does not found any key : %d in the hash map", key);
			removeByIndex(index);
		}

		bool isExist(uint64 key) {
			uint32 index = _findIndex(key);
			return (_indexes[index].key == key && _indexes[index].dib != 0);
		}

		inline Data& operator[](uint64 key) {
			uint32 index = _findIndex(key);
			if (_indexes[index].key == key && _indexes[index].dib != 0) {
				return _values[index];
			}
		}

		inline const Data& get(uint64 key, Data defaultValue) const {
			uint32 index = _findIndex(key);
			if (_indexes[index].key == key && _indexes[index].dib != 0) {
				return _values[index];
			} else {
				return defaultValue;
			}
		}
	};
}