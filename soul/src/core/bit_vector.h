#pragma once

#include <iostream>
#include <optional>

#include "core/bit_ref.h"
#include "core/config.h"
#include "core/type.h"
#include "memory/allocator.h"

namespace soul
{

  template <typename IteratorT>
  concept ts_bool_input_iterator =
    std::input_iterator<IteratorT> && std::same_as<std::iter_value_t<IteratorT>, bool>;

  struct BitVectorInitDesc {
    soul_size size = 0;
    bool val = false;
    soul_size capacity = 0;
  };

  template <
    ts_bit_block BlockT = SOUL_BIT_BLOCK_TYPE_DEFAULT,
    memory::allocator_type AllocatorT = memory::Allocator>
  class BitVector
  {
  public:
    using this_type = BitVector<BlockT, AllocatorT>;
    using value_type = bool;
    using reference = BitRef<BlockT>;
    using const_reference = bool;

    explicit BitVector(memory::Allocator* allocator = get_default_allocator());
    explicit BitVector(
      BitVectorInitDesc init_desc, AllocatorT& allocator = *get_default_allocator());
    explicit BitVector(soul_size size, AllocatorT& allocator = *get_default_allocator());
    BitVector(
      soul_size size, const value_type& value, AllocatorT& allocator = *get_default_allocator());
    BitVector(BitVector&& other) noexcept;

    template <ts_bool_input_iterator Iterator>
    BitVector(Iterator first, Iterator last, AllocatorT& allocator = *get_default_allocator());

    auto operator=(BitVector&& other) noexcept -> BitVector&;
    ~BitVector();

    [[nodiscard]]
    auto clone() const -> this_type;

    void clone_from(const this_type& other);

    auto swap(this_type& other) noexcept -> void;
    friend auto swap(this_type& a, this_type& b) noexcept -> void { a.swap(b); }

    [[nodiscard]]
    auto capacity() const noexcept -> soul_size;
    [[nodiscard]]
    auto size() const noexcept -> soul_size;
    [[nodiscard]]
    auto empty() const noexcept -> bool;

    auto set_allocator(AllocatorT& allocator) -> void;
    [[nodiscard]]
    auto get_allocator() const noexcept -> AllocatorT*;

    auto resize(soul_size size) -> void;
    auto reserve(soul_size capacity) -> void;

    auto clear() noexcept -> void;
    auto cleanup() -> void;

    auto push_back(bool val) -> void;
    auto push_back() -> reference;
    auto pop_back() -> void;
    auto pop_back(soul_size count) -> void;

    [[nodiscard]]
    auto front() noexcept -> reference;
    [[nodiscard]]
    auto front() const noexcept -> const_reference;

    [[nodiscard]]
    auto back() noexcept -> reference;
    [[nodiscard]]
    auto back() const noexcept -> const_reference;

    [[nodiscard]]
    auto
    operator[](soul_size index) -> reference;
    [[nodiscard]]
    auto
    operator[](soul_size index) const -> const_reference;

    [[nodiscard]]
    auto test(soul_size index, bool default_value) const -> bool;

    auto set(soul_size index, bool value = true) -> void;
    auto set() -> void;
    auto reset() -> void;

  private:
    AllocatorT* allocator_;
    BlockT* blocks_ = nullptr;
    soul_size size_ = 0;
    soul_size capacity_ = 0;

    static constexpr soul_size BLOCK_BIT_COUNT = sizeof(BlockT) * 8;
    static constexpr soul_size GROWTH_FACTOR = 2;

    BitVector(const BitVector& other);
    BitVector(const BitVector& other, AllocatorT& allocator);
    auto operator=(const BitVector& other) -> BitVector&;

    auto get_bit_ref(soul_size index) -> reference;
    [[nodiscard]]
    auto get_bool(soul_size index) const -> bool;
    static auto get_new_capacity(soul_size old_capacity) -> soul_size;
    auto init_reserve(soul_size capacity) -> void;
    auto init_resize(soul_size size, bool val) -> void;
    [[nodiscard]]
    static auto get_block_count(soul_size size) -> soul_size;
    [[nodiscard]]
    static auto get_block_index(soul_size index) -> soul_size;
    [[nodiscard]]
    static auto get_block_offset(soul_size index) -> soul_size;
  };

  template <ts_bit_block BlockT, memory::allocator_type AllocatorT>
  BitVector<BlockT, AllocatorT>::BitVector(memory::Allocator* allocator) : allocator_(allocator)
  {
  }

  template <ts_bit_block BlockT, memory::allocator_type AllocatorT>
  BitVector<BlockT, AllocatorT>::BitVector(BitVectorInitDesc init_desc, AllocatorT& allocator)
      : allocator_(&allocator), size_(init_desc.size)
  {
    const auto capacity = init_desc.size > init_desc.capacity ? init_desc.size : init_desc.capacity;
    const auto block_count = get_block_count(capacity);
    blocks_ = allocator_->template allocate_array<BlockT>(block_count);
    capacity_ = block_count * BLOCK_BIT_COUNT;
    std::uninitialized_fill_n(
      blocks_, get_block_count(init_desc.size), init_desc.val ? ~BlockT(0) : BlockT(0));
  }

  template <ts_bit_block BlockT, memory::allocator_type AllocatorT>
  BitVector<BlockT, AllocatorT>::BitVector(const soul_size size, AllocatorT& allocator)
      : allocator_(&allocator)
  {
    init_resize(size, false);
  }

  template <ts_bit_block BlockT, memory::allocator_type AllocatorT>
  BitVector<BlockT, AllocatorT>::BitVector(
    const soul_size size, const value_type& value, AllocatorT& allocator)
      : allocator_(&allocator)
  {
    init_resize(size, value);
  }

  template <ts_bit_block BlockT, memory::allocator_type AllocatorT>
  BitVector<BlockT, AllocatorT>::BitVector(const BitVector& other)
      : allocator_(other.allocator_), size_(other.size_)
  {
    init_reserve(other.size_);
    std::uninitialized_copy_n(other.blocks_, get_block_count(other.size_), blocks_);
  }

  template <ts_bit_block BlockT, memory::allocator_type AllocatorT>
  BitVector<BlockT, AllocatorT>::BitVector(const BitVector& other, AllocatorT& allocator)
      : allocator_(&allocator), size_(other.size_)
  {
    init_reserve(other.size_);
    std::uninitialized_copy_n(other.blocks_, get_block_count(other.size_), blocks_);
  }

  template <ts_bit_block BlockT, memory::allocator_type AllocatorT>
  BitVector<BlockT, AllocatorT>::BitVector(BitVector&& other) noexcept
      : allocator_(std::exchange(other.allocator_, nullptr)),
        blocks_(std::exchange(other.blocks_, nullptr)),
        size_(std::exchange(other.size_, 0)),
        capacity_(std::exchange(other.capacity_, 0))
  {
  }

  template <ts_bit_block BlockT, memory::allocator_type AllocatorT>
  template <ts_bool_input_iterator Iterator>
  BitVector<BlockT, AllocatorT>::BitVector(Iterator first, Iterator last, AllocatorT& allocator)
      : allocator_(&allocator)
  {
    if constexpr (std::forward_iterator<Iterator>) {
      init_reserve(std::distance(first, last));
    }
    for (; first != last; ++first) {
      push_back(*first);
    }
  }

  template <ts_bit_block BlockT, memory::allocator_type AllocatorT>
  auto BitVector<BlockT, AllocatorT>::operator=(const BitVector& other)
    -> BitVector<BlockT, AllocatorT>&
  // NOLINT(bugprone-unhandled-self-assignment)

  {
    this_type(other, *allocator_).swap(*this);
    return *this;
  }

  template <ts_bit_block BlockT, memory::allocator_type AllocatorT>
  auto BitVector<BlockT, AllocatorT>::operator=(BitVector&& other) noexcept
    -> BitVector<BlockT, AllocatorT>&
  {
    if (this->allocator_ == other.allocator_) {
      this_type(std::move(other)).swap(*this);
    } else {
      this_type(other, *allocator_).swap(*this);
    }
    return *this;
  }

  template <ts_bit_block BlockT, memory::allocator_type AllocatorT>
  BitVector<BlockT, AllocatorT>::~BitVector()
  {
    SOUL_ASSERT(0, allocator_ != nullptr || blocks_ == nullptr, "");
    if (allocator_ == nullptr) {
      return;
    }
    cleanup();
  }

  template <ts_bit_block BlockT, memory::allocator_type AllocatorT>
  [[nodiscard]]
  auto BitVector<BlockT, AllocatorT>::clone() const -> this_type
  {
    return this_type(*this);
  }

  template <ts_bit_block BlockT, memory::allocator_type AllocatorT>
  void BitVector<BlockT, AllocatorT>::clone_from(const this_type& other)
  {
    return *this = other;
  }

  template <ts_bit_block BlockT, memory::allocator_type AllocatorT>
  auto BitVector<BlockT, AllocatorT>::swap(this_type& other) noexcept -> void
  {
    using std::swap;
    SOUL_ASSERT(
      0, allocator_ == other.allocator_, "Cannot swap container with different allocators");
    swap(blocks_, other.blocks_);
    swap(size_, other.size_);
    swap(capacity_, other.capacity_);
  }

  template <ts_bit_block BlockT, memory::allocator_type AllocatorT>
  auto BitVector<BlockT, AllocatorT>::capacity() const noexcept -> soul_size
  {
    return capacity_;
  }

  template <ts_bit_block BlockT, memory::allocator_type AllocatorT>
  auto BitVector<BlockT, AllocatorT>::size() const noexcept -> soul_size
  {
    return size_;
  }

  template <ts_bit_block BlockT, memory::allocator_type AllocatorT>
  auto BitVector<BlockT, AllocatorT>::empty() const noexcept -> bool
  {
    return size_ == 0;
  }

  template <ts_bit_block BlockT, memory::allocator_type AllocatorT>
  auto BitVector<BlockT, AllocatorT>::set_allocator(AllocatorT& allocator) -> void
  {
    if (allocator_ != nullptr && blocks_ != nullptr) {
      const auto block_count = get_block_count(size_);
      BlockT* blocks = allocator.template allocate_array<BlockT>(size_);
      std::uninitialized_move_n(blocks_, block_count, blocks);
      allocator_->deallocate_array(blocks_, get_block_count(capacity_));
      blocks_ = blocks;
    }
    allocator_ = &allocator;
  }

  template <ts_bit_block BlockT, memory::allocator_type AllocatorT>
  auto BitVector<BlockT, AllocatorT>::get_allocator() const noexcept -> AllocatorT*
  {
    return allocator_;
  }

  template <ts_bit_block BlockT, memory::allocator_type AllocatorT>
  auto BitVector<BlockT, AllocatorT>::resize(const soul_size size) -> void
  {
    if (size > size_) {
      if (size > capacity_) {
        reserve(size);
      }
      const auto old_last_block = get_block_index(size_);
      blocks_[old_last_block] &= ((BlockT(1) << get_block_offset(size_)) - 1);
      const auto block_count_diff = get_block_count(size) - (old_last_block + 1);
      std::uninitialized_fill_n(blocks_ + old_last_block + 1, block_count_diff, false);
    }
    size_ = size;
  }

  template <ts_bit_block BlockT, memory::allocator_type AllocatorT>
  auto BitVector<BlockT, AllocatorT>::reserve(const soul_size capacity) -> void
  {
    if (capacity < capacity_) {
      return;
    }
    BlockT* old_blocks = blocks_;
    const auto block_count = get_block_count(capacity);
    blocks_ = allocator_->template allocate_array<BlockT>(block_count);
    if (old_blocks != nullptr) {
      std::uninitialized_move_n(old_blocks, get_block_count(size_), blocks_);
      allocator_->deallocate_array(old_blocks, get_block_count(capacity_));
    }
    capacity_ = block_count * BLOCK_BIT_COUNT;
  }

  template <ts_bit_block BlockT, memory::allocator_type AllocatorT>
  auto BitVector<BlockT, AllocatorT>::clear() noexcept -> void
  {
    size_ = 0;
  }

  template <ts_bit_block BlockT, memory::allocator_type AllocatorT>
  auto BitVector<BlockT, AllocatorT>::cleanup() -> void
  {
    clear();
    allocator_->deallocate_array(blocks_, get_block_count(capacity_));
    blocks_ = nullptr;
    capacity_ = 0;
  }

  template <ts_bit_block BlockT, memory::allocator_type AllocatorT>
  auto BitVector<BlockT, AllocatorT>::push_back(const bool val) -> void
  {
    if (size_ == capacity_) {
      reserve(get_new_capacity(capacity_));
    }
    size_++;
    back() = val;
  }

  template <ts_bit_block BlockT, memory::allocator_type AllocatorT>
  auto BitVector<BlockT, AllocatorT>::push_back() -> reference
  {
    if (size_ == capacity_) {
      reserve(get_new_capacity(capacity_));
    }
    size_++;
    back() = false;
    return back();
  }

  template <ts_bit_block BlockT, memory::allocator_type AllocatorT>
  auto BitVector<BlockT, AllocatorT>::pop_back() -> void
  {
    SOUL_ASSERT(0, size_ != 0, "");
    size_--;
  }

  template <ts_bit_block BlockT, memory::allocator_type AllocatorT>
  auto BitVector<BlockT, AllocatorT>::pop_back(const soul_size count) -> void
  {
    SOUL_ASSERT(0, size_ >= count, "");
    size_ -= count;
  }

  template <ts_bit_block BlockT, memory::allocator_type AllocatorT>
  auto BitVector<BlockT, AllocatorT>::front() noexcept -> reference
  {
    SOUL_ASSERT(0, size_ != 0, "");
    return reference(blocks_, 0);
  }

  template <ts_bit_block BlockT, memory::allocator_type AllocatorT>
  auto BitVector<BlockT, AllocatorT>::front() const noexcept -> const_reference
  {
    SOUL_ASSERT(0, size_ != 0, "");
    return static_cast<const_reference>(blocks_[0] & BlockT(1));
  }

  template <ts_bit_block BlockT, memory::allocator_type AllocatorT>
  auto BitVector<BlockT, AllocatorT>::back() noexcept -> reference
  {
    SOUL_ASSERT(0, size_ != 0, "");
    return get_bit_ref(size_ - 1);
  }

  template <ts_bit_block BlockT, memory::allocator_type AllocatorT>
  auto BitVector<BlockT, AllocatorT>::back() const noexcept -> const_reference
  {
    SOUL_ASSERT(0, size_ != 0, "");
    return static_cast<const_reference>(
      blocks_[get_block_index(size_ - 1)] & (BlockT(1) << get_block_offset(size_ - 1)));
  }

  template <ts_bit_block BlockT, memory::allocator_type AllocatorT>
  auto BitVector<BlockT, AllocatorT>::operator[](const soul_size index) -> reference
  {
    SOUL_ASSERT(0, index < size_, "");
    return get_bit_ref(index);
  }

  template <ts_bit_block BlockT, memory::allocator_type AllocatorT>
  auto BitVector<BlockT, AllocatorT>::operator[](const soul_size index) const -> const_reference
  {
    SOUL_ASSERT(0, index < size_, "");
    return get_bool(index);
  }

  template <ts_bit_block BlockT, memory::allocator_type AllocatorT>
  auto BitVector<BlockT, AllocatorT>::test(const soul_size index, const bool default_value) const
    -> bool
  {
    if (index >= size_) {
      return default_value;
    }
    return get_bool(index);
  }

  template <ts_bit_block BlockT, memory::allocator_type AllocatorT>
  auto BitVector<BlockT, AllocatorT>::set(const soul_size index, const bool value) -> void
  {
    if (index >= size_) {
      resize(index + 1);
    }
    get_bit_ref(index) = value;
  }

  template <ts_bit_block BlockT, memory::allocator_type AllocatorT>
  auto BitVector<BlockT, AllocatorT>::set() -> void
  {
    std::fill_n(blocks_, get_block_count(size_), ~BlockT(0));
  }

  template <ts_bit_block BlockT, memory::allocator_type AllocatorT>
  auto BitVector<BlockT, AllocatorT>::reset() -> void
  {
    std::fill_n(blocks_, get_block_count(size_), BlockT(0));
  }

  template <ts_bit_block BlockT, memory::allocator_type AllocatorT>
  auto BitVector<BlockT, AllocatorT>::get_bit_ref(const soul_size index) -> reference
  {
    return reference(blocks_ + get_block_index(index), get_block_offset(index));
  }

  template <ts_bit_block BlockT, memory::allocator_type AllocatorT>
  auto BitVector<BlockT, AllocatorT>::get_bool(const soul_size index) const -> bool
  {
    return blocks_[get_block_index(index)] & (BlockT(1) << get_block_offset(index));
  }

  template <ts_bit_block BlockT, memory::allocator_type AllocatorT>
  auto BitVector<BlockT, AllocatorT>::get_new_capacity(const soul_size old_capacity) -> soul_size
  {
    return old_capacity * GROWTH_FACTOR + BLOCK_BIT_COUNT;
  }

  template <ts_bit_block BlockT, memory::allocator_type AllocatorT>
  auto BitVector<BlockT, AllocatorT>::init_reserve(const soul_size capacity) -> void
  {
    const auto block_count = get_block_count(capacity);
    blocks_ = allocator_->template allocate_array<BlockT>(block_count);
    capacity_ = block_count * BLOCK_BIT_COUNT;
  }

  template <ts_bit_block BlockT, memory::allocator_type AllocatorT>
  auto BitVector<BlockT, AllocatorT>::init_resize(const soul_size size, const bool val) -> void
  {
    const auto block_count = get_block_count(size);
    blocks_ = allocator_->template allocate_array<BlockT>(block_count);
    capacity_ = block_count * BLOCK_BIT_COUNT;
    std::uninitialized_fill_n(blocks_, block_count, val ? ~BlockT(0) : BlockT(0));
    size_ = size;
  }

  template <ts_bit_block BlockT, memory::allocator_type AllocatorT>
  auto BitVector<BlockT, AllocatorT>::get_block_count(const soul_size size) -> soul_size
  {
    return (size + BLOCK_BIT_COUNT - 1) / BLOCK_BIT_COUNT;
  }

  template <ts_bit_block BlockT, memory::allocator_type AllocatorT>
  auto BitVector<BlockT, AllocatorT>::get_block_index(const soul_size index) -> soul_size
  {
    return index / BLOCK_BIT_COUNT;
  }

  template <ts_bit_block BlockT, memory::allocator_type AllocatorT>
  auto BitVector<BlockT, AllocatorT>::get_block_offset(const soul_size index) -> soul_size
  {
    return index % BLOCK_BIT_COUNT;
  }
} // namespace soul
