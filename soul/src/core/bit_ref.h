#pragma once
#include "core/type.h"

namespace soul {

	template<bit_block_type BlockType>
	class BitRef
	{
	public:

		BitRef(BlockType* ptr, soul_size bit_index);
		BitRef(const BitRef&) = default;
		BitRef(BitRef&&) = default;
		~BitRef() = default;

		BitRef& operator=(bool val);
		BitRef& operator=(const BitRef& rhs);
		BitRef& operator=(BitRef&& rhs) noexcept;

		BitRef& operator|=(bool val);
		BitRef& operator&=(bool val);
		BitRef& operator^=(bool val);

		bool operator~() const;
		operator bool() const;
		BitRef& flip();

		void operator&() = delete;  // NOLINT(google-runtime-operator)

	private:

		BlockType* bit_block_ = nullptr;
		soul_size bit_index_ = 0;
		BitRef() = default;

		void set_true();
		void set_false();

		BlockType get_mask() const;
		void copy_from(const BitRef& rhs);

	};

	template <bit_block_type ElementType>
	BitRef<ElementType>::BitRef(ElementType* ptr, const soul_size bit_index) :
		bit_block_(ptr), bit_index_(bit_index)
	{}

	template <bit_block_type ElementType>
	BitRef<ElementType>& BitRef<ElementType>::operator=(const bool val)
	{
		if (val)
			set_true();
		else
			set_false();
		return *this;
	}

	template <bit_block_type ElementType>
	BitRef<ElementType>& BitRef<ElementType>::operator=(const BitRef& rhs)  // NOLINT(bugprone-unhandled-self-assignment)
	{
		*this = static_cast<bool>(rhs);
		return *this;
	}

	template <bit_block_type BlockType>
	BitRef<BlockType>& BitRef<BlockType>::operator=(BitRef&& rhs) noexcept
	{
		*this = static_cast<bool>(rhs);
		return *this;
	}

	template <bit_block_type BlockType>
	BitRef<BlockType>& BitRef<BlockType>::operator|=(const bool val)
	{
		if (val)
			set_true();
		return *this;
	}

	template <bit_block_type BlockType>
	BitRef<BlockType>& BitRef<BlockType>::operator&=(const bool val)
	{
		if (!val)
			set_false();
		return *this;
	}

	template <bit_block_type BlockType>
	BitRef<BlockType>& BitRef<BlockType>::operator^=(const bool val)
	{
		if (*this != val)
			set_true();
		else
			set_false();
		return *this;
	}

	template <bit_block_type BlockType>
	bool BitRef<BlockType>::operator~() const
	{
		return !(*this);
	}

	template <bit_block_type ElementType>
	BitRef<ElementType>::operator bool() const
	{
		return static_cast<bool>(*bit_block_ & (static_cast<ElementType>(1) << bit_index_));
	}

	template <bit_block_type BlockType>
	BitRef<BlockType>& BitRef<BlockType>::flip()
	{
		const BlockType mask = get_mask();
		if (*this)
			set_false();
		else
			set_true();
		return *this;
	}

	template <bit_block_type BlockType>
	void BitRef<BlockType>::set_true()
	{
		*bit_block_ |= get_mask();
	}

	template <bit_block_type BlockType>
	void BitRef<BlockType>::set_false()
	{
		*bit_block_ &= ~get_mask();
	}

	template <bit_block_type BlockType>
	BlockType BitRef<BlockType>::get_mask() const
	{
		return static_cast<BlockType>(BlockType(1) << bit_index_);
	}

	template <bit_block_type ElementType>
	void BitRef<ElementType>::copy_from(const BitRef& rhs)
	{
		bit_block_ = rhs.bit_block_;
		bit_index_ = rhs.bit_index_;
	}
}