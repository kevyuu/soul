#pragma once

#include "core/builtins.h"
#include "core/compiler.h"
#include "core/config.h"
#include "core/log.h"
#include "core/objops.h"
#include "core/panic_lite.h"
#include "memory/allocator.h"

#include <deque>
#include <iterator>
#include <utility>

namespace soul
{

  template <
    typename T,
    memory::allocator_type AllocatorT = memory::Allocator,
    usize InlineSizeV = 0>
  class Deque
  {
  public:
    template <b8 IsConstV>
    class Iterator
    {

    private:
      using ItemItemT = std::conditional_t<IsConstV, T, const T>;

    public:
      using iterator_category = std::bidirectional_iterator_tag;
      using value_type = ItemItemT;
      using reference = ItemItemT&;
      using difference_type = std::ptrdiff_t;
      using pointer = ItemItemT*;

    private:
      ItemItemT* buffer_ = nullptr;
      usize offset_ = 0;
      usize size_ = 0;
      usize capacity_ = 0;

      explicit Iterator(ItemItemT* buffer, usize offset, usize size, usize capacity)
          : buffer_(buffer), offset_(offset), size_(size), capacity_(capacity)
      {
      }

      friend class Deque;

    public:
      explicit Iterator() = default;

      template <b8 IsOtherConstV>
        requires(IsConstV && !IsOtherConstV)
      explicit Iterator(const Iterator<IsOtherConstV>& other)
          : buffer_(other.buffer),
            offset_(other.offset),
            size_(other.size),
            capacity_(other.capacity)
      {
      }

      template <b8 IsOtherConstV>
        requires(IsConstV && !IsOtherConstV)
      auto operator=(const Iterator<IsOtherConstV>& other) -> Iterator&
      {
        buffer_ = other.buffer_;
        offset_ = other.offset_;
        size_ = other.size_;
        capacity_ = other.capacity_;
        return *this;
      }

      auto operator++() -> Iterator&
      {
        offset_++;
        if (SOUL_UNLIKELY(offset_ == capacity_)) {
          offset_ = 0;
        }
        size_--;
        return *this;
      }

      auto operator++(int) -> Iterator
      {
        Iterator iter_copy(buffer_, offset_, size_, capacity_);
        ++*this;
        return iter_copy;
      }

      auto operator--() -> Iterator&
      {
        if (SOUL_UNLIKELY(offset_ == 0)) {
          offset_ = capacity_;
        }
        offset_--;
        size_++;
        return *this;
      }

      auto operator--(int) -> Iterator
      {
        Iterator iter_copy(buffer_, offset_, size_, capacity_);
        --*this;
        return iter_copy;
      }

      template <b8 OtherIsConstV>
      auto operator-(const Iterator<OtherIsConstV>& other) const -> difference_type
      {
        return cast<i64>(other.size_) - cast<i64>(size_);
      }

      auto operator*() const -> reference { return buffer_[offset_]; }

      auto operator->() const -> pointer { return &buffer_[offset_]; }

      template <b8 IsOtherConstV>
      auto operator==(Iterator<IsOtherConstV> const& other) const -> b8
      {
        return buffer_ == other.buffer_ && offset_ == other.offset_ && size_ == other.size_ &&
               capacity_ == other.capacity_;
      }
    };

    using value_type = T;
    using pointer = T*;
    using const_pointer = const T*;
    using reference = T&;
    using const_reference = const T&;
    using iterator = Iterator<false>;
    using const_iterator = Iterator<true>;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    static constexpr usize INLINE_ELEMENT_COUNT = InlineSizeV;

  private:
    SOUL_NO_UNIQUE_ADDRESS RawBuffer<T, InlineSizeV> stack_storage_;
    static constexpr usize GROWTH_FACTOR = 2;
    static constexpr b8 IS_SBO = InlineSizeV > 0;
    AllocatorT* allocator_ = nullptr;
    T* buffer_ = stack_storage_.data();
    usize size_ = 0;
    usize capacity_ = InlineSizeV;
    usize head_idx_ = 0;

  public:
    [[nodiscard]]
    static auto WithCapacity(
      usize capacity, NotNull<AllocatorT*> allocator = get_default_allocator()) -> Deque
    {
      return Deque(Construct::with_capacity, capacity, allocator);
    }

    template <std::ranges::input_range RangeT>
    [[nodiscard]]
    static auto From(RangeT&& range, NotNull<AllocatorT*> allocator = get_default_allocator())
      -> Deque
    {
      return Deque(Construct::from, std::forward<RangeT>(range), allocator);
    }

    explicit Deque(NotNull<AllocatorT*> allocator = get_default_allocator()) : allocator_(allocator)
    {
    }

    Deque(Deque&& other) noexcept
        : allocator_(std::exchange(other.allocator_, nullptr)),
          buffer_(std::exchange(other.buffer_, nullptr)),
          size_(std::exchange(other.size_, 0)),
          capacity_(std::exchange(other.capacity_, 0)),
          head_idx_(std::exchange(other.head_idx_, 0))
    {
    }

    auto operator=(Deque&& other) noexcept -> Deque&
    {
      swap(other);
      other.clear();
      return *this;
    }

    ~Deque() { cleanup(); }

    [[nodiscard]]
    auto clone() const -> Deque
    {
      return Deque(*this);
    }

    void clone_from(const Deque& other) { *this = other; }

    constexpr void swap(Deque& other) noexcept
    {
      using std::swap;

      if (!is_using_stack_storage() && !other.is_using_stack_storage()) {
        SOUL_ASSERT(
          0, allocator_ == other.allocator_, "Cannot swap container with different allocator");
        swap(buffer_, other.buffer_);
        swap(head_idx_, other.head_idx_);
      } else if (is_using_stack_storage() && !other.is_using_stack_storage()) {
        UninitializedRelocate(buffer_, capacity_, head_idx_, size_, other.stack_storage_.data());
        buffer_ = other.buffer_;
        head_idx_ = other.head_idx_;
        other.buffer_ = other.stack_storage_.data();
        other.head_idx_ = 0;
      } else if (!is_using_stack_storage() && other.is_using_stack_storage()) {
        UninitializedRelocate(
          other.buffer_, other.capacity_, other.head_idx_, other.size_, stack_storage_.data());
        other.buffer_ = buffer_;
        other.head_idx_ = head_idx_;
        buffer_ = stack_storage_.data();
        head_idx_ = 0;
      } else {
        RawBuffer<T, InlineSizeV> temp;
        UninitializedRelocate(
          other.buffer_, other.capacity_, other.head_idx_, other.size_, temp.data());
        UninitializedRelocate(buffer_, capacity_, head_idx_, size_, other.buffer_);
        uninitialized_relocate_n(temp.data(), other.size_, buffer_);
        other.head_idx_ = 0;
        head_idx_ = 0;
      }

      swap(size_, other.size_);
      swap(capacity_, other.capacity_);
    }

    friend void swap(Deque& a, Deque& b) noexcept { a.swap(b); }

    void reserve(usize capacity)
    {
      if (capacity < capacity_) {
        return;
      }
      if (!IS_SBO || capacity > InlineSizeV) {
        T* new_buffer = allocator_->template allocate_array<T>(capacity);
        UninitializedRelocate(buffer_, capacity_, head_idx_, size_, new_buffer);
        if (buffer_ != stack_storage_.data()) {
          allocator_->deallocate_array(buffer_, capacity_);
        }
        buffer_ = new_buffer;
        capacity_ = capacity;
        head_idx_ = 0;
      }
    }

    void shrink_to_fit()
    {
      if (capacity_ == size_ || is_using_stack_storage()) {
        return;
      }
      T* old_buffer = buffer_;
      const auto old_capacity = capacity_;
      if (size_ > InlineSizeV) {
        buffer_ = allocator_->template allocate_array<T>(size_);
        capacity_ = size_;
      } else {
        buffer_ = stack_storage_.data();
        capacity_ = InlineSizeV;
      }
      UninitializedRelocate(old_buffer, old_capacity, head_idx_, size_, buffer_);
      head_idx_ = 0;
      allocator_->deallocate_array(old_buffer, old_capacity);
    }

    void push_back(OwnRef<T> item)
    {
      if (size_ == capacity_) {
        const auto new_capacity = get_new_capacity(capacity_);
        T* new_buffer = allocator_->template allocate_array<T>(new_capacity);
        item.store_at(new_buffer + size_);
        UninitializedRelocate(buffer_, capacity_, head_idx_, size_, new_buffer);
        if (buffer_ != stack_storage_.data()) {
          allocator_->deallocate_array(buffer_, capacity_);
        }
        buffer_ = new_buffer;
        capacity_ = new_capacity;
        head_idx_ = 0;
      } else {
        const auto offset = (head_idx_ + size_) % capacity_;
        item.store_at(buffer_ + offset);
      }
      ++size_;
    }

    auto pop_back() -> T
    {
      const auto back_idx = (head_idx_ + size_ - 1) % capacity_;
      T* item_addr = buffer_ + back_idx;
      size_--;
      T item = std::move(*item_addr);
      destroy_at(item_addr);
      return item;
    }

    void push_front(OwnRef<T> item)
    {
      if (size_ == capacity_) {
        const auto new_capacity = get_new_capacity(capacity_);
        T* new_buffer = allocator_->template allocate_array<T>(new_capacity);
        item.store_at(new_buffer + new_capacity - 1);
        UninitializedRelocate(buffer_, capacity_, head_idx_, size_, new_buffer);
        if (buffer_ != stack_storage_.data()) {
          allocator_->deallocate_array(buffer_, capacity_);
        }
        capacity_ = new_capacity;
        buffer_ = new_buffer;
        head_idx_ = new_capacity - 1;
      } else {
        decrement_head_idx();
        item.store_at(buffer_ + head_idx_);
      }
      ++size_;
    }

    auto pop_front() -> T
    {
      const auto idx = head_idx_;
      T* item_addr = buffer_ + idx;
      increment_head_idx();
      size_--;
      T item = std::move(*item_addr);
      destroy_at(item_addr);
      return item;
    }

    [[nodiscard]]
    auto capacity() const -> usize
    {
      return capacity_;
    }

    [[nodiscard]]
    auto size() const -> usize
    {
      return size_;
    }

    [[nodiscard]]
    auto size_in_bytes() const -> usize
    {
      return size_ * sizeof(T);
    }

    [[nodiscard]]
    auto empty() const -> b8
    {
      return size_ == 0;
    }

    [[nodiscard]]
    auto
    operator[](usize idx) -> reference
    {
      return buffer_[idx_offset(idx)];
    }

    [[nodiscard]]
    auto
    operator[](usize idx) const -> const_reference
    {
      return buffer_[idx_offset(idx)];
    }

    [[nodiscard]]
    auto begin() -> iterator
    {
      return iterator(buffer_, head_idx_, size_, capacity_);
    }

    [[nodiscard]]
    auto begin() const -> const_iterator
    {
      return const_iterator(buffer_, head_idx_, size_, capacity_);
    }

    [[nodiscard]]
    auto cbegin() const -> const_iterator
    {
      return const_iterator(buffer_, head_idx_, size_, capacity_);
    }

    [[nodiscard]]
    auto end() -> iterator
    {
      return iterator(buffer_, end_offset(), 0, capacity_);
    }

    [[nodiscard]]
    auto end() const -> const_iterator
    {
      return const_iterator(buffer_, end_offset(), 0, capacity_);
    }

    [[nodiscard]]
    auto cend() const -> const_iterator
    {
      return const_iterator(buffer_, end_offset(), 0, capacity_);
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

    [[nodiscard]]
    auto front_ref() -> reference
    {
      return buffer_[head_idx_];
    }

    [[nodiscard]]
    auto front_ref() const -> const_reference
    {
      return buffer_[head_idx_];
    }

    [[nodiscard]]
    auto back_ref() -> reference
    {
      return buffer_[(head_idx_ + size_ - 1) % capacity_];
    }

    [[nodiscard]]
    auto back_ref() const -> const_reference
    {
      return buffer_[(head_idx_ + size_ - 1) % capacity_];
    }

    void clear()
    {
      const auto size1 = head_idx_ + size_ > capacity_ ? (capacity_ - head_idx_) : size_;
      const auto size2 = size_ - size1;
      std::destroy_n(buffer_ + head_idx_, size1);
      std::destroy_n(buffer_, size2);
      size_ = 0;
      head_idx_ = 0;
    }

    void cleanup()
    {
      clear();
      if (buffer_ != stack_storage_.data()) {
        allocator_->deallocate_array(buffer_, capacity_);
      }
      buffer_ = stack_storage_.data();
      capacity_ = InlineSizeV;
    }

  private:
    Deque(const Deque& other) : size_(other.size_), allocator_(other.allocator_)
    {
      init_reserve(other.capacity_);
      UninitializedDuplicate(other.buffer_, other.capacity_, other.head_idx_, other.size_, buffer_);
    }

    auto operator=(const Deque& other) -> Deque&
    {
      // NOLINT(bugprone-unhandled-self-assignment) use copy and swap idiom
      Deque(other).swap(*this);
      return *this;
    }

    struct Construct {
      struct WithCapacity {
      };
      struct From {
      };

      static constexpr auto with_capacity = WithCapacity{};
      static constexpr auto from = From{};
    };

    Deque(Construct::WithCapacity /*tag*/, usize capacity, NotNull<AllocatorT*> allocator)
        : allocator_(allocator)
    {
      init_reserve(capacity);
    }

    template <std::ranges::input_range RangeT>
    Deque(Construct::From /*tag*/, RangeT&& range, NotNull<AllocatorT*> allocator)
        : allocator_(allocator)
    {
      if constexpr (std::ranges::sized_range<RangeT> || std::ranges::forward_range<RangeT>) {
        const auto size = usize(std::ranges::distance(range));
        init_reserve(size);
        uninitialized_copy_n(std::ranges::begin(range), size, buffer_);
        size_ = size;
      } else {
        for (auto it = std::ranges::begin(range), last = std::ranges::end(range); it != last;
             it++) {
          push_back(*it);
        }
      }
    }

    void init_reserve(usize capacity)
    {
      auto need_heap = true;
      if constexpr (IS_SBO) {
        need_heap = capacity > InlineSizeV;
      }
      if (need_heap) {
        buffer_ = allocator_->template allocate_array<T>(capacity);
        capacity_ = capacity;
      }
    }

    [[nodiscard]]
    auto idx_offset(usize idx) const -> usize
    {
      auto offset = head_idx_ + idx;
      return offset >= capacity_ ? offset - capacity_ : offset;
    }

    [[nodiscard]]
    auto end_offset() const -> usize
    {
      auto offset = head_idx_ + size_;
      return offset >= capacity_ ? offset - capacity_ : offset;
    }

    [[nodiscard]]
    static auto get_new_capacity(usize old_capacity) -> usize
    {
      return old_capacity * GROWTH_FACTOR + 8;
    }

    [[nodiscard]]
    auto is_using_stack_storage() const -> b8
    {
      if constexpr (!IS_SBO) {
        return false;
      }
      return buffer_ == stack_storage_.data();
    }

    static void UninitializedRelocate(T* src, usize capacity, usize head_idx, usize size, T* dst)
    {
      const auto size1 = head_idx + size > capacity ? (capacity - head_idx) : size;
      const auto size2 = size - size1;
      uninitialized_relocate_n(src + head_idx, size1, dst);
      uninitialized_relocate_n(src, size2, dst + size1);
    }

    static void UninitializedDuplicate(T* src, usize capacity, usize head_idx, usize size, T* dst)
    {

      const auto size1 = head_idx + size > capacity ? (capacity - head_idx) : size;
      const auto size2 = size - size1;
      uninitialized_duplicate_n(src + head_idx, size1, dst);
      uninitialized_duplicate_n(src, size2, dst + size1);
    }

    void decrement_head_idx()
    {
      if (head_idx_ == 0) {
        head_idx_ = capacity_ - 1;
      } else {
        head_idx_ -= 1;
      }
    }

    void increment_head_idx() { head_idx_ = head_idx_ + 1 == capacity_ ? 0 : head_idx_ + 1; }
  };
} // namespace soul
