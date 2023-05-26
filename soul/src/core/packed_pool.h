#pragma once

#include "core/compiler.h"
#include "core/config.h"
#include "core/panic.h"
#include "core/pool.h"
#include "core/type.h"

namespace soul
{
  // ReSharper disable once CppInconsistentNaming
  using PackedID = PoolID;

  template <typename T>
  class PackedPool
  {
  public:
    explicit PackedPool(memory::Allocator* allocator = get_default_allocator());
    PackedPool(const PackedPool& other);
    auto operator=(const PackedPool& other) -> PackedPool&;
    PackedPool(PackedPool&& other) noexcept;
    auto operator=(PackedPool&& other) noexcept -> PackedPool&;
    ~PackedPool();

    auto swap(PackedPool& other) noexcept -> void;
    static auto swap(PackedPool& a, PackedPool& b) noexcept -> void { a.swap(b); }

    auto reserve(soul_size capacity) -> void;
    auto add(const T& datum) -> PackedID;
    auto add(T&& datum) -> PackedID;
    auto append(const PackedPool& other) -> void;

    auto remove(PackedID id) -> void;

    [[nodiscard]] auto operator[](PackedID id) -> T&
    {
      auto internal_index = internal_indexes_[id];
      return buffer_[internal_index];
    }

    [[nodiscard]] auto get(PackedID id) const -> const T&
    {
      auto internal_index = internal_indexes_[id];
      return buffer_[internal_index];
    }

    [[nodiscard]] auto get_internal(soul_size idx) const -> const T& { return buffer_[idx]; }

    [[nodiscard]] auto ptr(PackedID id) -> T*
    {
      auto internal_index = internal_indexes_[id];
      return &buffer_[internal_index];
    }

    [[nodiscard]] auto size() const noexcept -> soul_size { return size_; }
    [[nodiscard]] auto capacity() const noexcept -> soul_size { return capacity_; }

    auto clear() -> void
    {
      internal_indexes_.clear();
      size_ = 0;
    }

    auto cleanup() -> void;

    [[nodiscard]] auto begin() noexcept -> T* { return buffer_; }
    [[nodiscard]] auto end() noexcept -> T* { return buffer_ + size_; }

    [[nodiscard]] auto begin() const -> const T* { return buffer_; }
    [[nodiscard]] auto end() const -> const T* { return buffer_ + size_; }

  private:
    using IndexPool = Pool<soul_size>;

    memory::Allocator* allocator_ = nullptr;
    IndexPool internal_indexes_;
    PoolID* pool_ids_ = nullptr;
    T* buffer_ = nullptr;
    soul_size capacity_ = 0;
    soul_size size_ = 0;
  };

  template <typename T>
  PackedPool<T>::PackedPool(memory::Allocator* allocator)
      : allocator_(allocator), internal_indexes_(allocator)
  {
  }

  template <typename T>
  PackedPool<T>::PackedPool(const PackedPool& other)
      : allocator_(other.allocator_),
        internal_indexes_(other.internal_indexes_),
        capacity_(other.capacity_),
        size_(other.size_)
  {
    pool_ids_ =
      static_cast<PoolID*>(allocator_->allocate(capacity_ * sizeof(PoolID), alignof(PoolID)));
    memcpy(pool_ids_, other.pool_ids_, other.size_ * sizeof(PoolID));

    buffer_ = static_cast<T*>(allocator_->allocate(capacity_ * sizeof(T), alignof(T)));
    Copy(other.buffer_, other.buffer_ + other.size_, buffer_);
  }

  template <typename T>
  auto PackedPool<T>::operator=(const PackedPool& other) -> PackedPool<T>&
  {
    // NOLINT(bugprone-unhandled-self-assignment)
    PackedPool<T>(other).swap(*this);
    return *this;
  }

  template <typename T>
  PackedPool<T>::PackedPool(PackedPool&& other) noexcept
      : allocator_(std::move(other.allocator_)),
        pool_ids_(std::exchange(other.pool_ids_, nullptr)),
        buffer_(std::exchange(other.buffer_, nullptr)),
        capacity_(std::exchange(other.capacity_, 0)),
        size_(std::exchange(other.size_, 0))
  {
    new (&internal_indexes_) IndexPool(std::move(other.internal_indexes_));
  }

  template <typename T>
  auto PackedPool<T>::operator=(PackedPool&& other) noexcept -> PackedPool<T>&
  {
    PackedPool<T>(std::move(other)).swap(*this);
    return *this;
  }

  template <typename T>
  PackedPool<T>::~PackedPool()
  {
    cleanup();
  }

  template <typename T>
  auto PackedPool<T>::swap(PackedPool& other) noexcept -> void
  {
    using std::swap;
    swap(allocator_, other.allocator_);
    swap(internal_indexes_, other.internal_indexes_);
    swap(pool_ids_, other.pool_ids_);
    swap(buffer_, other.buffer_);
    swap(capacity_, other.capacity_);
    swap(size_, other.size_);
  }

  template <typename T>
  auto PackedPool<T>::reserve(soul_size capacity) -> void
  {
    SOUL_ASSERT(0, capacity > capacity_, "");

    T* oldBuffer = buffer_;
    buffer_ = static_cast<T*>(allocator_->allocate(capacity * sizeof(T), alignof(T)));

    PoolID* oldPoolIDs = pool_ids_;
    pool_ids_ =
      static_cast<PoolID*>(allocator_->allocate(capacity * sizeof(PoolID), alignof(PoolID)));

    if (oldBuffer != nullptr) {
      Move(oldBuffer, oldBuffer + capacity_, buffer_);
      allocator_->deallocate(oldBuffer);
      SOUL_ASSERT(0, oldPoolIDs != nullptr, "");
      Move(oldPoolIDs, oldPoolIDs + capacity_, pool_ids_);
      allocator_->deallocate(oldPoolIDs);
    }
    capacity_ = capacity;
  }

  template <typename T>
  auto PackedPool<T>::add(const T& datum) -> PackedID
  {
    if (size_ == capacity_) {
      reserve(capacity_ * 2 + 1);
    }

    new (buffer_ + size_) T(datum);
    const auto id = internal_indexes_.create(size_);
    pool_ids_[size_] = id;
    size_++;

    return id;
  }

  template <typename T>
  auto PackedPool<T>::add(T&& datum) -> PackedID
  {
    if (size_ == capacity_) {
      reserve(capacity_ * 2 + 1);
    }
    new (buffer_ + size_) T(std::move(datum));
    const auto id = internal_indexes_.create(size_);
    pool_ids_[size_] = id;
    size_++;

    return id;
  }

  template <typename T>
  auto PackedPool<T>::append(const PackedPool<T>& other) -> void
  {
    for (const T& datum : other) {
      add(datum);
    }
  }

  template <typename T>
  auto PackedPool<T>::remove(PackedID id) -> void
  {
    auto internalIndex = internal_indexes_[id];
    buffer_[internalIndex] = buffer_[size_ - 1];
    pool_ids_[internalIndex] = pool_ids_[size_ - 1];
    internal_indexes_[pool_ids_[internalIndex]] = internalIndex;
    size_--;
  }

  template <typename T>
  auto PackedPool<T>::cleanup() -> void
  {
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

} // namespace soul
