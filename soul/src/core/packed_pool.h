#pragma once

#include "core/type.h"
#include "core/dev_util.h"
#include "core/pool.h"

#include "runtime/runtime.h"

namespace Soul
{
	using PackedID = PoolID;

	template<typename T>
	class PackedPool
	{
	private:
		Memory::Allocator* _allocator;
		Pool<uint32> _internalIndexes;
		PoolID* _poolIDs;
		T* _buffer;
		uint32 _capacity;
		uint32 _size;

	public:
		PackedPool();
		explicit PackedPool(Memory::Allocator* allocator);
		PackedPool(const PackedPool& other);
		PackedPool& operator=(const PackedPool& other);
		PackedPool(PackedPool&& other) noexcept;
		PackedPool& operator=(PackedPool&& other) noexcept;
		~PackedPool();

		void reserve(uint32 capacity);
		PackedID add(const T& datum);
		PackedID add(T&& datum);
		void append(const PackedPool& other);

		inline void remove(PackedID id);

		inline T& operator[](PackedID id) 
		{
			uint32 internalIndex = _internalIndexes[id];
			return _buffer[internalIndex];
		}

		inline const T& get(PackedID id) const
		{
			uint32 internalIndex = _internalIndexes[id];
			return _buffer[internalIndex];
		}

		inline const T& getInternal(uint32 idx) const {
			return _buffer[idx];
		}

		inline T* ptr(PackedID id)
		{
			uint32 internalIndex = _internalIndexes[id];
			return &_buffer[internalIndex];
		}

		inline uint32 size() const { return _size; }
		inline uint32 capacity() const { return _capacity; }

		inline void clear()
		{
			_internalIndexes.clear();
			_size = 0;
		}

		inline void cleanup();

		T* begin() { return _buffer; }
		T* end() { return _buffer + _size; }

		const T* begin() const { return _buffer; }
		const T* end() const { return _buffer + _size; }

	};

	template <typename T>
	PackedPool<T>::PackedPool() :
		_allocator((Memory::Allocator*) Runtime::GetContextAllocator()),
		_internalIndexes(_allocator),
		_poolIDs(nullptr), _buffer(nullptr),
		_size(0), _capacity(0)
		 {}

	template <typename T>
	PackedPool<T>::PackedPool(Memory::Allocator* allocator) :
		_allocator(allocator),
		_internalIndexes(allocator),
		_poolIDs(nullptr), _buffer(nullptr),
		_size(0), _capacity(0)
		{}

	template<typename T>
	PackedPool<T>::PackedPool(const PackedPool& other) : _allocator(other._allocator), _internalIndexes(other._internalIndexes), _capacity(other._capacity), _size(other._size) {
		_poolIDs = (PoolID*) _allocator->allocate(_capacity * sizeof(PoolID), alignof(PoolID));
		memcpy(_poolIDs, other._poolIDs, other._size * sizeof(PoolID));

		_buffer = (T*) _allocator->allocate(_capacity * sizeof(T), alignof(T));
		Copy(other._buffer, other._buffer + other._size, _buffer);
	}

	template<typename T>
	PackedPool<T>& PackedPool<T>::operator=(const PackedPool& other) {
		if (this == &other) return *this;
		cleanup();
		new (this) PackedPool<T>(other);
		return *this;
	}

	template <typename T>
	PackedPool<T>::PackedPool(PackedPool&& other) noexcept {
		_allocator = std::move(other._allocator);
		new (&_internalIndexes) Pool<uint32>(std::move(other._internalIndexes));
		_poolIDs = std::move(other._poolIDs);
		_buffer = std::move(other._buffer);
		_size = std::move(other._size);
		_capacity = std::move(other._capacity);

		other._poolIDs = nullptr;
		other._buffer = nullptr;
		other._size = 0;
		other._capacity = 0;
	}

	template <typename T>
	PackedPool<T>& PackedPool<T>::operator=(PackedPool&& other) noexcept {
		cleanup();
		new (this) PackedPool<T>(std::move(other));
		return *this;
	}

	template <typename T>
	PackedPool<T>::~PackedPool() {
		cleanup();
	}

	template <typename T>
	void PackedPool<T>::reserve(uint32 capacity) {
		SOUL_ASSERT(0, capacity > _capacity, "");

		T* oldBuffer = _buffer;
		_buffer = (T*) _allocator->allocate(capacity * sizeof(T), alignof(T));

		PoolID* oldPoolIDs = _poolIDs;
		_poolIDs = (PoolID*) _allocator->allocate(capacity * sizeof(PoolID), alignof(PoolID));

		if (oldBuffer != nullptr)
		{
			Move(oldBuffer, oldBuffer + _capacity, _buffer);
			_allocator->deallocate(oldBuffer, _capacity * sizeof(T));
			SOUL_ASSERT(0, oldPoolIDs != nullptr, "");
			Move(oldPoolIDs, oldPoolIDs + _capacity, _poolIDs);
			_allocator->deallocate(oldPoolIDs, _capacity * sizeof(PoolID));
		}
		_capacity = capacity;
	}

	template <typename T>
	PackedID PackedPool<T>::add(const T& datum) {
		if (_size == _capacity) {
			reserve(_capacity * 2 + 1);
		}

		new (_buffer + _size) T(datum);
		PackedID id = _internalIndexes.add(_size);
		_poolIDs[_size] = id;
		_size++;

		return id;
	}

	template <typename T>
	PackedID PackedPool<T>::add(T&& datum) {
		if (_size == _capacity) {
			reserve(_capacity * 2 + 1);
		}

		new (_buffer + _size) T(std::move(datum));
		PackedID id = _internalIndexes.add(_size);
		_poolIDs[_size] = id;
		_size++;

		return id;
	}

	template <typename T>
	void PackedPool<T>::append(const PackedPool<T>& other) {
		for (const T& datum : other) {
			add(datum);
		}
	}

	template <typename T>
	void PackedPool<T>::remove(PackedID id) {
		uint32 internalIndex = _internalIndexes[id];
		_buffer[internalIndex] = _buffer[_size - 1];
		_poolIDs[internalIndex] = _poolIDs[_size - 1];
		_internalIndexes[_poolIDs[internalIndex]] = internalIndex;
		_size--;
	}

	template <typename T>
	void PackedPool<T>::cleanup() {
		clear();
		_capacity = 0;

		Destruct(_buffer, _buffer + _size);
		_allocator->deallocate(_buffer, sizeof(T) * _capacity);
		_buffer = nullptr;

		Destruct(_poolIDs, _poolIDs + _size);
		_allocator->deallocate(_poolIDs, sizeof(PoolID) * _capacity);
		_poolIDs = nullptr;

		_internalIndexes.cleanup();
	}

}