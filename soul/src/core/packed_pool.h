#pragma once

#include "core/config.h"
#include "core/compiler.h"
#include "core/type.h"
#include "core/dev_util.h"
#include "core/pool.h"

namespace soul
{
	// ReSharper disable once CppInconsistentNaming
	using PackedID = PoolID;

	template<typename T>
	class PackedPool
	{

	public:
		explicit PackedPool(memory::Allocator* allocator = GetDefaultAllocator());
		PackedPool(const PackedPool& other);
		PackedPool& operator=(const PackedPool& other);
		PackedPool(PackedPool&& other) noexcept;
		PackedPool& operator=(PackedPool&& other) noexcept;
		~PackedPool();

		void swap(PackedPool& other) noexcept;
		static void swap(PackedPool& a, PackedPool& b) noexcept { a.swap(b); }

		void reserve(soul_size capacity);
		PackedID add(const T& datum);
		PackedID add(T&& datum);
		void append(const PackedPool& other);

		inline void remove(PackedID id);

		SOUL_NODISCARD T& operator[](PackedID id) 
		{
			soul_size internalIndex = _internalIndexes[id];
			return _buffer[internalIndex];
		}

		SOUL_NODISCARD const T& get(PackedID id) const
		{
			soul_size internalIndex = _internalIndexes[id];
			return _buffer[internalIndex];
		}

		SOUL_NODISCARD const T& getInternal(soul_size idx) const {
			return _buffer[idx];
		}

		SOUL_NODISCARD T* ptr(PackedID id)
		{
			soul_size internalIndex = _internalIndexes[id];
			return &_buffer[internalIndex];
		}

		SOUL_NODISCARD soul_size size() const noexcept { return _size; }
		SOUL_NODISCARD soul_size capacity() const noexcept { return _capacity; }

		inline void clear()
		{
			_internalIndexes.clear();
			_size = 0;
		}

		void cleanup();

		SOUL_NODISCARD T* begin() noexcept { return _buffer; }
		SOUL_NODISCARD T* end() noexcept { return _buffer + _size; }

		SOUL_NODISCARD const T* begin() const { return _buffer; }
		SOUL_NODISCARD const T* end() const { return _buffer + _size; }

	private:

		using IndexPool = Pool<soul_size>;

		memory::Allocator* _allocator = nullptr;
		IndexPool _internalIndexes;
		PoolID* _poolIDs = 0;
		T* _buffer = nullptr;
		soul_size _capacity = 0;
		soul_size _size = 0;

	};

	template <typename T>
	PackedPool<T>::PackedPool(memory::Allocator* allocator) :
		_allocator(allocator),
		_internalIndexes(allocator)
		{}

	template<typename T>
	PackedPool<T>::PackedPool(const PackedPool& other) : _allocator(other._allocator), _internalIndexes(other._internalIndexes), _capacity(other._capacity), _size(other._size) {
		_poolIDs = (PoolID*) _allocator->allocate(_capacity * sizeof(PoolID), alignof(PoolID));
		memcpy(_poolIDs, other._poolIDs, other._size * sizeof(PoolID));

		_buffer = (T*) _allocator->allocate(_capacity * sizeof(T), alignof(T));
		Copy(other._buffer, other._buffer + other._size, _buffer);
	}

	template<typename T>
	PackedPool<T>& PackedPool<T>::operator=(const PackedPool& other) {  // NOLINT(bugprone-unhandled-self-assignment)
		PackedPool<T>(other).swap(*this);
		return *this;
	}

	template <typename T>
	PackedPool<T>::PackedPool(PackedPool&& other) noexcept {
		_allocator = std::move(other._allocator);
		new (&_internalIndexes) IndexPool(std::move(other._internalIndexes));
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
		PackedPool<T>(std::move(other)).swap(*this);
		return *this;
	}

	template <typename T>
	PackedPool<T>::~PackedPool() {
		cleanup();
	}

	template <typename T>
	void PackedPool<T>::swap(PackedPool& other) noexcept {
		using std::swap;
		swap(_allocator, other._allocator);
		swap(_internalIndexes, other._internalIndexes);
		swap(_poolIDs, other._poolIDs);
		swap(_buffer, other._buffer);
		swap(_capacity, other._capacity);
		swap(_size, other._size);
	}

	template <typename T>
	void PackedPool<T>::reserve(soul_size capacity) {
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
		const PackedID id = _internalIndexes.add(_size);
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
		const PackedID id = _internalIndexes.add(_size);
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
		soul_size internalIndex = _internalIndexes[id];
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