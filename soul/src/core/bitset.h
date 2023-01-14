#pragma once
#include <optional>

#include "core/bit_ref.h"
#include "core/type.h"
#include "core/util.h"

namespace soul
{
    namespace impl
    {
        template <size_t BlockCount, bit_block_type BlockType>
        class BitsetImpl
        {
        public:
            using this_type = BitsetImpl<BlockCount, BlockType>;

            constexpr void reset();

            [[nodiscard]] constexpr bool any() const;
            [[nodiscard]] constexpr bool none() const;
            [[nodiscard]] constexpr soul_size count() const;

            [[nodiscard]] constexpr std::optional<soul_size> find_first() const;
            [[nodiscard]] constexpr std::optional<soul_size> find_next(soul_size last_find_index) const;
            [[nodiscard]] constexpr std::optional<soul_size> find_last() const;
            [[nodiscard]] constexpr std::optional<soul_size> find_prev(soul_size last_find_index) const;

            template <typename Func>
            requires is_lambda_v<Func, void(soul_size)>
            constexpr void for_each(Func func) const;

            template <soul_size IBlockCount = BlockCount, bit_block_type IBlockType = BlockType>
                requires (IBlockCount == 1 && sizeof(BlockType) <= 4)
            [[nodiscard]] constexpr uint32 to_uint32() const;

            template <soul_size IBlockCount = BlockCount, bit_block_type IBlockType = BlockType>
                requires (IBlockCount == 1 && sizeof(BlockType) <= 8)
            [[nodiscard]] constexpr uint64 to_uint64() const;
             
        protected:

            constexpr void operator&=(const this_type& rhs);
            constexpr void operator|=(const this_type& rhs);
            constexpr void operator^=(const this_type& rhs);

            constexpr void flip();
            constexpr void set();
            constexpr void set(soul_size index, bool val);

            constexpr bool operator==(const this_type& x) const;

            static constexpr soul_size BITS_PER_BLOCK = sizeof(BlockType) * 8;
            static constexpr soul_size BITS_PER_BLOCK_MASK = BITS_PER_BLOCK - 1;
            static constexpr soul_size BLOCK_COUNT = BlockCount;

            static constexpr soul_size get_block_index(soul_size index);
            static constexpr BlockType get_block_offset(soul_size index);
            static constexpr BlockType get_block_one_mask(soul_size index);
            BlockType blocks_[BlockCount] = {};

        };

        template <size_t BlockCount, bit_block_type BlockType>
        template <typename Func> requires is_lambda_v<Func, void(soul_size)>
        constexpr void BitsetImpl<BlockCount, BlockType>::for_each(Func func) const
        {
            for (soul_size block_index = 0; block_index < BlockCount; block_index++)
            {
                auto block = blocks_[block_index];
                auto get_next_block = [](const BlockType prev_block, soul_size prev_pos) -> BlockType
                {
                    const BlockType mask = ~static_cast<BlockType>(cast<BlockType>(1) << prev_pos);
                    return prev_block & mask;
                };
                auto pos = util::get_first_one_bit_pos(block);
                const soul_size block_start_index = block_index * BITS_PER_BLOCK;
                while(pos)
                {
                    func(block_start_index + static_cast<soul_size>(*pos));
                    block = get_next_block(block, *pos);
                    pos = util::get_first_one_bit_pos(block);
                }
            }
        }

        template <size_t BlockCount, bit_block_type BlockType>
        template <soul_size IBlockCount, bit_block_type IBlockType> requires (IBlockCount == 1 && sizeof(BlockType) <= 4)
        constexpr uint32 BitsetImpl<BlockCount, BlockType>::to_uint32() const
        {
            return blocks_[0];
        }

        template <size_t BlockCount, bit_block_type BlockType>
        template <soul_size IBlockCount, bit_block_type IBlockType> requires (IBlockCount == 1 && sizeof(BlockType) <= 8)
        constexpr auto BitsetImpl<BlockCount, BlockType>::to_uint64() const -> uint64
        {
            return blocks_[0];
        }

        template <size_t BlockCount, bit_block_type BlockType>
        constexpr void BitsetImpl<BlockCount, BlockType>::operator&=(const this_type& rhs)
        {
            for (soul_size i = 0; i < BlockCount; i++)
                blocks_[i] &= rhs.blocks_[i];
        }

        template <size_t BlockCount, bit_block_type BlockType>
        constexpr void BitsetImpl<BlockCount, BlockType>::operator|=(const this_type& rhs)
        {
            for (soul_size i = 0; i < BlockCount; i++)
                blocks_[i] |= rhs.blocks_[i];
        }

        template <size_t BlockCount, bit_block_type BlockType>
        constexpr void BitsetImpl<BlockCount, BlockType>::operator^=(const this_type& rhs)
        {
            for (soul_size i = 0; i < BlockCount; i++)
                blocks_[i] ^= rhs.blocks_[i];
        }

        template <size_t BlockCount, bit_block_type BlockType>
        constexpr void BitsetImpl<BlockCount, BlockType>::flip()
        {
            for (soul_size i = 0; i < BlockCount; i++)
                blocks_[i] = ~blocks_[i];
        }

        template <size_t BlockCount, bit_block_type BlockType>
        constexpr void BitsetImpl<BlockCount, BlockType>::set()
        {
            for (soul_size i = 0; i < BlockCount; i++)
                blocks_[i] = static_cast<BlockType>(~cast<BlockType>(0));
        }

        template <size_t BlockCount, bit_block_type BlockType>
        constexpr void BitsetImpl<BlockCount, BlockType>::set(const soul_size index, const bool val)
        {
            if (val)
                blocks_[get_block_index(index)] |= get_block_one_mask(index);
            else
                blocks_[get_block_index(index)] &= ~get_block_one_mask(index);
        }

        template <size_t BlockCount, bit_block_type BlockType>
        constexpr void BitsetImpl<BlockCount, BlockType>::reset()
        {
            for (soul_size i = 0; i < BlockCount; i++)
                blocks_[i] = 0;
        }

        template <size_t BlockCount, bit_block_type BlockType>
        constexpr bool BitsetImpl<BlockCount, BlockType>::operator==(const this_type& x) const
        {
            for (soul_size i = 0; i < BlockCount; i++)
                if (blocks_[i] != x.blocks_[i]) return false;
            return true;
        }

        template <size_t BlockCount, bit_block_type BlockType>
        constexpr bool BitsetImpl<BlockCount, BlockType>::any() const
        {
            for (soul_size i = 0; i < BlockCount; i++)
                if (blocks_[i]) return true;
            return false;
        }

        template <size_t BlockCount, bit_block_type BlockType>
        constexpr bool BitsetImpl<BlockCount, BlockType>::none() const
        {
            return !any();
        }

        template <size_t BlockCount, bit_block_type BlockType>
        constexpr soul_size BitsetImpl<BlockCount, BlockType>::count() const
        {
            soul_size n = 0;
            for (soul_size i = 0; i < BlockCount; i++)
                n += util::get_one_bit_count(blocks_[i]);
            return n;
        }

        template <size_t BlockCount, bit_block_type BlockType>
        constexpr std::optional<soul_size> BitsetImpl<BlockCount, BlockType>::find_first() const
        {
            for (soul_size block_index = 0; block_index < BlockCount; block_index++)
            {
                std::optional<uint32> pos = util::get_first_one_bit_pos(blocks_[block_index]);
                if (pos)
                    return (block_index * BITS_PER_BLOCK) + *pos;
            }
            return std::nullopt;
        }

        template <size_t BlockCount, bit_block_type BlockType>
        constexpr std::optional<soul_size> BitsetImpl<BlockCount, BlockType>::find_next(const soul_size last_find_index) const
        {
            const soul_size start_find_index = last_find_index + 1;
            soul_size block_index = get_block_index(start_find_index);
            const soul_size block_offset = get_block_offset(start_find_index);
            if (block_index < BlockCount)
            {
                const auto mask = static_cast<BlockType>(~cast<BlockType>(0) << block_offset);
                BlockType block = blocks_[block_index] & mask;
                while (true)
                {
                    const auto next_bit = util::get_first_one_bit_pos(block);
                    if (next_bit)
                        return (block_index * BITS_PER_BLOCK) + *next_bit;

                    block_index += 1;
                    if (block_index >= BlockCount) break;
                    block = blocks_[block_index];
                }
            }

            return std::nullopt;
        }

        template <size_t BlockCount, bit_block_type BlockType>
        constexpr std::optional<soul_size> BitsetImpl<BlockCount, BlockType>::find_last() const
        {
            for (soul_size block_index = BlockCount; block_index > 0; --block_index)
            {
                const auto last_bit = util::get_last_one_bit_pos(blocks_[block_index - 1]);
                if (last_bit)
                    return (block_index - 1) * BITS_PER_BLOCK + *last_bit;
            }
            return std::nullopt;
        }

        template <size_t BlockCount, bit_block_type BlockType>
        constexpr std::optional<soul_size> BitsetImpl<BlockCount, BlockType>::find_prev(const soul_size last_find_index) const
        {
            if (last_find_index > 0)
            {
                soul_size block_index = get_block_index(last_find_index);
                soul_size block_offset = get_block_offset(last_find_index);

                BlockType block = blocks_[block_index] & ((cast<BlockType>(1) << block_offset) - 1);
                while(true)
                {
                    const auto last_bit = util::get_last_one_bit_pos(block);
                    if (last_bit)
                        return (block_index * BITS_PER_BLOCK) + *last_bit;
                    if (block_index == 0) break;
                    block_index -= 1;
                    block = blocks_[block_index];
                }
            }

            return std::nullopt;
        }

        template <size_t BlockCount, bit_block_type BlockType>
        constexpr soul_size BitsetImpl<BlockCount, BlockType>::get_block_index(const soul_size index)
        {
            return index / BITS_PER_BLOCK;
        }

        template <size_t BlockCount, bit_block_type BlockType>
        constexpr BlockType BitsetImpl<BlockCount, BlockType>::get_block_offset(const soul_size index)
        {
            return index & BITS_PER_BLOCK_MASK;
        }

        template <size_t BlockCount, bit_block_type BlockType>
        constexpr BlockType BitsetImpl<BlockCount, BlockType>::get_block_one_mask(const soul_size index)
        {
            return static_cast<BlockType>(cast<BlockType>(1) << get_block_offset(index));
        }



        constexpr soul_size get_block_count(soul_size bit_count, soul_size block_size)
        {
            return bit_count == 0 ? 1 : ((bit_count - 1) / (8 * block_size)) + 1;
        }
    }

    template<soul_size BitCount>
    using default_block_type_t = std::conditional_t<(BitCount > 16),
        std::conditional_t<(BitCount > 32), uint64_t, uint32_t>,
        std::conditional_t<(BitCount > 8), uint16_t, uint8_t>
        >;

    template <
        soul_size BitCount,
        bit_block_type BlockType = default_block_type_t<BitCount>>
    class Bitset : public impl::BitsetImpl<
            impl::get_block_count(BitCount, sizeof(BlockType)),
            BlockType>
    {
    public:
        static_assert(BitCount != 0);
        using base_type = impl::BitsetImpl<impl::get_block_count(BitCount, sizeof(BlockType)), BlockType>;
        using this_type = Bitset<BitCount, BlockType>;
        using value_type = bool;
        using reference_type = BitRef<BlockType>;

        constexpr value_type operator[](soul_size index) const;
        constexpr reference_type operator[](soul_size index);

        constexpr this_type& operator&=(const this_type& rhs);
        constexpr this_type& operator|=(const this_type& rhs);
        constexpr this_type& operator^=(const this_type& rhs);

        constexpr this_type& set();
        constexpr this_type& set(soul_size index, bool val = true);

        constexpr this_type& flip();
        constexpr this_type& flip(soul_size index);

        constexpr this_type operator~() const;

        constexpr bool operator==(const this_type& rhs) const;

        [[nodiscard]] constexpr bool test(soul_size index) const;
        [[nodiscard]] constexpr bool all() const;
        [[nodiscard]] constexpr soul_size size() const;

    private:
        constexpr BlockType& get_block(soul_size bit_index);
        [[nodiscard]] constexpr BlockType get_block(soul_size bit_index) const;
        constexpr void clear_unused_bits();
    };

    template <soul_size BitCount, bit_block_type BlockType>
    constexpr typename Bitset<BitCount, BlockType>::value_type
    Bitset<BitCount, BlockType>::operator[](const soul_size index) const
    {
        return test(index);
    }

    template <soul_size BitCount, bit_block_type BlockType>
    constexpr typename Bitset<BitCount, BlockType>::reference_type
    Bitset<BitCount, BlockType>::operator[](const soul_size index)
    {
        return reference_type(&get_block(index), base_type::get_block_offset(index));
    }

    template <soul_size BitCount, bit_block_type BlockType>
    constexpr typename Bitset<BitCount, BlockType>::this_type& 
    Bitset<BitCount, BlockType>::operator&=(const this_type& rhs)
    {
        base_type::operator&=(rhs);
        return *this;
    }

    template <soul_size BitCount, bit_block_type BlockType>
    constexpr typename Bitset<BitCount, BlockType>::this_type& 
    Bitset<BitCount, BlockType>::operator|=(const this_type& rhs)
    {
        base_type::operator|=(rhs);
        return *this;
    }

    template <soul_size BitCount, bit_block_type BlockType>
    constexpr typename Bitset<BitCount, BlockType>::this_type& 
    Bitset<BitCount, BlockType>::operator^=(const this_type& rhs)
    {
        base_type::operator^=(rhs);
        clear_unused_bits();
        return *this;
    }

    template <soul_size BitCount, bit_block_type BlockType>
    constexpr typename Bitset<BitCount, BlockType>::this_type& 
    Bitset<BitCount, BlockType>::set()
    {
        base_type::set();
        clear_unused_bits();
        return *this;
    }

    template <soul_size BitCount, bit_block_type BlockType>
    constexpr typename Bitset<BitCount, BlockType>::this_type& 
    Bitset<BitCount, BlockType>::set(const soul_size index, const bool val)
    {
        base_type::set(index, val);
        return *this;
    }

    template <soul_size BitCount, bit_block_type BlockType>
    constexpr typename Bitset<BitCount, BlockType>::this_type& 
    Bitset<BitCount, BlockType>::flip()
    {
        base_type::flip();
        clear_unused_bits();
        return *this;
    }

    template <soul_size BitCount, bit_block_type BlockType>
    constexpr typename Bitset<BitCount, BlockType>::this_type& 
    Bitset<BitCount, BlockType>::flip(const soul_size index)
    {
        SOUL_ASSERT(0, index < BitCount, "");
        get_block(index) ^= base_type::get_block_one_mask(index);
        return *this;
    }

    template <soul_size BitCount, bit_block_type BlockType>
    constexpr typename Bitset<BitCount, BlockType>::this_type
    Bitset<BitCount, BlockType>::operator~() const
    {
        return this_type(*this).flip();
    }

    template <soul_size BitCount, bit_block_type BlockType>
    constexpr bool Bitset<BitCount, BlockType>::operator==(const this_type& rhs) const
    {
        return base_type::operator==(rhs);
    }

    template <soul_size BitCount, bit_block_type BlockType>
    constexpr bool Bitset<BitCount, BlockType>::test(soul_size index) const
    {
        SOUL_ASSERT(0, index < BitCount, "");
        BlockType block = get_block(index);
        return (block & base_type::get_block_one_mask(index));
    }

    template <soul_size BitCount, bit_block_type BlockType>
    constexpr bool Bitset<BitCount, BlockType>::all() const
    {
        return base_type::count() == BitCount;
    }

    template <soul_size BitCount, bit_block_type BlockType>
    constexpr soul_size Bitset<BitCount, BlockType>::size() const
    {
        return BitCount;
    }

    template <soul_size BitCount, bit_block_type BlockType>
    constexpr BlockType& Bitset<BitCount, BlockType>::get_block(soul_size bit_index)
    {
        return base_type::blocks_[base_type::get_block_index(bit_index)];
    }

    template <soul_size BitCount, bit_block_type BlockType>
    constexpr BlockType Bitset<BitCount, BlockType>::get_block(soul_size bit_index) const
    {
        return base_type::blocks_[base_type::get_block_index(bit_index)];
    }

    template <soul_size BitCount, bit_block_type BlockType>
    constexpr void Bitset<BitCount, BlockType>::clear_unused_bits()
    {
        if constexpr ((BitCount & base_type::BITS_PER_BLOCK_MASK) || (BitCount == 0)) {
            constexpr auto clear_mask = ~(static_cast<BlockType>(~static_cast<BlockType>(0)) << (BitCount & base_type::BITS_PER_BLOCK_MASK));
            base_type::blocks_[base_type::BLOCK_COUNT - 1] &= clear_mask;
        }
    }

    template<soul_size BitCount, bit_block_type BlockType>
    constexpr Bitset<BitCount, BlockType>
    operator&(const Bitset<BitCount, BlockType>& lhs, const Bitset<BitCount, BlockType>& rhs)
    {
        return Bitset<BitCount, BlockType>(lhs).operator&=(rhs);
    }

    template<soul_size BitCount, bit_block_type BlockType>
    constexpr Bitset<BitCount, BlockType>
    operator|(const Bitset<BitCount, BlockType>& lhs, const Bitset<BitCount, BlockType>& rhs)
    {
        return Bitset<BitCount, BlockType>(lhs).operator|=(rhs);
    }

    template<soul_size BitCount, bit_block_type BlockType>
    constexpr Bitset<BitCount, BlockType>
    operator^(const Bitset<BitCount, BlockType>& lhs, const Bitset<BitCount, BlockType>& rhs)
    {
        return Bitset<BitCount, BlockType>(lhs).operator^=(rhs);
    }
}
