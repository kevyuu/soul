#pragma once

#include <iostream>
#include <optional>

#include "core/bit_ref.h"
#include "core/config.h"
#include "core/type.h"
#include "memory/allocator.h"

namespace soul
{

  template <typename Iterator>
  concept bool_input_iterator =
    std::input_iterator<Iterator> && std::same_as<std::iter_value_t<Iterator>, bool>;

  struct BitVectorInitDesc {
    soul_size size = 0;
    bool val = false;
    soul_size capacity = 0;
  };

  template <
    bit_block_type BlockType = SOUL_BIT_BLOCK_TYPE_DEFAULT,
    memory::allocator_type AllocatorType = memory::Allocator>
  class BitVector
  {
  public:
    using this_type = BitVector<BlockType, AllocatorType>;
    using value_type = bool;
    using reference = BitRef<BlockType>;
    using const_reference = bool;

    explicit BitVector(memory::Allocator* allocator = get_default_allocator());
    explicit BitVector(
      BitVectorInitDesc init_desc, AllocatorType& allocator = *get_default_allocator());
    explicit BitVector(soul_size size, AllocatorType& allocator = *get_default_allocator());
    BitVector(
      soul_size size, const value_type& value, AllocatorType& allocator = *get_default_allocator());
    BitVector(BitVector&& other) noexcept;

    template <bool_input_iterator Iterator>
    BitVector(Iterator first, Iterator last, AllocatorType& allocator = *get_default_allocator());

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

    auto set_allocator(AllocatorType& allocator) -> void;
    [[nodiscard]]
    auto get_allocator() const noexcept -> AllocatorType*;

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
    AllocatorType* allocator_;
    BlockType* blocks_ = nullptr;
    soul_size size_ = 0;
    soul_size capacity_ = 0;

    static constexpr soul_size BLOCK_BIT_COUNT = sizeof(BlockType) * 8;
    static constexpr soul_size GROWTH_FACTOR = 2;

    BitVector(const BitVector& other);
    BitVector(const BitVector& other, AllocatorType& allocator);
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

  template <bit_block_type BlockType, memory::allocator_type AllocatorType>
  BitVector<BlockType, AllocatorType>::BitVector(memory::Allocator* allocator)
      : allocator_(allocator)
  {
  }

  template <bit_block_type BlockType, memory::allocator_type AllocatorType>
  BitVector<BlockType, AllocatorType>::BitVector(
    BitVectorInitDesc init_desc, AllocatorType& allocator)
      : allocator_(&allocator), size_(init_desc.size)
  {
    const auto capacity = init_desc.size > init_desc.capacity ? init_desc.size : init_desc.capacity;
    const auto block_count = get_block_count(capacity);
    blocks_ = allocator_->template allocate_array<BlockType>(block_count);
    capacity_ = block_count * BLOCK_BIT_COUNT;
    std::uninitialized_fill_n(
      blocks_, get_block_count(init_desc.size), init_desc.val ? ~BlockType(0) : BlockType(0));
  }

  template <bit_block_type BlockType, memory::allocator_type AllocatorType>
  BitVector<BlockType, AllocatorType>::BitVector(const soul_size size, AllocatorType& allocator)
      : allocator_(&allocator)
  {
    init_resize(size, false);
  }

  template <bit_block_type BlockType, memory::allocator_type AllocatorType>
  BitVector<BlockType, AllocatorType>::BitVector(
    const soul_size size, const value_type& value, AllocatorType& allocator)
      : allocator_(&allocator)
  {
    init_resize(size, value);
  }

  template <bit_block_type BlockType, memory::allocator_type AllocatorType>
  BitVector<BlockType, AllocatorType>::BitVector(const BitVector& other)
      : allocator_(other.allocator_), size_(other.size_)
  {
    init_reserve(other.size_);
    std::uninitialized_copy_n(other.blocks_, get_block_count(other.size_), blocks_);
  }

  template <bit_block_type BlockType, memory::allocator_type AllocatorType>
  BitVector<BlockType, AllocatorType>::BitVector(const BitVector& other, AllocatorType& allocator)
      : allocator_(&allocator), size_(other.size_)
  {
    init_reserve(other.size_);
    std::uninitialized_copy_n(other.blocks_, get_block_count(other.size_), blocks_);
  }

  template <bit_block_type BlockType, memory::allocator_type AllocatorType>
  BitVector<BlockType, AllocatorType>::BitVector(BitVector&& other) noexcept
      : allocator_(std::exchange(other.allocator_, nullptr)),
        blocks_(std::exchange(other.blocks_, nullptr)),
        size_(std::exchange(other.size_, 0)),
        capacity_(std::exchange(other.capacity_, 0))
  {
  }

  template <bit_block_type BlockType, memory::allocator_type AllocatorType>
  template <bool_input_iterator Iterator>
  BitVector<BlockType, AllocatorType>::BitVector(
    Iterator first, Iterator last, AllocatorType& allocator)
      : allocator_(&allocator)
  {
    if constexpr (std::forward_iterator<Iterator>) {
      init_reserve(std::distance(first, last));
    }
    for (; first != last; ++first) {
      push_back(*first);
    }
  }

  template <bit_block_type BlockType, memory::allocator_type AllocatorType>
  auto BitVector<BlockType, AllocatorType>::operator=(const BitVector& other)
    -> BitVector<BlockType, AllocatorType>&
  // NOLINT(bugprone-unhandled-self-assignment)

  {
    this_type(other, *allocator_).swap(*this);
    return *this;
  }

  template <bit_block_type BlockType, memory::allocator_type AllocatorType>
  auto BitVector<BlockType, AllocatorType>::operator=(BitVector&& other) noexcept
    -> BitVector<BlockType, AllocatorType>&
  {
    if (this->allocator_ == other.allocator_) {
      this_type(std::move(other)).swap(*this);
    } else {
      this_type(other, *allocator_).swap(*this);
    }
    return *this;
  }

  template <bit_block_type BlockType, memory::allocator_type AllocatorType>
  BitVector<BlockType, AllocatorType>::~BitVector()
  {
    SOUL_ASSERT(0, allocator_ != nullptr || blocks_ == nullptr, "");
    if (allocator_ == nullptr) {
      return;
    }
    cleanup();
  }

  template <bit_block_type BlockType, memory::allocator_type AllocatorType>
  [[nodiscard]]
  auto BitVector<BlockType, AllocatorType>::clone() const -> this_type
  {
    return this_type(*this);
  }

  template <bit_block_type BlockType, memory::allocator_type AllocatorType>
  void BitVector<BlockType, AllocatorType>::clone_from(const this_type& other)
  {
    return *this = other;
  }

  template <bit_block_type BlockType, memory::allocator_type AllocatorType>
  auto BitVector<BlockType, AllocatorType>::swap(this_type& other) noexcept -> void
  {
    using std::swap;
    SOUL_ASSERT(
      0, allocator_ == other.allocator_, "Cannot swap container with different allocators");
    swap(blocks_, other.blocks_);
    swap(size_, other.size_);
    swap(capacity_, other.capacity_);
  }

  template <bit_block_type BlockType, memory::allocator_type AllocatorType>
  auto BitVector<BlockType, AllocatorType>::capacity() const noexcept -> soul_size
  {
    return capacity_;
  }

  template <bit_block_type BlockType, memory::allocator_type AllocatorType>
  auto BitVector<BlockType, AllocatorType>::size() const noexcept -> soul_size
  {
    return size_;
  }

  template <bit_block_type BlockType, memory::allocator_type AllocatorType>
  auto BitVector<BlockType, AllocatorType>::empty() const noexcept -> bool
  {
    return size_ == 0;
  }

  template <bit_block_type BlockType, memory::allocator_type AllocatorType>
  auto BitVector<BlockType, AllocatorType>::set_allocator(AllocatorType& allocator) -> void
  {
    if (allocator_ != nullptr && blocks_ != nullptr) {
      const auto block_count = get_block_count(size_);
      BlockType* blocks = allocator.template allocate_array<BlockType>(size_);
      std::uninitialized_move_n(blocks_, block_count, blocks);
      allocator_->deallocate_array(blocks_, get_block_count(capacity_));
      blocks_ = blocks;
    }
    allocator_ = &allocator;
  }

  template <bit_block_type BlockType, memory::allocator_type AllocatorType>
  auto BitVector<BlockType, AllocatorType>::get_allocator() const noexcept -> AllocatorType*
  {
    return allocator_;
  }

  template <bit_block_type BlockType, memory::allocator_type AllocatorType>
  auto BitVector<BlockType, AllocatorType>::resize(const soul_size size) -> void
  {
    if (size > size_) {
      if (size > capacity_) {
        reserve(size);
      }
      const auto old_last_block = get_block_index(size_);
      blocks_[old_last_block] &= ((BlockType(1) << get_block_offset(size_)) - 1);
      const auto block_count_diff = get_block_count(size) - (old_last_block + 1);
      std::uninitialized_fill_n(blocks_ + old_last_block + 1, block_count_diff, false);
    }
    size_ = size;
  }

  template <bit_block_type BlockType, memory::allocator_type AllocatorType>
  auto BitVector<BlockType, AllocatorType>::reserve(const soul_size capacity) -> void
  {
    if (capacity < capacity_) {
      return;
    }
    BlockType* old_blocks = blocks_;
    const auto block_count = get_block_count(capacity);
    blocks_ = allocator_->template allocate_array<BlockType>(block_count);
    if (old_blocks != nullptr) {
      std::uninitialized_move_n(old_blocks, get_block_count(size_), blocks_);
      allocator_->deallocate_array(old_blocks, get_block_count(capacity_));
    }
    capacity_ = block_count * BLOCK_BIT_COUNT;
  }

  template <bit_block_type BlockType, memory::allocator_type AllocatorType>
  auto BitVector<BlockType, AllocatorType>::clear() noexcept -> void
  {
    size_ = 0;
  }

  template <bit_block_type BlockType, memory::allocator_type AllocatorType>
  auto BitVector<BlockType, AllocatorType>::cleanup() -> void
  {
    clear();
    allocator_->deallocate_array(blocks_, get_block_count(capacity_));
    blocks_ = nullptr;
    capacity_ = 0;
  }

  template <bit_block_type BlockType, memory::allocator_type AllocatorType>
  auto BitVector<BlockType, AllocatorType>::push_back(const bool val) -> void
  {
    if (size_ == capacity_) {
      reserve(get_new_capacity(capacity_));
    }
    size_++;
    back() = val;
  }

  template <bit_block_type BlockType, memory::allocator_type AllocatorType>
  auto BitVector<BlockType, AllocatorType>::push_back() -> reference
  {
    if (size_ == capacity_) {
      reserve(get_new_capacity(capacity_));
    }
    size_++;
    back() = false;
    return back();
  }

  template <bit_block_type BlockType, memory::allocator_type AllocatorType>
  auto BitVector<BlockType, AllocatorType>::pop_back() -> void
  {
    SOUL_ASSERT(0, size_ != 0, "");
    size_--;
  }

  template <bit_block_type BlockType, memory::allocator_type AllocatorType>
  auto BitVector<BlockType, AllocatorType>::pop_back(const soul_size count) -> void
  {
    SOUL_ASSERT(0, size_ >= count, "");
    size_ -= count;
  }

  template <bit_block_type BlockType, memory::allocator_type AllocatorType>
  auto BitVector<BlockType, AllocatorType>::front() noexcept -> reference
  {
    SOUL_ASSERT(0, size_ != 0, "");
    return reference(blocks_, 0);
  }

  template <bit_block_type BlockType, memory::allocator_type AllocatorType>
  auto BitVector<BlockType, AllocatorType>::front() const noexcept -> const_reference
  {
    SOUL_ASSERT(0, size_ != 0, "");
    return static_cast<const_reference>(blocks_[0] & BlockType(1));
  }

  template <bit_block_type BlockType, memory::allocator_type AllocatorType>
  auto BitVector<BlockType, AllocatorType>::back() noexcept -> reference
  {
    SOUL_ASSERT(0, size_ != 0, "");
    return get_bit_ref(size_ - 1);
  }

  template <bit_block_type BlockType, memory::allocator_type AllocatorType>
  auto BitVector<BlockType, AllocatorType>::back() const noexcept -> const_reference
  {
    SOUL_ASSERT(0, size_ != 0, "");
    return static_cast<const_reference>(
      blocks_[get_block_index(size_ - 1)] & (BlockType(1) << get_block_offset(size_ - 1)));
  }

  template <bit_block_type BlockType, memory::allocator_type AllocatorType>
  auto BitVector<BlockType, AllocatorType>::operator[](const soul_size index) -> reference
  {
    SOUL_ASSERT(0, index < size_, "");
    return get_bit_ref(index);
  }

  template <bit_block_type BlockType, memory::allocator_type AllocatorType>
  auto BitVector<BlockType, AllocatorType>::operator[](const soul_size index) const
    -> const_reference
  {
    SOUL_ASSERT(0, index < size_, "");
    return get_bool(index);
  }

  template <bit_block_type BlockType, memory::allocator_type AllocatorType>
  auto BitVector<BlockType, AllocatorType>::test(
    const soul_size index, const bool default_value) const -> bool
  {
    if (index >= size_) {
      return default_value;
    }
    return get_bool(index);
  }

  template <bit_block_type BlockType, memory::allocator_type AllocatorType>
  auto BitVector<BlockType, AllocatorType>::set(const soul_size index, const bool value) -> void
  {
    if (index >= size_) {
      resize(index + 1);
    }
    get_bit_ref(index) = value;
  }

  template <bit_block_type BlockType, memory::allocator_type AllocatorType>
  auto BitVector<BlockType, AllocatorType>::set() -> void
  {
    std::fill_n(blocks_, get_block_count(size_), ~BlockType(0));
  }

  template <bit_block_type BlockType, memory::allocator_type AllocatorType>
  auto BitVector<BlockType, AllocatorType>::reset() -> void
  {
    std::fill_n(blocks_, get_block_count(size_), BlockType(0));
  }

  template <bit_block_type BlockType, memory::allocator_type AllocatorType>
  auto BitVector<BlockType, AllocatorType>::get_bit_ref(const soul_size index) -> reference
  {
    return reference(blocks_ + get_block_index(index), get_block_offset(index));
  }

  template <bit_block_type BlockType, memory::allocator_type AllocatorType>
  auto BitVector<BlockType, AllocatorType>::get_bool(const soul_size index) const -> bool
  {
    return blocks_[get_block_index(index)] & (BlockType(1) << get_block_offset(index));
  }

  template <bit_block_type BlockType, memory::allocator_type AllocatorType>
  auto BitVector<BlockType, AllocatorType>::get_new_capacity(const soul_size old_capacity)
    -> soul_size
  {
    return old_capacity * GROWTH_FACTOR + BLOCK_BIT_COUNT;
  }

  template <bit_block_type BlockType, memory::allocator_type AllocatorType>
  auto BitVector<BlockType, AllocatorType>::init_reserve(const soul_size capacity) -> void
  {
    const auto block_count = get_block_count(capacity);
    blocks_ = allocator_->template allocate_array<BlockType>(block_count);
    capacity_ = block_count * BLOCK_BIT_COUNT;
  }

  template <bit_block_type BlockType, memory::allocator_type AllocatorType>
  auto BitVector<BlockType, AllocatorType>::init_resize(const soul_size size, const bool val)
    -> void
  {
    const auto block_count = get_block_count(size);
    blocks_ = allocator_->template allocate_array<BlockType>(block_count);
    capacity_ = block_count * BLOCK_BIT_COUNT;
    std::uninitialized_fill_n(blocks_, block_count, val ? ~BlockType(0) : BlockType(0));
    size_ = size;
  }

  template <bit_block_type BlockType, memory::allocator_type AllocatorType>
  auto BitVector<BlockType, AllocatorType>::get_block_count(const soul_size size) -> soul_size
  {
    return (size + BLOCK_BIT_COUNT - 1) / BLOCK_BIT_COUNT;
  }

  template <bit_block_type BlockType, memory::allocator_type AllocatorType>
  auto BitVector<BlockType, AllocatorType>::get_block_index(const soul_size index) -> soul_size
  {
    return index / BLOCK_BIT_COUNT;
  }

  template <bit_block_type BlockType, memory::allocator_type AllocatorType>
  auto BitVector<BlockType, AllocatorType>::get_block_offset(const soul_size index) -> soul_size
  {
    return index % BLOCK_BIT_COUNT;
  }
} // namespace soul
