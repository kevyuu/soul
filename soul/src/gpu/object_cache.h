#pragma once

#include "core/chunked_sparse_pool.h"
#include "core/hash.h"
#include "core/hash_map.h"
#include "core/intrusive_list.h"
#include "core/mutex.h"

namespace soul::gpu
{

  template <
    typename KeyT,
    typename ValT,
    typename RidT,
    is_shared_lockable_v MutexT = RWSpinMutex,
    typename Hash               = HashOp<KeyT>,
    typename KeyEqual           = std::equal_to<KeyT>>
  class ConcurrentObjectCache
  {
  public:
    explicit ConcurrentObjectCache(memory::Allocator* allocator = get_default_allocator())
        : read_only_map_(allocator),
          fallback_map_(allocator),
          fallback_keys_(allocator),
          object_pool_(allocator)
    {
    }

    ConcurrentObjectCache(const ConcurrentObjectCache&)                    = delete;
    auto operator=(const ConcurrentObjectCache&) -> ConcurrentObjectCache& = delete;
    ConcurrentObjectCache(ConcurrentObjectCache&&)                         = delete;
    auto operator=(ConcurrentObjectCache&&) -> ConcurrentObjectCache&      = delete;
    ~ConcurrentObjectCache()                                               = default;

    auto find(const KeyT& key) -> RidT
    {
      if (read_only_map_.contains(key))
      {
        return read_only_map_[key];
      }
      std::shared_lock lock(mutex_);
      if (fallback_map_.contains(key))
      {
        return fallback_map_[key];
      }
      return RidT::Null();
    }

    template <typename... Args, ts_fn<ValT, Args&&...> Fn>
    auto create(const KeyT& key, Fn func, Args&&... args) -> RidT
    {
      std::unique_lock lock(mutex_);
      if (fallback_map_.contains(key))
      {
        return fallback_map_[key];
      }
      fallback_keys_.push_back(key);
      auto id = object_pool_.create(func(std::forward<Args>(args)...));
      fallback_map_.insert(key, id);
      return id;
    }

    auto on_new_frame() -> void
    {
      for (const KeyT& key : fallback_keys_)
      {
        read_only_map_.insert(key, fallback_map_[key]);
      }
      fallback_keys_.clear();
      fallback_map_.clear();
    }

    auto ref(RidT id) const -> ValT&
    {
      return object_pool_.ref(id);
    }

    auto cref(RidT id) const -> const ValT&
    {
      return object_pool_.cref(id);
    }

  private:
    HashMap<KeyT, RidT> read_only_map_;
    HashMap<KeyT, RidT> fallback_map_;
    Vector<KeyT> fallback_keys_;
    ChunkedSparsePool<ValT, RidT, NullMutex> object_pool_;
    MutexT mutex_;
  };

} // namespace soul::gpu
