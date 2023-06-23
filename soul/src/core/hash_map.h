#pragma once

#include <functional>

#include "core/config.h"
#include "core/own_ref.h"
#include "core/panic.h"
#include "core/type.h"
#include "core/type_traits.h"
#include "core/util.h"
#include "runtime/runtime.h"

namespace soul
{

  template <typename T>
  concept is_std_hash_implemented_v = can_default_construct_v<std::hash<T>>;

  template <typename T>
  struct DefaultHashOperator {
    auto operator()(const T& t) const noexcept -> std::size_t { return util::hash_fnv1<T>(&t); }
  };

  template <is_std_hash_implemented_v T>
  struct DefaultHashOperator<T> {
    std::hash<T> hash;

    auto operator()(const T& t) const noexcept -> std::size_t { return hash(t); }
  };

  template <
    typename KeyT,
    typename ValT,
    typename Hash = DefaultHashOperator<KeyT>,
    typename KeyEqual = std::equal_to<KeyT>,
    memory::allocator_type AllocatorT = memory::Allocator>
  class HashMap
  {
  public:
    explicit HashMap(AllocatorT* allocator = get_default_allocator());

    HashMap(HashMap&& other) noexcept;

    auto operator=(HashMap&& other) noexcept -> HashMap&;

    ~HashMap();

    [[nodiscard]]
    auto clone() const -> HashMap;

    void clone_from(const HashMap& other) { return *this = other; }

    void swap(HashMap<KeyT, ValT>& other) noexcept;

    friend void swap(HashMap& a, HashMap& b) noexcept { a.swap(b); }

    void clear();

    void cleanup();

    void reserve(soul_size capacity);

    void insert(OwnRef<KeyT, true> key, OwnRef<ValT, true> value);

    void remove(const KeyT& key);

    [[nodiscard]]
    auto contains(const KeyT& key) const -> bool;

    [[nodiscard]]
    auto
    operator[](const KeyT& key) -> ValT&;

    [[nodiscard]]
    auto
    operator[](const KeyT& key) const -> const ValT&;

    [[nodiscard]]
    auto size() const -> soul_size;

    [[nodiscard]]
    auto capacity() const -> soul_size;

    [[nodiscard]]
    auto empty() const -> bool;

  private:
    struct Index {
      KeyT key;
      soul_size dib;
    };

    Hash hash_;
    KeyEqual key_equal_;
    AllocatorT* allocator_;
    Index* indexes_;
    ValT* values_;

    soul_size size_ = 0;
    soul_size capacity_ = 0;
    soul_size max_dib_ = 0;

    HashMap(const HashMap& other, AllocatorT& allocator = *get_default_allocator()) // NOLINT
        : allocator_(&allocator),
          indexes_(allocator_->template allocate_array<Index>(other.capacity())),
          values_(allocator_->template allocate_array<ValT>(other.capacity())),
          size_(other.size()),
          capacity_(other.capacity()),
          max_dib_(other.max_dib_)
    {
      memcpy(indexes_, other.indexes_, sizeof(Index) * capacity_);
      duplicate_values(other);
    }

    auto operator=(const HashMap& other) -> HashMap&
    {
      HashMap tmp(other);
      tmp.swap(*this);
      return *this;
    }

    auto init_assert() -> void
    {
      SOUL_ASSERT(0, indexes_ == nullptr, "");
      SOUL_ASSERT(0, values_ == nullptr, "");
      SOUL_ASSERT(0, size_ == 0, "");
      SOUL_ASSERT(0, capacity_ == 0, "");
      SOUL_ASSERT(0, max_dib_ == 0, "");
    }

    void capacity_unchecked_insert(OwnRef<KeyT, true> key, OwnRef<ValT, true> value)
    {
      const soul_size base_index = hash_(key.const_ref()) % capacity_;
      auto iter_index = base_index;
      soul_size dib = 1;
      while (indexes_[iter_index].dib != 0) {
        if (indexes_[iter_index].dib < dib) {
          key.swap_at(&indexes_[iter_index].key);
          value.swap_at(&values_[iter_index]);
          std::swap(indexes_[iter_index].dib, dib);
          if (max_dib_ < dib) {
            max_dib_ = dib;
          }
        }
        ++dib;
        ++iter_index;
        iter_index %= capacity_;
      }

      key.store_at(&indexes_[iter_index].key);
      value.store_at(&values_[iter_index]);
      indexes_[iter_index].dib = dib;
      if (max_dib_ < dib) {
        max_dib_ = dib;
      }

      size_++;
    }

    [[nodiscard]]
    auto find_index(const KeyT& key) const -> soul_size
    {
      const soul_size base_index = hash_(key) % capacity_;
      auto iter_index = base_index;
      soul_size dib = 0;
      while ((!key_equal_(indexes_[iter_index].key, key)) && (indexes_[iter_index].dib != 0) &&
             (dib < max_dib_)) {
        ++dib;
        ++iter_index;
        iter_index %= capacity_;
      }
      return iter_index;
    }

    void remove_by_index(soul_size index)
    {
      auto next_index = index + 1;
      next_index %= capacity_;
      while (indexes_[next_index].dib > 1) {
        indexes_[index].key = indexes_[next_index].key;
        indexes_[index].dib = indexes_[next_index].dib - 1;
        values_[index] = std::move(values_[next_index]);
        index = (index + 1) % capacity_;
        next_index = (next_index + 1) % capacity_;
      }
      indexes_[index].dib = 0;
      size_--;
    }

    void duplicate_values(const HashMap<KeyT, ValT>& other)
    {
      if constexpr (ts_copy<ValT>) {
        memcpy(values_, other.values_, sizeof(ValT) * other.capacity_);
      } else if constexpr (ts_clone<ValT>) {
        for (soul_size i = 0; i < other.capacity_; ++i) {
          if (other.indexes_[i].dib == 0) {
            continue;
          }
          clone_at(values_ + i, other.values_[i]);
        }
      } else {
        for (soul_size i = 0; i < other.capacity_; ++i) {
          if (other.indexes_[i].dib == 0) {
            continue;
          }
          new (values_ + i) ValT(other.values_[i]);
        }
      }
    }

    void destruct_values()
    {
      if constexpr (can_nontrivial_destruct_v<ValT>) {
        for (soul_size i = 0; i < capacity_; ++i) {
          if (indexes_[i].dib != 0) {
            values_[i].~U();
          }
        }
      }
    }
  };

  template <
    typename KeyT,
    typename ValT,
    typename Hash,
    typename KeyEqual,
    memory::allocator_type AllocatorT>
  HashMap<KeyT, ValT, Hash, KeyEqual, AllocatorT>::HashMap(AllocatorT* allocator)
      : allocator_(allocator), indexes_(nullptr), values_(nullptr)
  {
  }

  template <
    typename KeyT,
    typename ValT,
    typename Hash,
    typename KeyEqual,
    memory::allocator_type AllocatorT>
  HashMap<KeyT, ValT, Hash, KeyEqual, AllocatorT>::HashMap(HashMap&& other) noexcept
      : allocator_(std::move(other.allocator_)),
        indexes_(std::exchange(other.indexes_, nullptr)),
        values_(std::exchange(other.values_, nullptr)),
        size_(std::exchange(other.size_, 0)),
        capacity_(std::exchange(other.capacity_, 0)),
        max_dib_(std::exchange(other.max_dib_, 0))
  {
  }

  template <
    typename KeyT,
    typename ValT,
    typename Hash,
    typename KeyEqual,
    memory::allocator_type AllocatorT>
  auto HashMap<KeyT, ValT, Hash, KeyEqual, AllocatorT>::operator=(HashMap&& other) noexcept
    -> HashMap&
  {
    HashMap tmp(std::move(other));
    tmp.swap(*this);
    return *this;
  }

  template <
    typename KeyT,
    typename ValT,
    typename Hash,
    typename KeyEqual,
    memory::allocator_type AllocatorT>
  HashMap<KeyT, ValT, Hash, KeyEqual, AllocatorT>::~HashMap()
  {
    destruct_values();
    allocator_->deallocate_array(indexes_, capacity_);
    allocator_->deallocate_array(values_, capacity_);
  }

  template <
    typename KeyT,
    typename ValT,
    typename Hash,
    typename KeyEqual,
    memory::allocator_type AllocatorT>
  auto HashMap<KeyT, ValT, Hash, KeyEqual, AllocatorT>::clone() const -> HashMap
  {
    return HashMap(*this);
  }

  template <
    typename KeyT,
    typename ValT,
    typename Hash,
    typename KeyEqual,
    memory::allocator_type AllocatorT>
  void HashMap<KeyT, ValT, Hash, KeyEqual, AllocatorT>::swap(HashMap<KeyT, ValT>& other) noexcept
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

  template <
    typename KeyT,
    typename ValT,
    typename Hash,
    typename KeyEqual,
    memory::allocator_type AllocatorT>
  void HashMap<KeyT, ValT, Hash, KeyEqual, AllocatorT>::clear()
  {
    destruct_values();
    memset(indexes_, 0, sizeof(Index) * capacity_);
    max_dib_ = 0;
    size_ = 0;
  }

  template <
    typename KeyT,
    typename ValT,
    typename Hash,
    typename KeyEqual,
    memory::allocator_type AllocatorT>
  void HashMap<KeyT, ValT, Hash, KeyEqual, AllocatorT>::cleanup()
  {
    destruct_values();
    allocator_->deallocate_array(indexes_, capacity_);
    allocator_->deallocate_array(values_, capacity_);
    max_dib_ = 0;
    size_ = 0;
    capacity_ = 0;
  }

  template <
    typename KeyT,
    typename ValT,
    typename Hash,
    typename KeyEqual,
    memory::allocator_type AllocatorT>
  void HashMap<KeyT, ValT, Hash, KeyEqual, AllocatorT>::reserve(soul_size capacity)
  {
    SOUL_ASSERT(0, size_ == capacity_, "");
    Index* old_indexes = indexes_;
    indexes_ = allocator_->template allocate_array<Index>(capacity);
    memset(indexes_, 0, sizeof(Index) * capacity);
    ValT* old_values = values_;
    values_ = allocator_->template allocate_array<ValT>(capacity);
    const auto old_capacity = capacity_;
    capacity_ = capacity;
    max_dib_ = 0;
    size_ = 0;

    if (old_capacity != 0) {
      SOUL_ASSERT(0, old_indexes != nullptr, "");
      SOUL_ASSERT(0, old_values != nullptr, "");
      for (soul_size i = 0; i < old_capacity; ++i) {
        if (old_indexes[i].dib != 0) {
          capacity_unchecked_insert(std::move(old_indexes[i].key), std::move(old_values[i]));
        }
      }
      allocator_->deallocate_array(old_indexes, old_capacity);
      allocator_->deallocate_array(old_values, old_capacity);
    }
  }

  template <
    typename KeyT,
    typename ValT,
    typename Hash,
    typename KeyEqual,
    memory::allocator_type AllocatorT>
  void HashMap<KeyT, ValT, Hash, KeyEqual, AllocatorT>::insert(
    OwnRef<KeyT, true> key, OwnRef<ValT, true> value)
  {
    if (size_ == capacity_) {
      const auto capacity = capacity_ * 2 + 8;
      Index* old_indexes = indexes_;
      indexes_ = allocator_->template allocate_array<Index>(capacity);
      memset(indexes_, 0, sizeof(Index) * capacity);
      ValT* old_values = values_;
      values_ = allocator_->template allocate_array<ValT>(capacity);
      const auto old_capacity = capacity_;
      capacity_ = capacity;
      max_dib_ = 0;
      size_ = 0;

      if (old_capacity != 0) {
        SOUL_ASSERT(0, old_indexes != nullptr, "");
        SOUL_ASSERT(0, old_values != nullptr, "");
        for (soul_size i = 0; i < old_capacity; ++i) {
          if (old_indexes[i].dib != 0) {
            capacity_unchecked_insert(std::move(old_indexes[i].key), std::move(old_values[i]));
          }
        }
        capacity_unchecked_insert(key.forward(), value.forward());
        allocator_->deallocate_array(old_indexes, old_capacity);
        allocator_->deallocate_array(old_values, old_capacity);
      } else {
        capacity_unchecked_insert(key.forward(), value.forward());
      }
    } else {
      capacity_unchecked_insert(key.forward(), value.forward());
    }
  }

  template <
    typename KeyT,
    typename ValT,
    typename Hash,
    typename KeyEqual,
    memory::allocator_type AllocatorT>
  void HashMap<KeyT, ValT, Hash, KeyEqual, AllocatorT>::remove(const KeyT& key)
  {
    soul_size index = find_index(key);
    SOUL_ASSERT(
      0,
      key_equal_(indexes_[index].key, key) && indexes_[index].dib != 0,
      "Does not found any key : %d in the hash map",
      key);
    remove_by_index(index);
  }

  template <
    typename KeyT,
    typename ValT,
    typename Hash,
    typename KeyEqual,
    memory::allocator_type AllocatorT>
  auto HashMap<KeyT, ValT, Hash, KeyEqual, AllocatorT>::contains(const KeyT& key) const -> bool
  {
    if (capacity_ == 0) {
      return false;
    }
    soul_size index = find_index(key);
    return (key_equal_(indexes_[index].key, key) && indexes_[index].dib != 0);
  }

  template <
    typename KeyT,
    typename ValT,
    typename Hash,
    typename KeyEqual,
    memory::allocator_type AllocatorT>
  auto HashMap<KeyT, ValT, Hash, KeyEqual, AllocatorT>::operator[](const KeyT& key) -> ValT&
  {
    soul_size index = find_index(key);
    SOUL_ASSERT(
      0, key_equal_(indexes_[index].key, key) && indexes_[index].dib != 0, "Hashmap key not found");
    return values_[index];
  }

  template <
    typename KeyT,
    typename ValT,
    typename Hash,
    typename KeyEqual,
    memory::allocator_type AllocatorT>
  auto HashMap<KeyT, ValT, Hash, KeyEqual, AllocatorT>::operator[](const KeyT& key) const
    -> const ValT&
  {
    soul_size index = find_index(key);
    SOUL_ASSERT(
      0, key_equal(indexes_[index].key, key) && indexes_[index].dib != 0, "Hashmap key not found");
    return values_[index];
  }

  template <
    typename KeyT,
    typename ValT,
    typename Hash,
    typename KeyEqual,
    memory::allocator_type AllocatorT>
  auto HashMap<KeyT, ValT, Hash, KeyEqual, AllocatorT>::size() const -> soul_size
  {
    return size_;
  }

  template <
    typename KeyT,
    typename ValT,
    typename Hash,
    typename KeyEqual,
    memory::allocator_type AllocatorT>
  auto HashMap<KeyT, ValT, Hash, KeyEqual, AllocatorT>::capacity() const -> soul_size
  {
    return capacity_;
  }

  template <
    typename KeyT,
    typename ValT,
    typename Hash,
    typename KeyEqual,
    memory::allocator_type AllocatorT>
  auto HashMap<KeyT, ValT, Hash, KeyEqual, AllocatorT>::empty() const -> bool
  {
    return size_ == 0;
  }
} // namespace soul
