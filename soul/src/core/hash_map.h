#pragma once

#include "core/config.h"
#include "core/type.h"
#include "core/dev_util.h"
#include "core/math.h"
#include "runtime/runtime.h"

#include <functional>

namespace soul {

	template <typename T>
	concept is_std_hash_implemented_v = std::is_default_constructible_v<std::hash<T>>;

	template<typename T>
	struct DefaultHashOperator
	{
		std::size_t operator()(const T& t) const noexcept
		{
			return hash_fnv1<T>(&t);
		}
	};

	template<is_std_hash_implemented_v T>
	struct DefaultHashOperator<T>
	{
		std::hash<T> hash;
		std::size_t operator()(const T& t) const noexcept
		{
			return hash(t);
		}
	};

	template <
		typename KeyType,
		typename ValType,
		typename Hash=DefaultHashOperator<KeyType>,
		typename KeyEqual=std::equal_to<KeyType>>
	class HashMap {

	public:
		explicit HashMap(memory::Allocator* allocator = GetDefaultAllocator()) : allocator_(allocator) {}

		HashMap(const HashMap& other)
		{
			capacity_ = other.capacity();
			size_ = other.size();
			max_dib_ = other.max_dib_;

			indexes_ = allocator_->allocate_array<Index>(capacity_);
			memcpy(indexes_, other.indexes_, sizeof(Index) * capacity_);

			values_ = allocator_->allocate_array<ValType>(capacity_);
			copy_values(other);
		}

		HashMap& operator=(const HashMap& other)  // NOLINT(bugprone-unhandled-self-assignment) Copy and swap idiom
		{
			HashMap tmp(other);
			tmp.swap(*this);
			return *this;
		}

		HashMap(HashMap&& other) noexcept
		{
			indexes_ = std::move(other.indexes_);
			values_ = std::move(other.values_);
			size_ = std::move(other.size_);
			capacity_ = std::move(other.capacity_);
			max_dib_ = std::move(other.max_dib_);
			allocator_ = std::move(other.allocator_);

			other.indexes_ = nullptr;
			other.values_ = nullptr;
			other.size_ = 0;
			other.capacity_ = 0;
			other.max_dib_ = 0;
		}

		HashMap& operator=(HashMap&& other) noexcept
		{
			HashMap tmp(std::move(other));
			tmp.swap(*this);
			return *this;
		}

		~HashMap()
		{
			destruct_values<ValType>();
			allocator_->deallocate_array(indexes_, capacity_);
			allocator_->deallocate_array(values_, capacity_);
		}

		void swap(HashMap<KeyType, ValType>& other) noexcept
		{
			using std::swap;
			swap(hash_, other.hash_);
			swap(key_equal_, other.key_equal_);
			swap(allocator_, other.allocator_);
			swap(indexes_, other.indexes_);
			swap(values_, other.values_);
			swap(size_, other.size_);
			swap(capacity_, other.capacity_);
			swap(max_dib_, other.max_dib_);
		}

		friend void swap(HashMap& a, HashMap& b) noexcept { a.swap(b); }

		void clear()
		{
			destruct_values<ValType>();
			memset(indexes_, 0, sizeof(Index) * capacity_);
			max_dib_ = 0;
			size_ = 0;
		}

		void cleanup()
		{
			destruct_values<ValType>();
			allocator_->deallocate_array(indexes_, capacity_);
			allocator_->deallocate_array(values_, capacity_);
			max_dib_ = 0;
			size_ = 0;
			capacity_ = 0;
		}

		void reserve(soul_size capacity)
		{
			SOUL_ASSERT(0, size_ == capacity_, "");
			Index* old_indexes = indexes_;
			indexes_ = allocator_->allocate_array<Index>(capacity);
			memset(indexes_, 0, sizeof(Index) * capacity);
			ValType* old_values = values_;
			values_ = allocator_->allocate_array<ValType>(capacity);
			const soul_size old_capacity = capacity_;
			capacity_ = capacity;
			max_dib_ = 0;
			size_ = 0;

			if (old_capacity != 0) {
				SOUL_ASSERT(0, old_indexes != nullptr, "");
				SOUL_ASSERT(0, old_values != nullptr, "");
				for (soul_size i = 0; i < old_capacity; ++i) {
					if (old_indexes[i].dib != 0) {
						insert(old_indexes[i].key, std::move(old_values[i]));
					}
				}
				allocator_->deallocate_array(old_indexes, old_capacity);
				allocator_->deallocate_array(old_values,  old_capacity);
			}
		}

		void insert(const KeyType& key, const ValType& value)
		{
			ValType value_to_insert = value;
			insert(key, std::move(value_to_insert));
		}

		void insert(const KeyType& key, ValType&& value)
		{
			if (size_ == capacity_) {
				reserve((capacity_ * 2) + 8);
			}
			const soul_size base_index = hash_(key) % capacity_;
			soul_size iter_index = base_index;
			ValType value_to_insert = std::move(value);
			KeyType key_to_insert = key;
			soul_size dib = 1;
			while (indexes_[iter_index].dib != 0) {
				if (indexes_[iter_index].dib < dib) {
					KeyType tmp_key = indexes_[iter_index].key;
					ValType tmp_value = std::move(values_[iter_index]);
					const soul_size tmp_dib = indexes_[iter_index].dib;
					indexes_[iter_index].key = key_to_insert;
					indexes_[iter_index].dib = dib;
					if (max_dib_ < dib) {
						max_dib_ = dib;
					}
					new (values_ + iter_index) ValType(std::move(value_to_insert));
					key_to_insert = tmp_key;
					value_to_insert = std::move(tmp_value);
					dib = tmp_dib;
				}
				++dib;
				++iter_index;
				iter_index %= capacity_;
			}

			indexes_[iter_index].key = key_to_insert;
			indexes_[iter_index].dib = dib;
			if (max_dib_ < dib) {
				max_dib_ = dib;
			}
			new (values_ + iter_index) ValType(std::move(value_to_insert));
			size_++;
		}

		void remove(const KeyType& key)
		{
			soul_size index = find_index(key);
			SOUL_ASSERT(0, key_equal_(indexes_[index].key, key) && indexes_[index].dib != 0, "Does not found any key : %d in the hash map", key);
			remove_by_index(index);
		}

		[[nodiscard]] bool contains(const KeyType& key) const {
			if (capacity_ == 0) return false;
			soul_size index = find_index(key);
			return (key_equal_(indexes_[index].key, key) && indexes_[index].dib != 0);
		}

		[[nodiscard]] ValType& operator[](const KeyType& key)
		{
			soul_size index = find_index(key);
			SOUL_ASSERT(0, key_equal_(indexes_[index].key, key) && indexes_[index].dib != 0, "Hashmap key not found");
			return values_[index];
		}

		[[nodiscard]] const ValType& operator[](const KeyType& key) const
		{
			soul_size index = find_index(key);
			SOUL_ASSERT(0, key_equal(indexes_[index].key, key) && indexes_[index].dib != 0, "Hashmap key not found");
			return values_[index];
		}

		[[nodiscard]] soul_size size() const { return size_; }
		[[nodiscard]] soul_size capacity() const { return capacity_; }
		[[nodiscard]] bool empty() const { return size_ == 0; }

	private:
		struct Index {
			KeyType key;
			soul_size dib;
		};

		static constexpr soul_size VALTYPE_SIZE = sizeof(ValType);  // NOLINT(bugprone-sizeof-expression)

		Hash hash_;
		KeyEqual key_equal_;
		memory::Allocator* allocator_ = nullptr;
		Index * indexes_ = nullptr;
		ValType* values_ = nullptr;

		soul_size size_ = 0;
		soul_size capacity_ = 0;
		soul_size max_dib_ = 0;

		void init_assert() {
			SOUL_ASSERT(0, indexes_ == nullptr, "");
			SOUL_ASSERT(0, values_ == nullptr, "");
			SOUL_ASSERT(0, size_ == 0, "");
			SOUL_ASSERT(0, capacity_ == 0, "");
			SOUL_ASSERT(0, max_dib_ == 0, "");
		}

		[[nodiscard]] soul_size find_index(const KeyType& key) const {
			const soul_size base_index = hash_(key) % capacity_;
			soul_size iter_index = base_index;
			soul_size dib = 0;
			while ((!key_equal_(indexes_[iter_index].key, key)) && (indexes_[iter_index].dib != 0) && (dib < max_dib_)) {
				++dib;
				++iter_index;
				iter_index %= capacity_;
			}
			return iter_index;
		}

		void remove_by_index(soul_size index) {
			soul_size next_index = index + 1;
			next_index %= capacity_;
			while(indexes_[next_index].dib > 1) {
				indexes_[index].key = indexes_[next_index].key;
				indexes_[index].dib = indexes_[next_index].dib - 1;
				values_[index] = std::move(values_[next_index]);
				index = (index + 1) % capacity_;
				next_index = (next_index + 1) % capacity_;
			}
			indexes_[index].dib = 0;
			size_--;
		}

		template <typename U = ValType>
		void copy_values(const HashMap<KeyType, ValType>& other) {
			if (std::is_trivially_copyable_v<U>)
			{
				memcpy(values_, other.values_, sizeof(ValType) * other.capacity_);
			} else
			{
				for (soul_size i = 0; i < other.capacity_; ++i) {
					if (other.indexes_[i].dib == 0) continue;
					new (values_ + i) ValType(other.values_[i]);
				}
			}
		}

		template <typename U = ValType>
		void destruct_values() {
			if constexpr (!is_trivially_destructible_v<U>)
			{
				for (soul_size i = 0; i < capacity_; ++i) {
					if (indexes_[i].dib != 0) {
						values_[i].~U();
					}
				}
			}
		}
	};


}