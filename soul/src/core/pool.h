#pragma once

#include "core/bit_vector.h"
#include "core/compiler.h"
#include "core/config.h"
#include "core/panic.h"
#include "core/type.h"
#include "core/type_traits.h"
#include "memory/allocator.h"

namespace soul
{

  using PoolID = u32;

  template <typename T, memory::allocator_type AllocatorType = memory::Allocator>
  class Pool
  {
  public:
    using this_type = Pool<T, AllocatorType>;
    using value_type = T;
    using pointer = T*;
    using const_pointer = T*;
    using reference = T&;
    using const_reference = const T&;
    using iterator = T*;
    using const_iterator = const T*;
    using reverse_iterator = std::reverse_iterator<iterator>;

    using const_reverse_iterator = std::reverse_iterator<const_iterator>;
    using size_type = PoolID;

    explicit Pool(AllocatorType* allocator = get_default_allocator()) noexcept;
    Pool(Pool&& other) noexcept;
    auto operator=(Pool&& other) noexcept -> Pool&;
    ~Pool();

    [[nodiscard]]
    auto clone() -> this_type;

    void clone_from(const this_type& other);

    auto swap(Pool& other) noexcept -> void;
    friend auto swap(Pool& a, Pool& b) noexcept -> void { a.swap(b); }

    auto reserve(size_type capacity) -> void;

    template <typename... Args>
    auto create(Args&&... args) -> PoolID;

    auto remove(PoolID id) -> void;

    [[nodiscard]]
    auto
    operator[](PoolID id) -> reference;
    [[nodiscard]]
    auto
    operator[](PoolID id) const -> const_reference;

    [[nodiscard]]
    auto ptr(PoolID id) -> pointer;
    [[nodiscard]]
    auto ptr(PoolID id) const -> const_pointer;

    [[nodiscard]]
    auto size() const -> size_type
    {
      return size_;
    }
    [[nodiscard]]
    auto empty() const -> bool
    {
      return size_ == 0;
    }

    auto clear() -> void;
    auto cleanup() -> void;

  private:
    Pool(const Pool& other);
    Pool(const Pool& other, AllocatorType& allocator);
    auto operator=(const Pool& other) -> Pool&;

    auto allocate() -> PoolID;

    union Unit {
      T datum;
      size_type next;
      Unit() = delete;
      Unit(const Unit&) = delete;
      Unit(Unit&&) = delete;
      auto operator=(const Unit&) -> Unit& = delete;
      auto operator=(Unit&&) -> Unit& = delete;
      ~Unit() = delete;
    };

    AllocatorType* allocator_;
    BitVector<> bit_vector_;
    Unit* buffer_ = nullptr;
    size_type capacity_ = 0;
    size_type size_ = 0;
    size_type free_list_ = 0;

    auto move_units(Unit* dst) -> void;
    auto duplicate_units(const Pool<T, AllocatorType>& other) -> void;
    auto destruct_units() -> void;

    auto allocate_storage(size_type capacity) -> void;
  };

  template <typename T, memory::allocator_type AllocatorType>
  Pool<T, AllocatorType>::Pool(AllocatorType* allocator) noexcept
      : allocator_(allocator), bit_vector_(allocator_)
  {
  }

  template <typename T, memory::allocator_type AllocatorType>
  Pool<T, AllocatorType>::Pool(const Pool& other)
      : allocator_(other.allocator_),
        bit_vector_(other.bit_vector_),
        buffer_(nullptr),
        size_(other.size_),
        free_list_(other.free_list_)
  {
    allocate_storage(other.capacity_);
    duplicate_units(other);
  }

  template <typename T, memory::allocator_type AllocatorType>
  Pool<T, AllocatorType>::Pool(const Pool& other, AllocatorType& allocator)
      : allocator_(allocator),
        bit_vector_(other.bit_vector_),
        buffer_(nullptr),
        size_(other.size_),
        free_list_(other.free_list_)
  {
    allocate_storage(other.capacity_);
    duplicate_units(other);
  }

  template <typename T, memory::allocator_type AllocatorType>
  auto Pool<T, AllocatorType>::operator=(const Pool& other) -> Pool<T, AllocatorType>&
  {
    // NOLINT(bugprone-unhandled-self-assignment)
    Pool<T, AllocatorType>(other).swap(*this);
    return *this;
  }

  template <typename T, memory::allocator_type AllocatorType>
  Pool<T, AllocatorType>::Pool(Pool&& other) noexcept
      : allocator_(std::move(other.allocator_)),
        bit_vector_(std::move(other.bit_vector_)),
        buffer_(std::exchange(other.buffer_, nullptr)),
        capacity_(std::exchange(other.capacity_, 0)),
        size_(std::exchange(other.size_, 0)),
        free_list_(std::exchange(other.free_list_, 0))
  {
  }

  template <typename T, memory::allocator_type AllocatorType>
  Pool<T, AllocatorType>::~Pool()
  {
    if (allocator_ == nullptr) {
      return;
    }
    cleanup();
  }

  template <typename T, memory::allocator_type AllocatorType>
  auto Pool<T, AllocatorType>::clone() -> this_type
  {
    return Pool(*this);
  }

  template <typename T, memory::allocator_type AllocatorType>
  void Pool<T, AllocatorType>::clone_from(const this_type& other)
  {
    *this = other;
  }

  template <typename T, memory::allocator_type AllocatorType>
  auto Pool<T, AllocatorType>::swap(Pool& other) noexcept -> void
  {
    using std::swap;
    SOUL_ASSERT(0, allocator_ == other.allocator_, "");
    swap(bit_vector_, other.bit_vector_);
    swap(buffer_, other.buffer_);
    swap(capacity_, other.capacity_);
    swap(size_, other.size_);
    swap(free_list_, other.free_list_);
  }

  template <typename T, memory::allocator_type AllocatorType>
  auto Pool<T, AllocatorType>::operator=(Pool&& other) noexcept -> Pool<T, AllocatorType>&
  {
    if (this->allocator_ == other.allocator_) {
      this(std::move(other)).swap(*this);
    } else {
      this_type(other, *allocator_).swap(*this);
    }
    return *this;
  }

  template <typename T, memory::allocator_type AllocatorType>
  auto Pool<T, AllocatorType>::reserve(size_type capacity) -> void
  {
    Unit* new_buffer = allocator_->template allocate_array<Unit>(capacity);
    if (buffer_ != nullptr) {
      move_units(new_buffer);
      destruct_units();
      allocator_->deallocate(buffer_);
    }
    buffer_ = new_buffer;
    for (auto i = capacity_; i < capacity; i++) {
      buffer_[i].next = i + 1;
    }
    free_list_ = size_;
    capacity_ = capacity;

    bit_vector_.resize(capacity_);
  }

  template <typename T, memory::allocator_type AllocatorType>
  auto Pool<T, AllocatorType>::allocate() -> PoolID
  {
    if (size_ == capacity_) {
      reserve(capacity_ * 2 + 8);
    }
    const auto id = free_list_;
    free_list_ = buffer_[free_list_].next;
    size_++;
    return id;
  }

  template <typename T, memory::allocator_type AllocatorType>
  auto Pool<T, AllocatorType>::move_units(Unit* dst) -> void
  {
    if constexpr (ts_copy<T>) {
      memcpy(dst, buffer_, capacity_ * sizeof(Unit));
    } else {
      for (size_type i = 0; i < capacity_; i++) {
        if (bit_vector_[i]) {
          new (dst + i) T(std::move(buffer_[i].datum));
        } else {
          dst[i].next = buffer_[i].next;
        }
      }
    }
  }

  template <typename T, memory::allocator_type AllocatorType>
  auto Pool<T, AllocatorType>::duplicate_units(const Pool<T, AllocatorType>& other) -> void
  {
    if constexpr (ts_copy<T>) {
      memcpy(buffer_, other.buffer_, other.size_ * sizeof(Unit));
    } else if constexpr (ts_clone<T>) {
      for (size_type i = 0; i < capacity_; i++) {
        if (bit_vector_[i]) {
          clone_at(&buffer_[i].datum, other.buffer_[i].datum);
        } else {
          buffer_[i].next = other.buffer_[i].next;
        }
      }
    } else {
      for (size_type i = 0; i < capacity_; i++) {
        if (bit_vector_[i]) {
          new (buffer_ + i) T(other.buffer_[i].datum);
        } else {
          buffer_[i].next = other.buffer_[i].next;
        }
      }
    }
  }

  template <typename T, memory::allocator_type AllocatorType>
  auto Pool<T, AllocatorType>::destruct_units() -> void
  {
    if constexpr (can_nontrivial_destruct_v<T>) {
      for (size_type i = 0; i < capacity_; i++) {
        if (!bit_vector_[i]) {
          continue;
        }
        buffer_[i].datum.~T();
      }
    }
  }

  template <typename T, memory::allocator_type AllocatorType>
  auto Pool<T, AllocatorType>::allocate_storage(size_type capacity) -> void
  {
    buffer_ = allocator_->template allocate_array<Unit>(capacity);
    capacity_ = capacity;
    bit_vector_.resize(capacity_);
  }

  template <typename T, memory::allocator_type AllocatorType>
  template <typename... Args>
  auto Pool<T, AllocatorType>::create(Args&&... args) -> PoolID
  {
    auto id = allocate();
    new (buffer_ + id) T(std::forward<Args>(args)...);
    bit_vector_.set(id);
    return id;
  }

  template <typename T, memory::allocator_type AllocatorType>
  auto Pool<T, AllocatorType>::remove(PoolID id) -> void
  {
    size_--;
    buffer_[id].datum.~T();
    buffer_[id].next = free_list_;
    free_list_ = id;
    bit_vector_[id] = false;
  }

  template <typename T, memory::allocator_type AllocatorType>
  auto Pool<T, AllocatorType>::operator[](PoolID id) -> reference
  {
    SOUL_ASSERT(0, id < capacity_, "Pool access violation");
    return buffer_[id].datum;
  }

  template <typename T, memory::allocator_type AllocatorType>
  auto Pool<T, AllocatorType>::operator[](PoolID id) const -> const_reference
  {
    SOUL_ASSERT(0, id < capacity_, "Pool access violation");
    return buffer_[id].datum;
  }

  template <typename T, memory::allocator_type AllocatorType>
  auto Pool<T, AllocatorType>::ptr(PoolID id) -> pointer
  {
    SOUL_ASSERT(0, id < capacity_, "Pool access violation");
    return &(buffer_[id].datum);
  }

  template <typename T, memory::allocator_type AllocatorType>
  auto Pool<T, AllocatorType>::ptr(PoolID id) const -> const_pointer
  {
    SOUL_ASSERT(0, id < capacity_, "Pool access violation");
    return &(buffer_[id].datum);
  }

  template <typename T, memory::allocator_type AllocatorType>
  auto Pool<T, AllocatorType>::clear() -> void
  {
    destruct_units();
    size_ = 0;
    free_list_ = 0;
    for (u64 i = 0; i < capacity_; i++) {
      buffer_[i].next = i + 1;
    }
    bit_vector_.clear();
  }

  template <typename T, memory::allocator_type AllocatorType>
  auto Pool<T, AllocatorType>::cleanup() -> void
  {
    destruct_units();
    capacity_ = 0;
    size_ = 0;
    allocator_->deallocate(buffer_);
    buffer_ = nullptr;
    bit_vector_.cleanup();
  }

} // namespace soul
