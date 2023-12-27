#pragma once

#include <ranges>

#include "core/bit_ref.h"
#include "core/config.h"
#include "core/panic.h"
#include "core/type.h"

#include "memory/allocator.h"

namespace soul
{

  template <
    ts_bit_block BlockT = SOUL_BIT_BLOCK_TYPE_DEFAULT,
    memory::allocator_type AllocatorT = memory::Allocator>
  class BitVector
  {
  public:
    using this_type = BitVector<BlockT, AllocatorT>;
    using value_type = b8;
    using reference = BitRef<BlockT>;
    using const_reference = b8;

    explicit BitVector(memory::Allocator* allocator = get_default_allocator());

    BitVector(BitVector&& other) noexcept;

    auto operator=(BitVector&& other) noexcept -> BitVector&;

    ~BitVector();

    [[nodiscard]]
    static auto WithCapacity(usize capacity, AllocatorT& allocator = *get_default_allocator())
      -> this_type;

    template <std::ranges::input_range RangeT>
    [[nodiscard]]
    static auto From(RangeT&& range, AllocatorT& allocator = *get_default_allocator()) -> this_type;

    [[nodiscard]]
    static auto FillN(usize size, b8 val, AllocatorT& allocator = *get_default_allocator())
      -> this_type;

    [[nodiscard]]
    auto clone() const -> this_type;

    void clone_from(const this_type& other);

    void swap(this_type& other) noexcept;

    friend void swap(this_type& a, this_type& b) noexcept { a.swap(b); }

    [[nodiscard]]
    auto capacity() const noexcept -> usize;

    [[nodiscard]]
    auto size() const noexcept -> usize;

    [[nodiscard]]
    auto empty() const noexcept -> b8;

    void set_allocator(AllocatorT& allocator);

    [[nodiscard]]
    auto get_allocator() const noexcept -> AllocatorT*;

    void resize(usize size);

    void reserve(usize capacity);

    void clear() noexcept;

    void cleanup();

    void push_back(b8 val);

    auto push_back() -> reference;

    void pop_back();

    void pop_back(usize count);

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
    operator[](usize index) -> reference;

    [[nodiscard]]
    auto
    operator[](usize index) const -> const_reference;

    [[nodiscard]]
    auto test(usize index, b8 default_value) const -> b8;

    void set(usize index, b8 value = true);

    void set();

    void reset();

  private:
    NotNull<AllocatorT*> allocator_;
    BlockT* blocks_ = nullptr;
    usize size_ = 0;
    usize capacity_ = 0;

    static constexpr usize BLOCK_BIT_COUNT = sizeof(BlockT) * 8;
    static constexpr usize GROWTH_FACTOR = 2;

    BitVector(const BitVector& other);

    BitVector(const BitVector& other, AllocatorT& allocator);

    struct Construct {
      struct WithCapacity {
      };
      struct From {
      };
      struct FillN {
      };
      static constexpr auto with_capacity = WithCapacity{};
      static constexpr auto from = From{};
      static constexpr auto fill_n = FillN{};
    };

    BitVector(Construct::WithCapacity tag, usize capacity, AllocatorT& allocator);

    template <std::ranges::input_range RangeT>
    BitVector(Construct::From tag, RangeT&& range, AllocatorT& allocator);

    BitVector(Construct::FillN tag, usize size, b8 val, AllocatorT& allocator);

    auto operator=(const BitVector& other) -> BitVector&;

    auto get_bit_ref(usize index) -> reference;

    [[nodiscard]]
    auto get_bool(usize index) const -> b8;

    static auto get_new_capacity(usize old_capacity) -> usize;

    void init_reserve(usize capacity);

    void init_resize(usize size, b8 val);

    [[nodiscard]]
    static auto get_block_count(usize size) -> usize;

    [[nodiscard]]
    static auto get_block_index(usize index) -> usize;

    [[nodiscard]]
    static auto get_block_offset(usize index) -> usize;
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
      : allocator_(other.allocator_),
        blocks_(std::exchange(other.blocks_, nullptr)),
        size_(std::exchange(other.size_, 0)),
        capacity_(std::exchange(other.capacity_, 0))
  {
  }

  template <ts_bit_block BlockT, memory::allocator_type AllocatorT>
  BitVector<BlockT, AllocatorT>::BitVector(
    Construct::WithCapacity /* tag */, usize capacity, AllocatorT& allocator)
      : allocator_(&allocator)
  {
    init_reserve(capacity);
  }

  template <ts_bit_block BlockT, memory::allocator_type AllocatorT>
  template <std::ranges::input_range RangeT>
  BitVector<BlockT, AllocatorT>::BitVector(
    Construct::From /* tag */, RangeT&& range, AllocatorT& allocator)
      : allocator_(&allocator)
  {
    if constexpr (std::ranges::sized_range<RangeT>) {
      init_reserve(std::ranges::distance(range));
    }
    for (b8 val : range) {
      push_back(val);
    }
  }

  template <ts_bit_block BlockT, memory::allocator_type AllocatorT>
  BitVector<BlockT, AllocatorT>::BitVector(
    Construct::FillN /* tag */, usize size, b8 val, AllocatorT& allocator)
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
    SOUL_ASSERT(0, allocator_ != nullptr || blocks_ == nullptr);
    if (allocator_ == nullptr) {
      return;
    }
    cleanup();
  }

  template <ts_bit_block BlockT, memory::allocator_type AllocatorT>
  [[nodiscard]]
  auto BitVector<BlockT, AllocatorT>::WithCapacity(usize capacity, AllocatorT& allocator)
    -> this_type
  {
    return BitVector(Construct::with_capacity, capacity, allocator);
  }

  template <ts_bit_block BlockT, memory::allocator_type AllocatorT>
  template <std::ranges::input_range RangeT>
  [[nodiscard]]
  auto BitVector<BlockT, AllocatorT>::From(RangeT&& range, AllocatorT& allocator) -> this_type
  {
    return BitVector(Construct::from, std::forward<RangeT>(range), allocator);
  }

  template <ts_bit_block BlockT, memory::allocator_type AllocatorT>
  [[nodiscard]]
  auto BitVector<BlockT, AllocatorT>::FillN(usize size, b8 val, AllocatorT& allocator) -> this_type
  {
    return BitVector(Construct::fill_n, size, val, allocator);
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
  auto BitVector<BlockT, AllocatorT>::capacity() const noexcept -> usize
  {
    return capacity_;
  }

  template <ts_bit_block BlockT, memory::allocator_type AllocatorT>
  auto BitVector<BlockT, AllocatorT>::size() const noexcept -> usize
  {
    return size_;
  }

  template <ts_bit_block BlockT, memory::allocator_type AllocatorT>
  auto BitVector<BlockT, AllocatorT>::empty() const noexcept -> b8
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
  void BitVector<BlockT, AllocatorT>::resize(const usize size)
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
  void BitVector<BlockT, AllocatorT>::reserve(const usize capacity)
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
  void BitVector<BlockT, AllocatorT>::push_back(const b8 val)
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
    SOUL_ASSERT(0, size_ != 0);
    size_--;
  }

  template <ts_bit_block BlockT, memory::allocator_type AllocatorT>
  void BitVector<BlockT, AllocatorT>::pop_back(const usize count)
  {
    SOUL_ASSERT(0, size_ >= count);
    size_ -= count;
  }

  template <ts_bit_block BlockT, memory::allocator_type AllocatorT>
  auto BitVector<BlockT, AllocatorT>::front() noexcept -> reference
  {
    SOUL_ASSERT(0, size_ != 0);
    return reference(blocks_, 0);
  }

  template <ts_bit_block BlockT, memory::allocator_type AllocatorT>
  auto BitVector<BlockT, AllocatorT>::front() const noexcept -> const_reference
  {
    SOUL_ASSERT(0, size_ != 0);
    return static_cast<const_reference>(blocks_[0] & BlockT(1));
  }

  template <ts_bit_block BlockT, memory::allocator_type AllocatorT>
  auto BitVector<BlockT, AllocatorT>::back() noexcept -> reference
  {
    SOUL_ASSERT(0, size_ != 0);
    return get_bit_ref(size_ - 1);
  }

  template <ts_bit_block BlockT, memory::allocator_type AllocatorT>
  auto BitVector<BlockT, AllocatorT>::back() const noexcept -> const_reference
  {
    SOUL_ASSERT(0, size_ != 0);
    return static_cast<const_reference>(
      blocks_[get_block_index(size_ - 1)] & (BlockT(1) << get_block_offset(size_ - 1)));
  }

  template <ts_bit_block BlockT, memory::allocator_type AllocatorT>
  auto BitVector<BlockT, AllocatorT>::operator[](const usize index) -> reference
  {
    SOUL_ASSERT_UPPER_BOUND_CHECK(index, size_);
    return get_bit_ref(index);
  }

  template <ts_bit_block BlockT, memory::allocator_type AllocatorT>
  auto BitVector<BlockT, AllocatorT>::operator[](const usize index) const -> const_reference
  {
    SOUL_ASSERT_UPPER_BOUND_CHECK(index, size_);
    return get_bool(index);
  }

  template <ts_bit_block BlockT, memory::allocator_type AllocatorT>
  auto BitVector<BlockT, AllocatorT>::test(const usize index, const b8 default_value) const -> b8
  {
    if (index >= size_) {
      return default_value;
    }
    return get_bool(index);
  }

  template <ts_bit_block BlockT, memory::allocator_type AllocatorT>
  void BitVector<BlockT, AllocatorT>::set(const usize index, const b8 value)
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
  auto BitVector<BlockT, AllocatorT>::get_bit_ref(const usize index) -> reference
  {
    return reference(blocks_ + get_block_index(index), get_block_offset(index));
  }

  template <ts_bit_block BlockT, memory::allocator_type AllocatorT>
  auto BitVector<BlockT, AllocatorT>::get_bool(const usize index) const -> b8
  {
    return blocks_[get_block_index(index)] & (BlockT(1) << get_block_offset(index));
  }

  template <ts_bit_block BlockT, memory::allocator_type AllocatorT>
  auto BitVector<BlockT, AllocatorT>::get_new_capacity(const usize old_capacity) -> usize
  {
    return old_capacity * GROWTH_FACTOR + BLOCK_BIT_COUNT;
  }

  template <ts_bit_block BlockT, memory::allocator_type AllocatorT>
  void BitVector<BlockT, AllocatorT>::init_reserve(const usize capacity)
  {
    const auto block_count = get_block_count(capacity);
    blocks_ = allocator_->template allocate_array<BlockT>(block_count);
    capacity_ = block_count * BLOCK_BIT_COUNT;
  }

  template <ts_bit_block BlockT, memory::allocator_type AllocatorT>
  void BitVector<BlockT, AllocatorT>::init_resize(const usize size, const b8 val)
  {
    const auto block_count = get_block_count(size);
    blocks_ = allocator_->template allocate_array<BlockT>(block_count);
    capacity_ = block_count * BLOCK_BIT_COUNT;
    std::uninitialized_fill_n(blocks_, block_count, val ? ~BlockT(0) : BlockT(0));
    size_ = size;
  }

  template <ts_bit_block BlockT, memory::allocator_type AllocatorT>
  auto BitVector<BlockT, AllocatorT>::get_block_count(const usize size) -> usize
  {
    return (size + BLOCK_BIT_COUNT - 1) / BLOCK_BIT_COUNT;
  }

  template <ts_bit_block BlockT, memory::allocator_type AllocatorT>
  auto BitVector<BlockT, AllocatorT>::get_block_index(const usize index) -> usize
  {
    return index / BLOCK_BIT_COUNT;
  }

  template <ts_bit_block BlockT, memory::allocator_type AllocatorT>
  auto BitVector<BlockT, AllocatorT>::get_block_offset(const usize index) -> usize
  {
    return index % BLOCK_BIT_COUNT;
  }
} // namespace soul
