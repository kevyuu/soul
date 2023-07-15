#pragma once

#include <optional>

#include "core/bit_ref.h"
#include "core/type.h"
#include "core/util.h"

namespace soul
{
  namespace impl
  {
    template <size_t BlockCount, ts_bit_block BlockType>
    class BitsetImpl
    {
    public:
      using this_type = BitsetImpl<BlockCount, BlockType>;

      constexpr auto reset() -> void;

      [[nodiscard]]
      constexpr auto any() const -> bool;
      [[nodiscard]]
      constexpr auto none() const -> bool;
      [[nodiscard]]
      constexpr auto count() const -> usize;

      [[nodiscard]]
      constexpr auto find_first() const -> std::optional<usize>;
      [[nodiscard]]
      constexpr auto find_next(usize last_find_index) const -> std::optional<usize>;
      [[nodiscard]]
      constexpr auto find_last() const -> std::optional<usize>;
      [[nodiscard]]
      constexpr auto find_prev(usize last_find_index) const -> std::optional<usize>;
      template <ts_fn<bool, usize> Fn>
      [[nodiscard]]
      constexpr auto find_if(Fn fn) const -> std::optional<usize>;

      template <ts_fn<void, usize> Fn>
      constexpr auto for_each(Fn fn) const -> void;

      template <usize IBlockCount = BlockCount, ts_bit_block IBlockType = BlockType>
        requires(IBlockCount == 1 && sizeof(BlockType) <= 4)
      [[nodiscard]]
      constexpr auto to_uint32() const -> ui32;

      template <usize IBlockCount = BlockCount, ts_bit_block IBlockType = BlockType>
        requires(IBlockCount == 1 && sizeof(BlockType) <= 8)
      [[nodiscard]]
      constexpr auto to_uint64() const -> ui64;

      BlockType blocks_[BlockCount] = {};

    protected:
      constexpr auto operator&=(const this_type& rhs) -> void;
      constexpr auto operator|=(const this_type& rhs) -> void;
      constexpr auto operator^=(const this_type& rhs) -> void;

      constexpr auto flip() -> void;
      constexpr auto set() -> void;
      constexpr auto set(usize index, bool val) -> void;

      constexpr auto operator==(const this_type& x) const -> bool;

      static constexpr usize BITS_PER_BLOCK = sizeof(BlockType) * 8;
      static constexpr usize BITS_PER_BLOCK_MASK = BITS_PER_BLOCK - 1;
      static constexpr usize BLOCK_COUNT = BlockCount;

      static constexpr auto get_block_index(usize index) -> usize;
      static constexpr auto get_block_offset(usize index) -> BlockType;
      static constexpr auto get_block_one_mask(usize index) -> BlockType;
    };

    template <size_t BlockCount, ts_bit_block BlockType>
    template <ts_fn<void, usize> Fn>
    constexpr auto BitsetImpl<BlockCount, BlockType>::for_each(Fn fn) const -> void
    {
      for (usize block_index = 0; block_index < BlockCount; block_index++) {
        auto block = blocks_[block_index];
        auto get_next_block = [](const BlockType prev_block, usize prev_pos) -> BlockType {
          const BlockType mask = ~static_cast<BlockType>(cast<BlockType>(1) << prev_pos);
          return prev_block & mask;
        };
        auto pos = util::get_first_one_bit_pos(block);
        const auto block_start_index = block_index * BITS_PER_BLOCK;
        while (pos) {
          fn(block_start_index + static_cast<usize>(*pos));
          block = get_next_block(block, *pos);
          pos = util::get_first_one_bit_pos(block);
        }
      }
    }

    template <size_t BlockCount, ts_bit_block BlockType>
    template <usize IBlockCount, ts_bit_block IBlockType>
      requires(IBlockCount == 1 && sizeof(BlockType) <= 4)
    constexpr auto BitsetImpl<BlockCount, BlockType>::to_uint32() const -> ui32
    {
      return blocks_[0];
    }

    template <size_t BlockCount, ts_bit_block BlockType>
    template <usize IBlockCount, ts_bit_block IBlockType>
      requires(IBlockCount == 1 && sizeof(BlockType) <= 8)
    constexpr auto BitsetImpl<BlockCount, BlockType>::to_uint64() const -> ui64
    {
      return blocks_[0];
    }

    template <size_t BlockCount, ts_bit_block BlockType>
    constexpr auto BitsetImpl<BlockCount, BlockType>::operator&=(const this_type& rhs) -> void
    {
      for (usize i = 0; i < BlockCount; i++) {
        blocks_[i] &= rhs.blocks_[i];
      }
    }

    template <size_t BlockCount, ts_bit_block BlockType>
    constexpr auto BitsetImpl<BlockCount, BlockType>::operator|=(const this_type& rhs) -> void
    {
      for (usize i = 0; i < BlockCount; i++) {
        blocks_[i] |= rhs.blocks_[i];
      }
    }

    template <size_t BlockCount, ts_bit_block BlockType>
    constexpr auto BitsetImpl<BlockCount, BlockType>::operator^=(const this_type& rhs) -> void
    {
      for (usize i = 0; i < BlockCount; i++) {
        blocks_[i] ^= rhs.blocks_[i];
      }
    }

    template <size_t BlockCount, ts_bit_block BlockType>
    constexpr auto BitsetImpl<BlockCount, BlockType>::flip() -> void
    {
      for (usize i = 0; i < BlockCount; i++) {
        blocks_[i] = ~blocks_[i];
      }
    }

    template <size_t BlockCount, ts_bit_block BlockType>
    constexpr auto BitsetImpl<BlockCount, BlockType>::set() -> void
    {
      for (usize i = 0; i < BlockCount; i++) {
        blocks_[i] = static_cast<BlockType>(~cast<BlockType>(0));
      }
    }

    template <size_t BlockCount, ts_bit_block BlockType>
    constexpr auto BitsetImpl<BlockCount, BlockType>::set(const usize index, const bool val)
      -> void
    {
      if (val) {
        blocks_[get_block_index(index)] |= get_block_one_mask(index);
      } else {
        blocks_[get_block_index(index)] &= ~get_block_one_mask(index);
      }
    }

    template <size_t BlockCount, ts_bit_block BlockType>
    constexpr auto BitsetImpl<BlockCount, BlockType>::reset() -> void
    {
      for (usize i = 0; i < BlockCount; i++) {
        blocks_[i] = 0;
      }
    }

    template <size_t BlockCount, ts_bit_block BlockType>
    constexpr auto BitsetImpl<BlockCount, BlockType>::operator==(const this_type& x) const -> bool
    {
      for (usize i = 0; i < BlockCount; i++) {
        if (blocks_[i] != x.blocks_[i]) {
          return false;
        }
      }
      return true;
    }

    template <size_t BlockCount, ts_bit_block BlockType>
    constexpr auto BitsetImpl<BlockCount, BlockType>::any() const -> bool
    {
      for (usize i = 0; i < BlockCount; i++) {
        if (blocks_[i]) {
          return true;
        }
      }
      return false;
    }

    template <size_t BlockCount, ts_bit_block BlockType>
    constexpr auto BitsetImpl<BlockCount, BlockType>::none() const -> bool
    {
      return !any();
    }

    template <size_t BlockCount, ts_bit_block BlockType>
    constexpr auto BitsetImpl<BlockCount, BlockType>::count() const -> usize
    {
      usize n = 0;
      for (usize i = 0; i < BlockCount; i++) {
        n += util::get_one_bit_count(blocks_[i]);
      }
      return n;
    }

    template <size_t BlockCount, ts_bit_block BlockType>
    constexpr auto BitsetImpl<BlockCount, BlockType>::find_first() const -> std::optional<usize>
    {
      for (usize block_index = 0; block_index < BlockCount; block_index++) {
        std::optional<ui32> pos = util::get_first_one_bit_pos(blocks_[block_index]);
        if (pos) {
          return (block_index * BITS_PER_BLOCK) + *pos;
        }
      }
      return std::nullopt;
    }

    template <size_t BlockCount, ts_bit_block BlockType>
    constexpr auto BitsetImpl<BlockCount, BlockType>::find_next(
      const usize last_find_index) const -> std::optional<usize>
    {
      const auto start_find_index = last_find_index + 1;
      auto block_index = get_block_index(start_find_index);
      const usize block_offset = get_block_offset(start_find_index);
      if (block_index < BlockCount) {
        const auto mask = static_cast<BlockType>(~cast<BlockType>(0) << block_offset);
        BlockType block = blocks_[block_index] & mask;
        while (true) {
          const auto next_bit = util::get_first_one_bit_pos(block);
          if (next_bit) {
            return (block_index * BITS_PER_BLOCK) + *next_bit;
          }

          block_index += 1;
          if (block_index >= BlockCount) {
            break;
          }
          block = blocks_[block_index];
        }
      }

      return std::nullopt;
    }

    template <size_t BlockCount, ts_bit_block BlockType>
    constexpr auto BitsetImpl<BlockCount, BlockType>::find_last() const -> std::optional<usize>
    {
      for (auto block_index = BlockCount; block_index > 0; --block_index) {
        const auto last_bit = util::get_last_one_bit_pos(blocks_[block_index - 1]);
        if (last_bit) {
          return (block_index - 1) * BITS_PER_BLOCK + *last_bit;
        }
      }
      return std::nullopt;
    }

    template <size_t BlockCount, ts_bit_block BlockType>
    constexpr auto BitsetImpl<BlockCount, BlockType>::find_prev(
      const usize last_find_index) const -> std::optional<usize>
    {
      if (last_find_index > 0) {
        auto block_index = get_block_index(last_find_index);
        usize block_offset = get_block_offset(last_find_index);

        BlockType block = blocks_[block_index] & ((cast<BlockType>(1) << block_offset) - 1);
        while (true) {
          const auto last_bit = util::get_last_one_bit_pos(block);
          if (last_bit) {
            return (block_index * BITS_PER_BLOCK) + *last_bit;
          }
          if (block_index == 0) {
            break;
          }
          block_index -= 1;
          block = blocks_[block_index];
        }
      }

      return std::nullopt;
    }

    template <size_t BlockCount, ts_bit_block BlockType>
    template <ts_fn<bool, usize> Fn>
    constexpr auto BitsetImpl<BlockCount, BlockType>::find_if(Fn fn) const
      -> std::optional<usize>
    {
      for (usize block_index = 0; block_index < BlockCount; block_index++) {
        auto block = blocks_[block_index];
        auto get_next_block = [](const BlockType prev_block, usize prev_pos) -> BlockType {
          const BlockType mask = ~static_cast<BlockType>(cast<BlockType>(1) << prev_pos);
          return prev_block & mask;
        };
        auto pos = util::get_first_one_bit_pos(block);
        const auto block_start_index = block_index * BITS_PER_BLOCK;
        while (pos) {
          if (fn(block_start_index + static_cast<usize>(*pos))) {
            return block_start_index + static_cast<usize>(*pos);
          }
          block = get_next_block(block, *pos);
          pos = util::get_first_one_bit_pos(block);
        }
      }
      return std::nullopt;
    }

    template <size_t BlockCount, ts_bit_block BlockType>
    constexpr auto BitsetImpl<BlockCount, BlockType>::get_block_index(const usize index)
      -> usize
    {
      return index / BITS_PER_BLOCK;
    }

    template <size_t BlockCount, ts_bit_block BlockType>
    constexpr auto BitsetImpl<BlockCount, BlockType>::get_block_offset(const usize index)
      -> BlockType
    {
      return index & BITS_PER_BLOCK_MASK;
    }

    template <size_t BlockCount, ts_bit_block BlockType>
    constexpr auto BitsetImpl<BlockCount, BlockType>::get_block_one_mask(const usize index)
      -> BlockType
    {
      return static_cast<BlockType>(cast<BlockType>(1) << get_block_offset(index));
    }

    constexpr auto get_block_count(usize bit_count, usize block_size) -> usize
    {
      return bit_count == 0 ? 1 : ((bit_count - 1) / (8 * block_size)) + 1;
    }
  } // namespace impl

  template <usize BitCount>
  using default_block_type_t = std::conditional_t<
    (BitCount > 16),
    std::conditional_t<(BitCount > 32), uint64_t, uint32_t>,
    std::conditional_t<(BitCount > 8), uint16_t, uint8_t>>;

  template <usize BitCount, ts_bit_block BlockType = default_block_type_t<BitCount>>
  class Bitset
      : public impl::BitsetImpl<impl::get_block_count(BitCount, sizeof(BlockType)), BlockType>
  {
  public:
    static_assert(BitCount != 0);
    using base_type =
      impl::BitsetImpl<impl::get_block_count(BitCount, sizeof(BlockType)), BlockType>;
    using this_type = Bitset<BitCount, BlockType>;
    using value_type = bool;
    using reference_type = BitRef<BlockType>;

    constexpr auto operator[](usize index) const -> value_type;
    constexpr auto operator[](usize index) -> reference_type;

    constexpr auto operator&=(const this_type& rhs) -> this_type&;
    constexpr auto operator|=(const this_type& rhs) -> this_type&;
    constexpr auto operator^=(const this_type& rhs) -> this_type&;

    constexpr auto set() -> this_type&;
    constexpr auto set(usize index, bool val = true) -> this_type&;

    constexpr auto flip() -> this_type&;
    constexpr auto flip(usize index) -> this_type&;

    constexpr auto operator~() const -> this_type;

    constexpr auto operator==(const this_type& rhs) const -> bool;

    [[nodiscard]]
    constexpr auto test(usize index) const -> bool;
    [[nodiscard]]
    constexpr auto all() const -> bool;
    [[nodiscard]]
    constexpr auto size() const -> usize;

  private:
    constexpr auto get_block(usize bit_index) -> BlockType&;
    [[nodiscard]]
    constexpr auto get_block(usize bit_index) const -> BlockType;
    constexpr auto clear_unused_bits() -> void;
  };

  template <usize BitCount, ts_bit_block BlockType>
  constexpr auto Bitset<BitCount, BlockType>::operator[](const usize index) const -> value_type
  {
    return test(index);
  }

  template <usize BitCount, ts_bit_block BlockType>
  constexpr auto Bitset<BitCount, BlockType>::operator[](const usize index) -> reference_type
  {
    return reference_type(&get_block(index), base_type::get_block_offset(index));
  }

  template <usize BitCount, ts_bit_block BlockType>
  constexpr auto Bitset<BitCount, BlockType>::operator&=(const this_type& rhs) -> this_type&
  {
    base_type::operator&=(rhs);
    return *this;
  }

  template <usize BitCount, ts_bit_block BlockType>
  constexpr auto Bitset<BitCount, BlockType>::operator|=(const this_type& rhs) -> this_type&
  {
    base_type::operator|=(rhs);
    return *this;
  }

  template <usize BitCount, ts_bit_block BlockType>
  constexpr auto Bitset<BitCount, BlockType>::operator^=(const this_type& rhs) -> this_type&
  {
    base_type::operator^=(rhs);
    clear_unused_bits();
    return *this;
  }

  template <usize BitCount, ts_bit_block BlockType>
  constexpr auto Bitset<BitCount, BlockType>::set() -> this_type&
  {
    base_type::set();
    clear_unused_bits();
    return *this;
  }

  template <usize BitCount, ts_bit_block BlockType>
  constexpr auto Bitset<BitCount, BlockType>::set(const usize index, const bool val)
    -> this_type&
  {
    base_type::set(index, val);
    return *this;
  }

  template <usize BitCount, ts_bit_block BlockType>
  constexpr auto Bitset<BitCount, BlockType>::flip() -> this_type&
  {
    base_type::flip();
    clear_unused_bits();
    return *this;
  }

  template <usize BitCount, ts_bit_block BlockType>
  constexpr auto Bitset<BitCount, BlockType>::flip(const usize index) -> this_type&
  {
    SOUL_ASSERT(0, index < BitCount, "");
    get_block(index) ^= base_type::get_block_one_mask(index);
    return *this;
  }

  template <usize BitCount, ts_bit_block BlockType>
  constexpr auto Bitset<BitCount, BlockType>::operator~() const -> this_type
  {
    return this_type(*this).flip();
  }

  template <usize BitCount, ts_bit_block BlockType>
  constexpr auto Bitset<BitCount, BlockType>::operator==(const this_type& rhs) const -> bool
  {
    return base_type::operator==(rhs);
  }

  template <usize BitCount, ts_bit_block BlockType>
  constexpr auto Bitset<BitCount, BlockType>::test(usize index) const -> bool
  {
    SOUL_ASSERT(0, index < BitCount, "");
    BlockType block = get_block(index);
    return (block & base_type::get_block_one_mask(index));
  }

  template <usize BitCount, ts_bit_block BlockType>
  constexpr auto Bitset<BitCount, BlockType>::all() const -> bool
  {
    return base_type::count() == BitCount;
  }

  template <usize BitCount, ts_bit_block BlockType>
  constexpr auto Bitset<BitCount, BlockType>::size() const -> usize
  {
    return BitCount;
  }

  template <usize BitCount, ts_bit_block BlockType>
  constexpr auto Bitset<BitCount, BlockType>::get_block(usize bit_index) -> BlockType&
  {
    return base_type::blocks_[base_type::get_block_index(bit_index)];
  }

  template <usize BitCount, ts_bit_block BlockType>
  constexpr auto Bitset<BitCount, BlockType>::get_block(usize bit_index) const -> BlockType
  {
    return base_type::blocks_[base_type::get_block_index(bit_index)];
  }

  template <usize BitCount, ts_bit_block BlockType>
  constexpr auto Bitset<BitCount, BlockType>::clear_unused_bits() -> void
  {
    if constexpr ((BitCount & base_type::BITS_PER_BLOCK_MASK) || (BitCount == 0)) {
      constexpr auto clear_mask =
        ~(static_cast<BlockType>(~static_cast<BlockType>(0))
          << (BitCount & base_type::BITS_PER_BLOCK_MASK));
      base_type::blocks_[base_type::BLOCK_COUNT - 1] &= clear_mask;
    }
  }

  template <usize BitCount, ts_bit_block BlockType>
  constexpr auto operator&(
    const Bitset<BitCount, BlockType>& lhs, const Bitset<BitCount, BlockType>& rhs)
    -> Bitset<BitCount, BlockType>
  {
    return Bitset<BitCount, BlockType>(lhs).operator&=(rhs);
  }

  template <usize BitCount, ts_bit_block BlockType>
  constexpr auto operator|(
    const Bitset<BitCount, BlockType>& lhs, const Bitset<BitCount, BlockType>& rhs)
    -> Bitset<BitCount, BlockType>
  {
    return Bitset<BitCount, BlockType>(lhs).operator|=(rhs);
  }

  template <usize BitCount, ts_bit_block BlockType>
  constexpr auto operator^(
    const Bitset<BitCount, BlockType>& lhs, const Bitset<BitCount, BlockType>& rhs)
    -> Bitset<BitCount, BlockType>
  {
    return Bitset<BitCount, BlockType>(lhs).operator^=(rhs);
  }
} // namespace soul
