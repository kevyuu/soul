#pragma once

#include "core/type.h"

namespace soul
{

  template <bit_block_type BlockType>
  class BitRef
  {
  public:
    using this_type = BitRef<BlockType>;
    BitRef(BlockType* ptr, soul_size bit_index);

    ~BitRef() = default;

    auto operator=(bool val) -> BitRef&;
    auto operator=(const BitRef& rhs) -> BitRef&;
    auto operator=(BitRef&& rhs) noexcept -> BitRef&;

    auto operator|=(bool val) -> BitRef&;
    auto operator&=(bool val) -> BitRef&;
    auto operator^=(bool val) -> BitRef&;

    auto operator~() const -> bool;
    operator bool() const; // NOLINT(hicpp-explicit-conversions)
    auto flip() -> BitRef&;

    auto operator&() -> void = delete; // NOLINT(google-runtime-operator)

  private:
    BitRef(const BitRef&) = default;
    BitRef(BitRef&&) noexcept = default;

    BlockType* bit_block_ = nullptr;
    soul_size bit_index_ = 0;
    BitRef() = default;

    auto set_true() -> void;
    auto set_false() -> void;

    [[nodiscard]] auto get_mask() const -> BlockType;
    auto copy_from(const BitRef& rhs) -> void;
  };

  template <bit_block_type ElementType>
  BitRef<ElementType>::BitRef(ElementType* ptr, const soul_size bit_index)
      : bit_block_(ptr), bit_index_(bit_index)
  {
  }

  template <bit_block_type ElementType>
  auto BitRef<ElementType>::operator=(const bool val) -> BitRef<ElementType>&
  {
    if (val) {
      set_true();
    } else {
      set_false();
    }
    return *this;
  }

  // NOLINTBEGIN(cert-oop54-cpp, bugprone-unhandled-self-assignment)
  template <bit_block_type ElementType>
  auto BitRef<ElementType>::operator=(const BitRef& rhs) -> BitRef<ElementType>&
  {
    *this = static_cast<bool>(rhs);
    return *this;
  }
  // NOLINTEND(cert-oop54-cpp, bugprone-unhandled-self-assignment)

  template <bit_block_type BlockType>
  auto BitRef<BlockType>::operator=(BitRef&& rhs) noexcept -> BitRef<BlockType>&
  {
    *this = static_cast<bool>(rhs);
    return *this;
  }

  template <bit_block_type BlockType>
  auto BitRef<BlockType>::operator|=(const bool val) -> BitRef<BlockType>&
  {
    if (val) {
      set_true();
    }
    return *this;
  }

  template <bit_block_type BlockType>
  auto BitRef<BlockType>::operator&=(const bool val) -> BitRef<BlockType>&
  {
    if (!val) {
      set_false();
    }
    return *this;
  }

  template <bit_block_type BlockType>
  auto BitRef<BlockType>::operator^=(const bool val) -> BitRef<BlockType>&
  {
    if (*this != val) {
      set_true();
    } else {
      set_false();
    }
    return *this;
  }

  template <bit_block_type BlockType>
  auto BitRef<BlockType>::operator~() const -> bool
  {
    return !(*this);
  }

  template <bit_block_type ElementType>
  BitRef<ElementType>::operator bool() const
  {
    return static_cast<bool>(*bit_block_ & (static_cast<ElementType>(1) << bit_index_));
  }

  template <bit_block_type BlockType>
  auto BitRef<BlockType>::flip() -> BitRef<BlockType>&
  {
    const BlockType mask = get_mask();
    if (*this) {
      set_false();
    } else {
      set_true();
    }
    return *this;
  }

  template <bit_block_type BlockType>
  auto BitRef<BlockType>::set_true() -> void
  {
    *bit_block_ |= get_mask();
  }

  template <bit_block_type BlockType>
  auto BitRef<BlockType>::set_false() -> void
  {
    *bit_block_ &= ~get_mask();
  }

  template <bit_block_type BlockType>
  auto BitRef<BlockType>::get_mask() const -> BlockType
  {
    return static_cast<BlockType>(BlockType(1) << bit_index_);
  }

  template <bit_block_type ElementType>
  auto BitRef<ElementType>::copy_from(const BitRef& rhs) -> void
  {
    bit_block_ = rhs.bit_block_;
    bit_index_ = rhs.bit_index_;
  }
} // namespace soul
