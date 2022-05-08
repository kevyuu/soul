#pragma once

#include "core/hash_map.h"
#include "core/mutex.h"
#include "object_pool.h"

namespace soul
{

	namespace gpu
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
			static constexpr ID NULLVAL = nullptr;

			explicit ConcurrentObjectCache(memory::Allocator* allocator = GetDefaultAllocator()) :
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
				ID id = object_pool_.create(func(std::forward<Args>(args)...));
				fallback_map_.insert(key, id);
				return id;
			}

			void flush_to_read_cache()
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
				return id;
			}

		private:
			HashMap<KeyType, ID> read_only_map_;
			HashMap<KeyType, ID> fallback_map_;
			Array<KeyType> fallback_keys_;
			ObjectPool<ValType> object_pool_;
			SharedLockable mutex_;
		};
	}
}