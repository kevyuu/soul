#pragma once

#include "core/compiler.h"
#include "core/config.h"
#include "core/panic.h"
#include "core/type.h"

#include "memory/allocator.h"

namespace soul
{

  template <typename T, memory::allocator_type AllocatorT = memory::Allocator>
  class FixedVector
  {
  public:
    using this_type              = FixedVector<T, AllocatorT>;
    using value_type             = T;
    using pointer                = T*;
    using const_pointer          = const T*;
    using reference              = T&;
    using const_reference        = T&;
    using iterator               = T*;
    using const_iterator         = const T*;
    using reverse_iterator       = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    explicit FixedVector(AllocatorT* allocator = get_default_allocator()) : allocator_(allocator) {}

    FixedVector(const FixedVector& other);
    auto operator=(const FixedVector& other) -> FixedVector& = delete;
    FixedVector(FixedVector&& other) noexcept;
    auto operator=(FixedVector&& other) noexcept -> FixedVector& = delete;
    ~FixedVector();

    template <typename... Args>
    auto init(AllocatorT& allocator, usize size, Args&&... args) -> void;

    template <typename... Args>
    auto init(usize size, Args&&... args) -> void;

    template <typename GenerateFn>
    auto init_generate(NotNull<AllocatorT*> allocator, usize size, GenerateFn func) -> void;

    auto cleanup() -> void;

    [[nodiscard]]
    auto data() -> pointer
    {
      return &buffer_[0];
    }

    [[nodiscard]]
    auto data() const -> const_pointer
    {
      return &buffer_[0];
    }

    [[nodiscard]]
    auto
    operator[](usize idx) -> reference;
    [[nodiscard]]
    auto
    operator[](usize idx) const -> const_reference;

    [[nodiscard]]
    auto size() const -> usize
    {
      return size_;
    }

    [[nodiscard]]
    auto begin() -> iterator
    {
      return buffer_;
    }

    [[nodiscard]]
    auto begin() const -> const_iterator
    {
      return buffer_;
    }

    [[nodiscard]]
    auto cbegin() const -> const_iterator
    {
      return buffer_;
    }

    [[nodiscard]]
    auto end() -> iterator
    {
      return buffer_ + size_;
    }

    [[nodiscard]]
    auto end() const -> const_iterator
    {
      return buffer_ + size_;
    }

    [[nodiscard]]
    auto cend() const -> const_iterator
    {
      return buffer_ + size_;
    }

    [[nodiscard]]
    auto rbegin() -> reverse_iterator
    {
      return reverse_iterator(end());
    }

    [[nodiscard]]
    auto rbegin() const -> const_reverse_iterator
    {
      return const_reverse_iterator(cend());
    }

    [[nodiscard]]
    auto crbegin() const -> const_reverse_iterator
    {
      return const_reverse_iterator(cend());
    }

    [[nodiscard]]
    auto rend() -> reverse_iterator
    {
      return reverse_iterator(begin());
    }

    [[nodiscard]]
    auto rend() const -> const_reverse_iterator
    {
      return const_reverse_iterator(cbegin());
    }

    [[nodiscard]]
    auto crend() const -> const_reverse_iterator
    {
      return const_reverse_iterator(cbegin());
    }

  private:
    AllocatorT* allocator_;
    T* buffer_  = nullptr;
    usize size_ = 0;
  };

  template <typename T, memory::allocator_type AllocatorT>
  FixedVector<T, AllocatorT>::FixedVector(const FixedVector<T, AllocatorT>& other)
      : allocator_(other.allocator_), size_(other.size_)
  {
    buffer_ = allocator_->template allocate_array<T>(size_);
    std::uninitialized_copy_n(other.buffer_, other.size_, buffer_);
  }

  template <typename T, memory::allocator_type AllocatorT>
  FixedVector<T, AllocatorT>::FixedVector(FixedVector&& other) noexcept
      : allocator_(std::move(other.allocator_)),
        buffer_(std::move(other.buffer_)),
        size_(std::move(other.size_))
  {
    other.buffer_ = nullptr;
  }

  template <typename T, memory::allocator_type AllocatorT>
  FixedVector<T, AllocatorT>::~FixedVector()
  {
    if (allocator_ == nullptr)
    {
      return;
    }
    cleanup();
  }

  template <typename T, memory::allocator_type AllocatorT>
  template <typename... Args>
  auto FixedVector<T, AllocatorT>::init(AllocatorT& allocator, usize size, Args&&... args) -> void
  {
    SOUL_ASSERT(0, size != 0);
    SOUL_ASSERT(0, size_ == 0, "Array have been initialized before");
    SOUL_ASSERT(0, buffer_ == nullptr, "Array have been initialized before");
    SOUL_ASSERT(0, allocator_ == nullptr);
    allocator_ = &allocator;
    init(size, std::forward<Args>(args)...);
  }

  template <typename T, memory::allocator_type AllocatorT>
  template <typename... Args>
  auto FixedVector<T, AllocatorT>::init(const usize size, Args&&... args) -> void
  {
    SOUL_ASSERT(0, size != 0);
    SOUL_ASSERT(0, size_ == 0, "Array have been initialized before");
    SOUL_ASSERT(0, buffer_ == nullptr, "Array have been initialized before");
    size_   = size;
    buffer_ = allocator_->template allocate_array<T>(size_);
    for (usize i = 0; i < size_; i++)
    {
      new (buffer_ + i) T(std::forward<Args>(args)...);
    }
  }

  template <typename T, memory::allocator_type AllocatorT>
  template <typename GenerateFn>
  auto FixedVector<T, AllocatorT>::init_generate(
    NotNull<AllocatorT*> allocator, const usize size, GenerateFn generate_fn) -> void
  {
    allocator_ = allocator;
    size_      = size;
    buffer_    = allocator_->template allocate_array<T>(size_);
    for (usize i = 0; i < size_; i++)
    {
      new (buffer_ + i) T(generate_fn(i));
    }
  }

  template <typename T, memory::allocator_type AllocatorT>
  auto FixedVector<T, AllocatorT>::cleanup() -> void
  {
    destroy_n(buffer_, size_);
    allocator_->deallocate(buffer_);
    buffer_ = nullptr;
  }

  template <typename T, memory::allocator_type AllocatorT>
  auto FixedVector<T, AllocatorT>::operator[](usize idx) -> reference
  {
    SOUL_ASSERT_UPPER_BOUND_CHECK(idx, size_);
    return buffer_[idx];
  }

  template <typename T, memory::allocator_type AllocatorT>
  auto FixedVector<T, AllocatorT>::operator[](usize idx) const -> const_reference
  {
    SOUL_ASSERT_UPPER_BOUND_CHECK(idx, size_);
    return buffer_[idx];
  }
} // namespace soul
