#pragma once

#include "core/type.h"
#include "core/debug.h"
#include "core/pool_array.h"

namespace Soul
{
	typedef PoolID PackedID;

	template<typename T>
	struct PackedArray
	{

		PoolArray<uint32> _internalIndexes;
		PoolID* _poolIDs;
		T* _buffer;
		uint32 _capacity;
		uint32 _size;
		
		PackedArray()
		{
			_poolIDs = nullptr;
			_buffer = nullptr;
			_size = 0;
			_capacity = 0;
		}

		~PackedArray()
		{
			SOUL_ASSERT(0, _internalIndexes.size() == 0);
			SOUL_ASSERT(0, _capacity == 0);
			SOUL_ASSERT(0, _size == 0);
			SOUL_ASSERT(0, _buffer == nullptr);
			SOUL_ASSERT(0, _poolIDs == nullptr);
		}

		inline void reserve(uint32 capacity) 
		{
			SOUL_ASSERT(0, capacity > _capacity);
			
			T* oldBuffer = _buffer;
			_buffer = (T*)malloc(capacity * sizeof(T));
			
			PoolID* oldPoolIDs = _poolIDs;
			_poolIDs = (PoolID*)malloc(capacity * sizeof(PoolID));
			
			if (oldBuffer != nullptr)
			{
				memcpy(_buffer, oldBuffer, _capacity * sizeof(T));
				SOUL_ASSERT(0, oldPoolIDs != nullptr);
				memcpy(_poolIDs, oldPoolIDs, _capacity * sizeof(PoolID));
			}

			_internalIndexes.reserve(capacity);

			_capacity = capacity;
		}

		inline PackedID add(const T& datum)
		{
			if (_size == _capacity)
			{
				reserve(_capacity * 2 + 1);
			}

			_buffer[_size] = datum;
			PackedID id = _internalIndexes.add(_size);
			_poolIDs[_size] = id;
			_size++;

			return id;
		}

		inline void remove(PackedID id) 
		{
			uint32 internalIndex = _internalIndexes[id];
			_buffer[internalIndex] = _buffer[_size - 1];
			_poolIDs[internalIndex] = _poolIDs[_size - 1];
			_internalIndexes[_poolIDs[internalIndex]] = internalIndex;
			_size--;
		}

		inline T& operator[](PackedID id) 
		{
			uint32 internalIndex = _internalIndexes[id];
			return _buffer[internalIndex];
		}

		inline T* ptr(PackedID id)
		{
			uint32 internalIndex = _internalIndexes[id];
			return &_buffer[internalIndex];
		}

		inline uint32 size() const 
		{
			return _size;
		}

		inline void clear()
		{
			_internalIndexes.clear();
			_size = 0;
		}

		inline void cleanup() 
		{
			clear();
			_capacity = 0;

			free(_buffer);
			_buffer = nullptr;

			free(_poolIDs);
			_poolIDs = nullptr;

			_internalIndexes.cleanup();

		}

		struct Iterator
		{
			uint32 _idx;

			Iterator()
			{
				_idx = 0;
			}

			Iterator(uint32 idx)
			{
				_idx = idx;
			}

			inline void next()
			{
				_idx++;
			}

			inline bool operator==(const Iterator& iter)
			{
				return _idx == iter._idx;
			}

			inline bool operator!=(const Iterator& iter)
			{
				return _idx != iter._idx;
			}

		};

		inline Iterator begin()
		{
			return Iterator();
		}

		inline Iterator end()
		{
			return Iterator(_size);
		}

		inline T& iter(const Iterator& iterator)
		{
			return _buffer[iterator._idx];
		}
		
	};
}