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
		explicit PackedPool(memory::Allocator* allocator = get_default_allocator());
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

		[[nodiscard]] T& operator[](PackedID id) 
		{
			soul_size internalIndex = internal_indexes_[id];
			return buffer_[internalIndex];
		}

		[[nodiscard]] const T& get(PackedID id) const
		{
			soul_size internalIndex = internal_indexes_[id];
			return buffer_[internalIndex];
		}

		[[nodiscard]] const T& getInternal(soul_size idx) const {
			return buffer_[idx];
		}

		[[nodiscard]] T* ptr(PackedID id)
		{
			soul_size internalIndex = internal_indexes_[id];
			return &buffer_[internalIndex];
		}

		[[nodiscard]] soul_size size() const noexcept { return size_; }
		[[nodiscard]] soul_size capacity() const noexcept { return capacity_; }

		inline void clear()
		{
			internal_indexes_.clear();
			size_ = 0;
		}

		void cleanup();

		[[nodiscard]] T* begin() noexcept { return buffer_; }
		[[nodiscard]] T* end() noexcept { return buffer_ + size_; }

		[[nodiscard]] const T* begin() const { return buffer_; }
		[[nodiscard]] const T* end() const { return buffer_ + size_; }

	private:

		using IndexPool = Pool<soul_size>;

		memory::Allocator* allocator_ = nullptr;
		IndexPool internal_indexes_;
		PoolID* pool_ids_ = 0;
		T* buffer_ = nullptr;
		soul_size capacity_ = 0;
		soul_size size_ = 0;

	};

	template <typename T>
	PackedPool<T>::PackedPool(memory::Allocator* allocator) :
		allocator_(allocator),
		internal_indexes_(allocator)
		{}

	template<typename T>
	PackedPool<T>::PackedPool(const PackedPool& other) : allocator_(other.allocator_), internal_indexes_(other.internal_indexes_), capacity_(other.capacity_), size_(other.size_) {
		pool_ids_ = (PoolID*) allocator_->allocate(capacity_ * sizeof(PoolID), alignof(PoolID));
		memcpy(pool_ids_, other.pool_ids_, other.size_ * sizeof(PoolID));

		buffer_ = (T*) allocator_->allocate(capacity_ * sizeof(T), alignof(T));
		Copy(other.buffer_, other.buffer_ + other.size_, buffer_);
	}

	template<typename T>
	PackedPool<T>& PackedPool<T>::operator=(const PackedPool& other) {  // NOLINT(bugprone-unhandled-self-assignment)
		PackedPool<T>(other).swap(*this);
		return *this;
	}

	template <typename T>
	PackedPool<T>::PackedPool(PackedPool&& other) noexcept {
		allocator_ = std::move(other.allocator_);
		new (&internal_indexes_) IndexPool(std::move(other.internal_indexes_));
		pool_ids_ = std::move(other.pool_ids_);
		buffer_ = std::move(other.buffer_);
		size_ = std::move(other.size_);
		capacity_ = std::move(other.capacity_);

		other.pool_ids_ = nullptr;
		other.buffer_ = nullptr;
		other.size_ = 0;
		other.capacity_ = 0;
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
		swap(allocator_, other.allocator_);
		swap(internal_indexes_, other.internal_indexes_);
		swap(pool_ids_, other.pool_ids_);
		swap(buffer_, other.buffer_);
		swap(capacity_, other.capacity_);
		swap(size_, other.size_);
	}

	template <typename T>
	void PackedPool<T>::reserve(soul_size capacity) {
		SOUL_ASSERT(0, capacity > capacity_, "");

		T* oldBuffer = buffer_;
		buffer_ = (T*) allocator_->allocate(capacity * sizeof(T), alignof(T));

		PoolID* oldPoolIDs = pool_ids_;
		pool_ids_ = (PoolID*) allocator_->allocate(capacity * sizeof(PoolID), alignof(PoolID));

		if (oldBuffer != nullptr)
		{
			Move(oldBuffer, oldBuffer + capacity_, buffer_);
			allocator_->deallocate(oldBuffer);
			SOUL_ASSERT(0, oldPoolIDs != nullptr, "");
			Move(oldPoolIDs, oldPoolIDs + capacity_, pool_ids_);
			allocator_->deallocate(oldPoolIDs);
		}
		capacity_ = capacity;
	}

	template <typename T>
	PackedID PackedPool<T>::add(const T& datum) {
		if (size_ == capacity_) {
			reserve(capacity_ * 2 + 1);
		}

		new (buffer_ + size_) T(datum);
		const PackedID id = internal_indexes_.create(size_);
		pool_ids_[size_] = id;
		size_++;

		return id;
	}

	template <typename T>
	PackedID PackedPool<T>::add(T&& datum) {
		if (size_ == capacity_) {
			reserve(capacity_ * 2 + 1);
		}
		new (buffer_ + size_) T(std::move(datum));
		const PackedID id = internal_indexes_.create(size_);
		pool_ids_[size_] = id;
		size_++;

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
		soul_size internalIndex = internal_indexes_[id];
		buffer_[internalIndex] = buffer_[size_ - 1];
		pool_ids_[internalIndex] = pool_ids_[size_ - 1];
		internal_indexes_[pool_ids_[internalIndex]] = internalIndex;
		size_--;
	}

	template <typename T>
	void PackedPool<T>::cleanup() {
		clear();
		capacity_ = 0;

		Destruct(buffer_, buffer_ + size_);
		allocator_->deallocate(buffer_);
		buffer_ = nullptr;

		Destruct(pool_ids_, pool_ids_ + size_);
		allocator_->deallocate(pool_ids_);
		pool_ids_ = nullptr;

		internal_indexes_.cleanup();
	}

}