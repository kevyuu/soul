#pragma once

#include "core/type.h"

namespace soul
{

  template <ts_bit_block BlockT>
  class BitRef
  {
  public:
    using this_type = BitRef<BlockT>;
    BitRef(BlockT* ptr, usize bit_index);

    ~BitRef() = default;

    auto operator=(b8 val) -> BitRef&;

    auto operator=(const BitRef& rhs) -> BitRef&;

    auto operator=(BitRef&& rhs) noexcept -> BitRef&;

    auto operator|=(b8 val) -> BitRef&;

    auto operator&=(b8 val) -> BitRef&;

    auto operator^=(b8 val) -> BitRef&;

    auto operator~() const -> b8;

    operator b8() const; // NOLINT(hicpp-explicit-conversions)
                         //
    auto flip() -> BitRef&;

    auto operator&() -> void = delete; // NOLINT(google-runtime-operator)

  private:
    BitRef(const BitRef&) = default;

    BitRef(BitRef&&) noexcept = default;

    BlockT* bit_block_ = nullptr;

    usize bit_index_ = 0;

    BitRef() = default;

    void set_true();

    void set_false();

    [[nodiscard]]
    auto get_mask() const -> BlockT;

    auto copy_from(const BitRef& rhs) -> void;
  };

  template <ts_bit_block ElementType>
  BitRef<ElementType>::BitRef(ElementType* ptr, const usize bit_index)
      : bit_block_(ptr), bit_index_(bit_index)
  {
  }

  template <ts_bit_block ElementType>
  auto BitRef<ElementType>::operator=(const b8 val) -> BitRef<ElementType>&
  {
    if (val)
    {
      set_true();
    } else
    {
      set_false();
    }
    return *this;
  }

  // NOLINTBEGIN(cert-oop54-cpp, bugprone-unhandled-self-assignment)
  template <ts_bit_block ElementType>
  auto BitRef<ElementType>::operator=(const BitRef& rhs) -> BitRef<ElementType>&
  {
    *this = static_cast<b8>(rhs);
    return *this;
  }

  // NOLINTEND(cert-oop54-cpp, bugprone-unhandled-self-assignment)

  template <ts_bit_block BlockT>
  auto BitRef<BlockT>::operator=(BitRef&& rhs) noexcept -> BitRef<BlockT>&
  {
    *this = static_cast<b8>(rhs);
    return *this;
  }

  template <ts_bit_block BlockT>
  auto BitRef<BlockT>::operator|=(const b8 val) -> BitRef<BlockT>&
  {
    if (val)
    {
      set_true();
    }
    return *this;
  }

  template <ts_bit_block BlockT>
  auto BitRef<BlockT>::operator&=(const b8 val) -> BitRef<BlockT>&
  {
    if (!val)
    {
      set_false();
    }
    return *this;
  }

  template <ts_bit_block BlockT>
  auto BitRef<BlockT>::operator^=(const b8 val) -> BitRef<BlockT>&
  {
    if (*this != val)
    {
      set_true();
    } else
    {
      set_false();
    }
    return *this;
  }

  template <ts_bit_block BlockT>
  auto BitRef<BlockT>::operator~() const -> b8
  {
    return !(*this);
  }

  template <ts_bit_block ElementType>
  BitRef<ElementType>::operator b8() const
  {
    return static_cast<b8>(*bit_block_ & (static_cast<ElementType>(1) << bit_index_));
  }

  template <ts_bit_block BlockT>
  auto BitRef<BlockT>::flip() -> BitRef<BlockT>&
  {
    const BlockT mask = get_mask();
    if (*this)
    {
      set_false();
    } else
    {
      set_true();
    }
    return *this;
  }

  template <ts_bit_block BlockT>
  void BitRef<BlockT>::set_true()
  {
    *bit_block_ |= get_mask();
  }

  template <ts_bit_block BlockT>
  void BitRef<BlockT>::set_false()
  {
    *bit_block_ &= ~get_mask();
  }

  template <ts_bit_block BlockT>
  auto BitRef<BlockT>::get_mask() const -> BlockT
  {
    return static_cast<BlockT>(BlockT(1) << bit_index_);
  }

  template <ts_bit_block ElementType>
  void BitRef<ElementType>::copy_from(const BitRef& rhs)
  {
    bit_block_ = rhs.bit_block_;
    bit_index_ = rhs.bit_index_;
  }
} // namespace soul
