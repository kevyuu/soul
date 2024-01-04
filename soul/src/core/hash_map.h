#pragma once

#include "core/config.h"
#include "core/hash.h"
#include "core/own_ref.h"
#include "core/robin_table.h"
#include "core/type.h"
#include "core/type_traits.h"
#include "core/util.h"

#include "runtime/runtime.h"

namespace soul
{
  template <typename KeyT, typename ValueT>
  struct Entry
  {
    KeyT key;
    ValueT value;

    struct GetKeyOp
    {
      auto operator()(const Entry& entry) const -> const KeyT&
      {
        return entry.key;
      }
    };
  };

  template <
    typename KeyT,
    typename ValT,
    typename Hash                     = HashOp<KeyT>,
    typename KeyEqual                 = std::equal_to<KeyT>,
    memory::allocator_type AllocatorT = memory::Allocator>
  class HashMap
  {
  private:
    using HashTableT = RobinTable<
      KeyT,
      Entry<KeyT, ValT>,
      typename Entry<KeyT, ValT>::GetKeyOp,
      Hash,
      RobinTableConfig{.load_factor = 0.5f},
      AllocatorT>;

    HashTableT hash_table_;

    using EntryT = Entry<KeyT, ValT>;

  public:
    explicit HashMap(AllocatorT* allocator = get_default_allocator()) : hash_table_(*allocator) {}

    HashMap(HashMap&& other) noexcept = default;

    auto operator=(HashMap&& other) noexcept -> HashMap& = default;

    ~HashMap() = default;

    [[nodiscard]]
    auto clone() const -> HashMap
    {
      return HashMap(*this);
    }

    void clone_from(const HashMap& other)
    {
      return *this = other;
    }

    void swap(HashMap<KeyT, ValT>& other) noexcept
    {
      using std::swap;
      swap(this->hash_table_, other.hash_table_);
    }

    friend void swap(HashMap& a, HashMap& b) noexcept
    {
      a.swap(b);
    }

    void clear()
    {
      hash_table_.clear();
    }

    void cleanup()
    {
      hash_table_.cleanup();
    }

    void reserve(usize capacity)
    {
      hash_table_.reserve(capacity);
    }

    void insert(OwnRef<KeyT> key, OwnRef<ValT> value)
    {
      return hash_table_.insert(EntryT{.key = std::move(key), .value = std::move(value)});
    }

    template <typename QueryT>
      requires(BorrowTrait<KeyT, QueryT>::available)
    void remove(QueryT key)
    {
      return hash_table_.remove(key);
    }

    void remove(const KeyT& key)
    {
      return hash_table_.remove(key);
    }

    template <typename QueryT>
      requires(BorrowTrait<KeyT, QueryT>::available)
    [[nodiscard]]
    auto contains(QueryT key) const -> b8
    {
      return hash_table_.contains(key);
    }

    [[nodiscard]]
    auto contains(const KeyT& key) const -> b8
    {
      return hash_table_.contains(key);
    }

    template <typename QueryT>
      requires(BorrowTrait<KeyT, QueryT>::available)
    [[nodiscard]]
    auto
    operator[](QueryT key) -> ValT&
    {
      return hash_table_.entry_ref(key).value;
    }

    [[nodiscard]]
    auto
    operator[](const KeyT& key) -> ValT&
    {
      return hash_table_.entry_ref(key).value;
    }

    template <typename QueryT>
      requires(BorrowTrait<KeyT, QueryT>::available)
    [[nodiscard]]
    auto
    operator[](QueryT key) const -> const ValT&
    {
      return hash_table_.entry_ref(key).value;
    }

    [[nodiscard]]
    auto
    operator[](const KeyT& key) const -> const ValT&
    {
      return hash_table_.entry_ref(key).value;
    }

    template <typename QueryT>
      requires(BorrowTrait<KeyT, QueryT>::available)
    [[nodiscard]]
    auto ref(QueryT key) -> ValT&
    {
      return operator[](key);
    }

    [[nodiscard]]
    auto ref(const KeyT& key) -> ValT&
    {
      return operator[](key);
    }

    template <typename QueryT>
      requires(BorrowTrait<KeyT, QueryT>::available)
    [[nodiscard]]
    auto ref(QueryT key) const -> const ValT&
    {
      return operator[](key);
    }

    [[nodiscard]]
    auto ref(const KeyT& key) const -> const ValT&
    {
      return operator[](key);
    }

    [[nodiscard]]
    auto size() const -> usize
    {
      return hash_table_.size();
    }

    [[nodiscard]]
    auto capacity() const -> usize
    {
      return hash_table_.capacity();
    }

    [[nodiscard]]
    auto empty() const -> b8
    {
      return hash_table_.empty();
    }

  private:
    HashMap(const HashMap& other) // NOLINT
        : hash_table_(other.hash_table_)
    {
    }

    auto operator=(const HashMap& other) -> HashMap&
    {
      HashMap tmp(other);
      tmp.swap(*this);
      return *this;
    }
  };

} // namespace soul
