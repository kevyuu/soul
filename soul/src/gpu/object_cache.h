#pragma once

#include "core/hash.h"
#include "core/hash_map.h"
#include "core/intrusive_list.h"
#include "core/mutex.h"

#include "object_pool.h"

namespace soul::gpu
{

  template <
    typename KeyType,
    typename ValType,
    is_shared_lockable_v SharedLockable = RWSpinMutex,
    typename Hash = HashOp<KeyType>,
    typename KeyEqual = std::equal_to<KeyType>>
  class ConcurrentObjectCache
  {
  public:
    using ID = typename ObjectPool<ValType>::ID;
    static constexpr ID NULLVAL = ObjectPool<ValType>::NULLVAL;

    explicit ConcurrentObjectCache(memory::Allocator* allocator = get_default_allocator())
        : read_only_map_(allocator),
          fallback_map_(allocator),
          fallback_keys_(allocator),
          object_pool_(allocator)
    {
    }

    ConcurrentObjectCache(const ConcurrentObjectCache&) = delete;
    auto operator=(const ConcurrentObjectCache&) -> ConcurrentObjectCache& = delete;
    ConcurrentObjectCache(ConcurrentObjectCache&&) = delete;
    auto operator=(ConcurrentObjectCache&&) -> ConcurrentObjectCache& = delete;
    ~ConcurrentObjectCache() = default;

    auto find(const KeyType& key) -> ID
    {
      if (read_only_map_.contains(key)) {
        return read_only_map_[key];
      }
      std::shared_lock lock(mutex_);
      if (fallback_map_.contains(key)) {
        return fallback_map_[key];
      }
      return NULLVAL;
    }

    template <typename... Args, ts_fn<ValType, Args&&...> Fn>
    auto create(const KeyType& key, Fn func, Args&&... args) -> ID
    {
      std::unique_lock lock(mutex_);
      if (fallback_map_.contains(key)) {
        return fallback_map_[key];
      }
      fallback_keys_.push_back(key);
      auto id = object_pool_.create(func(std::forward<Args>(args)...));
      fallback_map_.insert(key, id);
      return id;
    }

    auto on_new_frame() -> void
    {
      for (const KeyType& key : fallback_keys_) {
        read_only_map_.insert(key, fallback_map_[key]);
      }
      fallback_keys_.clear();
      fallback_map_.clear();
    }

    auto get(ID id) const -> ValType* { return object_pool_.get(id); }

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
    typename KeyEqual = std::equal_to<KeyType>>
  class RingCache
  {
  public:
    using ID = ValType*;
    static constexpr const ValType* NULLVAL = nullptr;

    explicit RingCache(
      memory::Allocator* allocator = get_default_allocator(), ValDeleter deleter = ValDeleter())
        : map_(allocator), object_pool_(allocator), deleter_(deleter)
    {
    }

    RingCache(const RingCache&) = delete;
    auto operator=(const RingCache&) -> RingCache& = delete;
    RingCache(RingCache&&) = delete;
    auto operator=(RingCache&&) -> RingCache& = delete;

    ~RingCache()
    {
      for (Ring& ring : rings_) {
        for (Item& item : ring) {
          deleter_(item.val);
        }
      }
    }

    template <typename... Args, ts_fn<ValType, Args&&...> Fn>
    auto get_or_create(const KeyType& key, Fn func, Args&&... args) -> ID
    {
      ItemKey item_search_key = get_search_key(key);
      if (map_.contains(item_search_key)) {
        auto item_id = map_[item_search_key];
        Item* item = object_pool_.get(item_id);
        if (item->index != frame_index_) {
          rings_[frame_index_].splice(rings_[frame_index_].begin, *item);
          item->index = frame_index_;
        }
        return &(item->val);
      }

      auto item_id = object_pool_.create(
        Item{key, func(std::forward<Args>(args)...), frame_index_, nullptr, nullptr});
      ItemKey item_persistent_key = get_persistent_key(item_id);
      map_.insert(item_persistent_key, item_id);
      Item* item = object_pool_.get(item_id);
      rings_[frame_index_].push_front(item);
      item->index = frame_index_;
      return &(item->val);
    }

    auto on_new_frame() -> void
    {
      frame_index_ = (frame_index_ + 1) % RingSize;
      usize free_count = 0;
      for (Item& item : rings_[frame_index_]) {
        auto search_key = get_search_key(item.key);
        auto item_id = map_[search_key];
        map_.remove(search_key);
        deleter_(item.val);
        object_pool_.destroy(item_id);
        free_count++;
      }
      rings_[frame_index_].clear();
    }

    auto get(ID id) const -> ValType* { return id; }

  private:
    struct Item : public IntrusiveListNode {
      KeyType key;
      ValType val;
      usize index = 0;
    };

    struct ItemKey {
      std::size_t hash = 0;
      const KeyType* key = nullptr;
      ItemKey() = default;

      ItemKey(std::size_t hash, const KeyType* key) : hash(hash), key(key) {}
    };

    struct ItemKeyHashOperator {
      auto operator()(const ItemKey& item_key) const noexcept -> std::size_t
      {
        if (item_key.key == nullptr) {
          return 0u;
        }
        return item_key.hash;
      }
    };

    struct ItemKeyHashCompareOperator {
      KeyEqual key_equal_op_;

      auto operator()(const ItemKey& item_key1, const ItemKey& item_key2) const noexcept -> b8
      {
        if (item_key1.key == nullptr || item_key2.key == nullptr) {
          return false;
        }
        if (item_key1.hash != item_key2.hash) {
          return false;
        }
        return key_equal_op_(*item_key1.key, *item_key2.key);
      }
    };

    using ItemID = typename ObjectPool<Item>::ID;

    auto get_search_key(const KeyType& key) -> ItemKey { return ItemKey(hash_op_(key), &key); }

    auto get_persistent_key(typename ObjectPool<Item>::ID id) -> ItemKey
    {
      const KeyType* key = &object_pool_.get(id)->key;
      return ItemKey(hash_op_(*key), key);
    }

    HashMap<ItemKey, ItemID, ItemKeyHashOperator, ItemKeyHashCompareOperator> map_;
    ObjectPool<Item> object_pool_;
    using Ring = IntrusiveList<Item>;
    Ring rings_[RingSize];
    usize frame_index_ = 0;
    Hash hash_op_;
    ValDeleter deleter_;
  };

} // namespace soul::gpu
