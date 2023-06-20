#pragma once

#include <iostream>
#include <optional>
#include <ranges>

#include "core/bit_ref.h"
#include "core/config.h"
#include "core/type.h"
#include "memory/allocator.h"

namespace soul
{

  struct bit_vector_construct {
    struct with_capacity_t {
    };
    struct from_t {
    };
    struct init_fill_n_t {
    };
    static constexpr auto with_capacity = with_capacity_t{};
    static constexpr auto from = from_t{};
    static constexpr auto init_fill_n = init_fill_n_t{};
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

    BitVector(BitVector&& other) noexcept;

    auto operator=(BitVector&& other) noexcept -> BitVector&;

    ~BitVector();

    [[nodiscard]]
    static auto with_capacity(soul_size capacity, AllocatorT& allocator = *get_default_allocator())
      -> this_type;

    template <std::ranges::input_range RangeT>
    [[nodiscard]]
    static auto from(RangeT&& range, AllocatorT& allocator = *get_default_allocator()) -> this_type;

    [[nodiscard]]
    static auto init_fill_n(
      soul_size size, bool val, AllocatorT& allocator = *get_default_allocator()) -> this_type;

    [[nodiscard]]
    auto clone() const -> this_type;

    void clone_from(const this_type& other);

    void swap(this_type& other) noexcept;

    friend void swap(this_type& a, this_type& b) noexcept { a.swap(b); }

    [[nodiscard]]
    auto capacity() const noexcept -> soul_size;

    [[nodiscard]]
    auto size() const noexcept -> soul_size;

    [[nodiscard]]
    auto empty() const noexcept -> bool;

    void set_allocator(AllocatorT& allocator);

    [[nodiscard]]
    auto get_allocator() const noexcept -> AllocatorT*;

    void resize(soul_size size);

    void reserve(soul_size capacity);

    void clear() noexcept;

    void cleanup();

    void push_back(bool val);

    auto push_back() -> reference;

    void pop_back();

    void pop_back(soul_size count);

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

    void set(soul_size index, bool value = true);

    void set();

    void reset();

  private:
    AllocatorT* allocator_;
    BlockT* blocks_ = nullptr;
    soul_size size_ = 0;
    soul_size capacity_ = 0;

    static constexpr soul_size BLOCK_BIT_COUNT = sizeof(BlockT) * 8;
    static constexpr soul_size GROWTH_FACTOR = 2;

    BitVector(const BitVector& other);

    BitVector(const BitVector& other, AllocatorT& allocator);

    BitVector(bit_vector_construct::with_capacity_t tag, soul_size capacity, AllocatorT& allocator);

    template <std::ranges::input_range RangeT>
    BitVector(bit_vector_construct::from_t tag, RangeT&& range, AllocatorT& allocator);

    BitVector(
      bit_vector_construct::init_fill_n_t tag, soul_size size, bool val, AllocatorT& allocator);

    auto operator=(const BitVector& other) -> BitVector&;

    auto get_bit_ref(soul_size index) -> reference;

    [[nodiscard]]
    auto get_bool(soul_size index) const -> bool;

    static auto get_new_capacity(soul_size old_capacity) -> soul_size;

    void init_reserve(soul_size capacity);

    void init_resize(soul_size size, bool val);

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
  BitVector<BlockT, AllocatorT>::BitVector(
    bit_vector_construct::with_capacity_t /* tag */, soul_size capacity, AllocatorT& allocator)
      : allocator_(&allocator)
  {
    init_reserve(capacity);
  }

  template <ts_bit_block BlockT, memory::allocator_type AllocatorT>
  template <std::ranges::input_range RangeT>
  BitVector<BlockT, AllocatorT>::BitVector(
    bit_vector_construct::from_t /* tag */, RangeT&& range, AllocatorT& allocator)
      : allocator_(&allocator)
  {
    if constexpr (std::ranges::sized_range<RangeT>) {
      init_reserve(std::ranges::distance(range));
    }
    for (bool val : range) {
      push_back(val);
    }
  }

  template <ts_bit_block BlockT, memory::allocator_type AllocatorT>
  BitVector<BlockT, AllocatorT>::BitVector(
    bit_vector_construct::init_fill_n_t /* tag */, soul_size size, bool val, AllocatorT& allocator)
      : allocator_(&allocator)
  {
    init_resize(size, val);
  }

  template <ts_bit_block BlockT, memory::allocator_type AllocatorT>
  auto BitVector<BlockT, AllocatorT>::operator=(const BitVector& other) -> this_type&
  {
    this_type(other, *allocator_).swap(*this);
    return *this;
  }

  template <ts_bit_block BlockT, memory::allocator_type AllocatorT>
  auto BitVector<BlockT, AllocatorT>::operator=(BitVector&& other) noexcept -> this_type&
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
  auto BitVector<BlockT, AllocatorT>::with_capacity(soul_size capacity, AllocatorT& allocator)
    -> this_type
  {
    return BitVector(bit_vector_construct::with_capacity, capacity, allocator);
  }

  template <ts_bit_block BlockT, memory::allocator_type AllocatorT>
  template <std::ranges::input_range RangeT>
  [[nodiscard]]
  auto BitVector<BlockT, AllocatorT>::from(RangeT&& range, AllocatorT& allocator) -> this_type
  {
    return BitVector(bit_vector_construct::from, std::forward<RangeT>(range), allocator);
  }

  template <ts_bit_block BlockT, memory::allocator_type AllocatorT>
  [[nodiscard]]
  auto BitVector<BlockT, AllocatorT>::init_fill_n(soul_size size, bool val, AllocatorT& allocator)
    -> this_type
  {
    return BitVector(bit_vector_construct::init_fill_n, size, val, allocator);
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
  void BitVector<BlockT, AllocatorT>::swap(this_type& other) noexcept
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
  void BitVector<BlockT, AllocatorT>::set_allocator(AllocatorT& allocator)
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
  void BitVector<BlockT, AllocatorT>::resize(const soul_size size)
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
  void BitVector<BlockT, AllocatorT>::reserve(const soul_size capacity)
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
  void BitVector<BlockT, AllocatorT>::clear() noexcept
  {
    size_ = 0;
  }

  template <ts_bit_block BlockT, memory::allocator_type AllocatorT>
  void BitVector<BlockT, AllocatorT>::cleanup()
  {
    clear();
    allocator_->deallocate_array(blocks_, get_block_count(capacity_));
    blocks_ = nullptr;
    capacity_ = 0;
  }

  template <ts_bit_block BlockT, memory::allocator_type AllocatorT>
  void BitVector<BlockT, AllocatorT>::push_back(const bool val)
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
  void BitVector<BlockT, AllocatorT>::pop_back()
  {
    SOUL_ASSERT(0, size_ != 0, "");
    size_--;
  }

  template <ts_bit_block BlockT, memory::allocator_type AllocatorT>
  void BitVector<BlockT, AllocatorT>::pop_back(const soul_size count)
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
  void BitVector<BlockT, AllocatorT>::set(const soul_size index, const bool value)
  {
    if (index >= size_) {
      resize(index + 1);
    }
    get_bit_ref(index) = value;
  }

  template <ts_bit_block BlockT, memory::allocator_type AllocatorT>
  void BitVector<BlockT, AllocatorT>::set()
  {
    std::fill_n(blocks_, get_block_count(size_), ~BlockT(0));
  }

  template <ts_bit_block BlockT, memory::allocator_type AllocatorT>
  void BitVector<BlockT, AllocatorT>::reset()
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
  void BitVector<BlockT, AllocatorT>::init_reserve(const soul_size capacity)
  {
    const auto block_count = get_block_count(capacity);
    blocks_ = allocator_->template allocate_array<BlockT>(block_count);
    capacity_ = block_count * BLOCK_BIT_COUNT;
  }

  template <ts_bit_block BlockT, memory::allocator_type AllocatorT>
  void BitVector<BlockT, AllocatorT>::init_resize(const soul_size size, const bool val)
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
