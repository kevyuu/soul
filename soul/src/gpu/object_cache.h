#pragma once

#include "core/hash_map.h"
#include "core/intrusive_list.h"
#include "core/mutex.h"

#include "object_pool.h"

namespace soul::gpu
{
	template<
		typename KeyType,
		typename ValType,
		is_shared_lockable_v SharedLockable = RWSpinMutex,
		typename Hash = DefaultHashOperator<KeyType>,
		typename KeyEqual = std::equal_to<KeyType>>
	class ConcurrentObjectCache
	{
	public:
		using ID = typename ObjectPool<ValType>::ID;
		static constexpr ID NULLVAL = ObjectPool<ValType>::NULLVAL;

		explicit ConcurrentObjectCache(memory::Allocator* allocator = get_default_allocator()) :
			read_only_map_(allocator), fallback_map_(allocator), fallback_keys_(allocator), object_pool_(allocator) {}
		ConcurrentObjectCache(const ConcurrentObjectCache&) = delete;
		ConcurrentObjectCache& operator=(const ConcurrentObjectCache&) = delete;
		ConcurrentObjectCache(ConcurrentObjectCache&&) = delete;
		ConcurrentObjectCache& operator=(ConcurrentObjectCache&&) = delete;
		~ConcurrentObjectCache() = default;

		ID find(const KeyType& key)
		{
			if (read_only_map_.contains(key))
				return read_only_map_[key];
			std::shared_lock lock(mutex_);
			if (fallback_map_.contains(key))
				return fallback_map_[key];
			return NULLVAL;
		}

		template <typename T, typename... Args>
			requires is_lambda_v<T, ValType(Args...)>
		ID create(const KeyType& key, T func, Args&&... args)
		{
			std::unique_lock lock(mutex_);
			if (fallback_map_.contains(key))
				return fallback_map_[key];
			fallback_keys_.push_back(key);
			auto id = object_pool_.create(func(std::forward<Args>(args)...));
			fallback_map_.insert(key, id);
			return id;
		}

		void on_new_frame()
		{
			for (const KeyType& key : fallback_keys_)
			{
				read_only_map_.insert(key, fallback_map_[key]);
			}
			fallback_keys_.clear();
			fallback_map_.clear();
		}

		ValType* get(ID id) const
		{
			return object_pool_.get(id);
		}

	private:
		HashMap<KeyType, ID> read_only_map_;
		HashMap<KeyType, ID> fallback_map_;
		Vector<KeyType> fallback_keys_;
		ObjectPool<ValType> object_pool_;
		SharedLockable mutex_;
	};

	template <
		typename KeyType,
		typename ValType,
		std::size_t RingSize,
		typename ValDeleter,
		typename Hash = DefaultHashOperator<KeyType>,
		typename KeyEqual = std::equal_to<KeyType>
	>
	class RingCache
	{
	public:
		using ID = ValType*;
		static constexpr ValType* NULLVAL = nullptr;

		explicit RingCache(memory::Allocator* allocator = get_default_allocator(), ValDeleter deleter = ValDeleter()) : map_(allocator), object_pool_(allocator), deleter_(deleter) {}
		RingCache(const RingCache&) = delete;
		RingCache& operator=(const RingCache&) = delete;
		RingCache(RingCache&&) = delete;
		RingCache& operator=(RingCache&&) = delete;
		~RingCache()
		{
			for (Ring& ring : rings_)
			{
				for (Item& item : ring) deleter_(item.val);
			}
		}

		template <typename T, typename... Args>
			requires is_lambda_v<T, ValType(Args...)>
		ID get_or_create(const KeyType& key, T func, Args&&... args)
		{
			ItemKey item_search_key = get_search_key(key);
			if (map_.contains(item_search_key))
			{
				auto item_id = map_[item_search_key];
				Item* item = object_pool_.get(item_id);
				if (item->index != frame_index_)
				{
					rings_[item->index].remove(item);
					rings_[frame_index_].insert(item);
					item->index = frame_index_;
				}
				return &(item->val);
			}
			
			auto item_id = object_pool_.create(Item{ key, func(std::forward<Args>(args)...), frame_index_, nullptr, nullptr});
			ItemKey item_persistent_key = get_persistent_key(item_id);
			map_.insert(item_persistent_key, item_id);
			Item* item = object_pool_.get(item_id);
			rings_[frame_index_].insert(item);
			item->index = frame_index_;
			return &(item->val);
		}

		void on_new_frame()
		{
			frame_index_ = (frame_index_ + 1) % RingSize;
			soul_size free_count = 0;
			for (Item& item : rings_[frame_index_])
			{
				auto search_key = get_search_key(item.key);
				auto item_id = map_[search_key];
				map_.remove(search_key);
				deleter_(item.val);
				object_pool_.destroy(item_id);
				free_count++;
			}
			rings_[frame_index_].clear();
		}

		ValType* get(ID id) const
		{
			return id;
		}

	private:
		struct Item
		{
			KeyType key;
			ValType val;
			soul_size index = 0;
			Item* prev = nullptr;
			Item* next = nullptr;
		};

		struct ItemKey
		{
			std::size_t hash = 0;
			const KeyType* key = nullptr;
			ItemKey() = default;
			ItemKey(std::size_t hash, const KeyType* key) : hash(hash), key(key) {}
		};

		struct ItemKeyHashOperator
		{
			std::size_t operator()(const ItemKey& item_key) const noexcept
			{
				if (item_key.key == nullptr) return false;
				return item_key.hash;
			}
		};

		struct ItemKeyHashCompareOperator
		{
			KeyEqual key_equal_op_;
			bool operator()(const ItemKey& item_key1, const ItemKey& item_key2) const noexcept
			{
				if (item_key1.key == nullptr || item_key2.key == nullptr) return false;
				if (item_key1.hash != item_key2.hash) return false;
				return key_equal_op_(*item_key1.key, *item_key2.key);
			}

		};

		using ItemID = typename ObjectPool<Item>::ID;

		ItemKey get_search_key(const KeyType& key)
		{
			return ItemKey( hash_op_(key), &key );
		}

		ItemKey get_persistent_key(typename ObjectPool<Item>::ID id)
		{
			const KeyType* key = &object_pool_.get(id)->key;
			return ItemKey(hash_op_(*key), key);
		}

		HashMap<ItemKey, ItemID, ItemKeyHashOperator, ItemKeyHashCompareOperator> map_;
		ObjectPool<Item> object_pool_;
		using Ring = IntrusiveList<Item>;
		Ring rings_[RingSize];
		soul_size frame_index_ = 0;
		Hash hash_op_;
		ValDeleter deleter_;
	};
	
}
