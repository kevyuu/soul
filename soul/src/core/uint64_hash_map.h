#pragma once

#include "core/config.h"
#include "core/panic.h"
#include "core/type.h"

namespace soul
{

  template <typename T>
  class UInt64HashMap
  {
  public:
    explicit UInt64HashMap(memory::Allocator* allocator = get_default_allocator())
        : allocator_(allocator)
    {
    }

    UInt64HashMap(const UInt64HashMap& other);
    auto operator=(const UInt64HashMap& other) -> UInt64HashMap&;
    UInt64HashMap(UInt64HashMap&& other) noexcept;
    auto operator=(UInt64HashMap&& other) noexcept -> UInt64HashMap&;
    ~UInt64HashMap() { cleanup(); }

    auto swap(UInt64HashMap<T>& other) noexcept -> void;
    friend auto swap(UInt64HashMap<T>& a, UInt64HashMap<T>& b) noexcept -> void { a.swap(b); }

    auto clear() -> void;
    auto cleanup() -> void;

    auto reserve(soul_size capacity) -> void;
    auto add(uint64 key, const T& value) -> void;
    auto add(uint64 key, T&& value) -> void;

    auto remove(uint64 key) -> void;

    [[nodiscard]]
    auto is_exist(uint64 key) const noexcept -> bool
    {
      if (size_ == 0) {
        return false;
      }
      uint32 index = find_index(key);
      return (indexes_[index].key == key && indexes_[index].dib != 0);
    }

    [[nodiscard]]
    auto
    operator[](uint64 key) -> T&;
    [[nodiscard]]
    auto
    operator[](uint64 key) const -> const T&;

    [[nodiscard]]
    auto size() const noexcept -> soul_size
    {
      return size_;
    }
    [[nodiscard]]
    auto capacity() const -> soul_size
    {
      return capacity_;
    }
    [[nodiscard]]
    auto empty() const noexcept -> bool
    {
      return size_ == 0;
    }

  private:
    memory::Allocator* allocator_ = nullptr;

    struct Index {
      uint64 key;
      soul_size dib;
    };

    Index* indexes_ = nullptr;
    T* values_ = nullptr;

    soul_size size_ = 0;
    soul_size capacity_ = 0;
    soul_size max_dib_ = 0;

    auto init_assert() -> void
    {
      SOUL_ASSERT(0, indexes_ == nullptr, "");
      SOUL_ASSERT(0, values_ == nullptr, "");
      SOUL_ASSERT(0, size_ == 0, "");
      SOUL_ASSERT(0, capacity_ == 0, "");
      SOUL_ASSERT(0, max_dib_ == 0, "");
    }

    auto find_index(uint64 key) const -> soul_size
    {
      const uint32 baseIndex = key % capacity_;
      auto iter_index = baseIndex;
      uint32 dib = 0;
      while ((indexes_[iter_index].key != key) && (indexes_[iter_index].dib != 0) &&
             (dib < max_dib_)) {
        dib++;
        iter_index++;
        iter_index %= capacity_;
      }
      return iter_index;
    }

    auto remove_by_index(soul_size index) -> void
    {
      uint32 next_index = index + 1;
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

    template <typename U = T>
      requires can_trivial_move_v<U>
    auto move_values(const UInt64HashMap<U>& other) -> void
    {
      memcpy(values_, other._values, sizeof(U) * other._capacity);
    }

    template <typename U = T>
      requires can_nontrivial_move_v<U>
    auto move_values(const UInt64HashMap<U>& other) -> void
    {
      for (soul_size i = 0; i < other._capacity; ++i) {
        if (other._indexes[i].dib == 0) {
          continue;
        }
        new (values_ + i) U(std::move(other._values[i]));
      }
    }

    template <typename U = T>
      requires can_trivial_copy_v<U>
    auto copy_values(const UInt64HashMap<T>& other) -> void
    {
      memcpy(values_, other.values_, sizeof(T) * other.capacity_);
    }

    template <typename U = T>
      requires can_nontrivial_copy_v<U>
    auto copy_values(const UInt64HashMap<T>& other) -> void
    {
      for (auto i = 0; i < other.capacity_; i++) {
        if (other.indexes_[i].dib == 0) {
          continue;
        }
        new (values_ + i) T(other.values_[i]);
      }
    }

    template <typename U = T>
    auto destruct_values() -> void
    {
      if constexpr (can_nontrivial_destruct_v<U>) {
        for (auto i = 0; i < capacity_; i++) {
          if (indexes_[i].dib != 0) {
            values_[i].~U();
          }
        }
      }
    }
  };

  template <typename T>
  UInt64HashMap<T>::UInt64HashMap(const UInt64HashMap& other) : allocator_(other.allocator_)
  {
    capacity_ = other.capacity();
    size_ = other.size();
    max_dib_ = other.max_dib_;

    indexes_ = static_cast<Index*>(allocator_->allocate(capacity_ * sizeof(Index), alignof(Index)));
    memcpy(indexes_, other.indexes_, sizeof(Index) * capacity_);

    values_ = static_cast<T*>(allocator_->allocate(capacity_ * sizeof(T), alignof(T)));
    copy_values(other);
  }

  template <typename T>
  auto UInt64HashMap<T>::operator=(const UInt64HashMap& other) -> UInt64HashMap<T>&
  {
    // NOLINT(bugprone-unhandled-self-assignment)
    UInt64HashMap<T>(other).swap(*this);
    return *this;
  }

  template <typename T>
  UInt64HashMap<T>::UInt64HashMap(UInt64HashMap&& other) noexcept
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

  template <typename T>
  auto UInt64HashMap<T>::operator=(UInt64HashMap&& other) noexcept -> UInt64HashMap<T>&
  {
    UInt64HashMap<T>(std::move(other)).swap(*this);
    return *this;
  }

  template <typename T>
  auto UInt64HashMap<T>::swap(UInt64HashMap<T>& other) noexcept -> void
  {
    using std::swap;
    swap(allocator_, other.allocator_);
    swap(indexes_, other.indexes_);
    swap(size_, other.size_);
    swap(capacity_, other.capacity_);
    swap(max_dib_, other.max_dib_);
  }

  template <typename T>
  auto UInt64HashMap<T>::clear() -> void
  {
    destruct_values();
    memset(indexes_, 0, sizeof(Index) * capacity_);
    max_dib_ = 0;
    size_ = 0;
  }

  template <typename T>
  auto UInt64HashMap<T>::cleanup() -> void
  {
    destruct_values();
    allocator_->deallocate(indexes_);
    allocator_->deallocate(values_); // NOLINT(bugprone-sizeof-expression)
    max_dib_ = 0;
    size_ = 0;
    capacity_ = 0;
  }

  template <typename T>
  auto UInt64HashMap<T>::reserve(soul_size capacity) -> void
  {
    SOUL_ASSERT(0, size_ == capacity_, "");
    Index* old_indexes = indexes_;
    indexes_ = static_cast<Index*>(allocator_->allocate(capacity * sizeof(Index), alignof(Index)));
    memset(indexes_, 0, sizeof(Index) * capacity);
    T* old_values = values_;
    values_ = static_cast<T*>(allocator_->allocate(capacity * sizeof(T), alignof(T)));
    const auto old_capacity = capacity_;
    capacity_ = capacity;
    max_dib_ = 0;
    size_ = 0;

    if (old_capacity != 0) {
      SOUL_ASSERT(0, old_indexes != nullptr, "");
      SOUL_ASSERT(0, old_values != nullptr, "");
      for (soul_size i = 0; i < old_capacity; ++i) {
        if (old_indexes[i].dib != 0) {
          add(old_indexes[i].key, std::move(old_values[i]));
        }
      }
      allocator_->deallocate(old_indexes);
      allocator_->deallocate(old_values);
    }
  }

  template <typename T>
  auto UInt64HashMap<T>::add(uint64 key, const T& value) -> void
  {
    T valueToInsert = value;
    add(key, std::move(valueToInsert));
  }

  template <typename T>
  auto UInt64HashMap<T>::add(uint64 key, T&& value) -> void
  {
    if (size_ == capacity_) {
      reserve(capacity_ * 2 + 8);
    }
    const auto base_index = static_cast<uint32>(key % capacity_);
    auto iter_index = base_index;
    T value_to_insert = std::move(value);
    auto key_to_insert = key;
    uint32 dib = 1;
    while (indexes_[iter_index].dib != 0) {
      if (indexes_[iter_index].dib < dib) {
        const uint64 tmpKey = indexes_[iter_index].key;
        T tmpValue = std::move(values_[iter_index]);
        const uint32 tmpDIB = indexes_[iter_index].dib;
        indexes_[iter_index].key = key_to_insert;
        indexes_[iter_index].dib = dib;
        if (max_dib_ < dib) {
          max_dib_ = dib;
        }
        new (values_ + iter_index) T(std::move(value_to_insert));
        key_to_insert = tmpKey;
        value_to_insert = std::move(tmpValue);
        dib = tmpDIB;
      }
      dib++;
      iter_index++;
      iter_index %= capacity_;
    }

    indexes_[iter_index].key = key_to_insert;
    indexes_[iter_index].dib = dib;
    if (max_dib_ < dib) {
      max_dib_ = dib;
    }
    new (values_ + iter_index) T(std::move(value_to_insert));
    size_++;
  }

  template <typename T>
  auto UInt64HashMap<T>::remove(uint64 key) -> void
  {
    uint32 index = find_index(key);
    SOUL_ASSERT(
      0,
      indexes_[index].key == key && indexes_[index].dib != 0,
      "Does not found any key : %d in the hash map",
      key);
    remove_by_index(index);
  }

  template <typename T>
  auto UInt64HashMap<T>::operator[](uint64 key) -> T&
  {
    uint32 index = find_index(key);
    SOUL_ASSERT(0, indexes_[index].key == key && indexes_[index].dib != 0, "");
    return values_[index];
  }

  template <typename T>
  auto UInt64HashMap<T>::operator[](uint64 key) const -> const T&
  {
    uint32 index = find_index(key);
    SOUL_ASSERT(0, indexes_[index].key == key && indexes_[index].dib != 0, "");
    return values_[index];
  }

} // namespace soul
