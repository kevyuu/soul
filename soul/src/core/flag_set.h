#pragma once
#include "core/type.h"

namespace soul
{
	template <flag_scope_enum ENUMT>
	class FlagSet
	{
	public:

		using enum_type = ENUMT;
		using store_type = min_uint_t<(1 << to_underlying(ENUMT::COUNT))>;

		// Default constructor (all 0s)
		constexpr FlagSet() = default;
		constexpr FlagSet(const FlagSet& other) = default;
		FlagSet& operator=(const FlagSet& other) = default;
		FlagSet(FlagSet&& other) = default;
		FlagSet& operator=(FlagSet&& other) = default;
		~FlagSet() = default;

		// Initializer list constructor
		constexpr FlagSet(const std::initializer_list<enum_type>& initList) :
			flags_(std::accumulate(initList.begin(), initList.end(), store_type(0), [](store_type x, enum_type y)
			{
				return x | store_type(1u << to_underlying(y));
			}))
		{}

		constexpr explicit FlagSet(store_type val) : flags_(val) {}

		bool operator [] (enum_type bit) const
		{
			return test(bit);
		}

		FlagSet& set()
		{
			flags_ = ~store_type(0);
			return *this;
		}

		FlagSet& reset()
		{
			flags_ = store_type(0);
			return *this;
		}

		FlagSet& reset(enum_type bit)
		{
			flags_ &= ~(1u << bit);
			return *this;
		}

		FlagSet& flip()
		{
			flags_ = ~flags_;
			return *this;
		}

		FlagSet& flip(enum_type bit)
		{
			flags_ ^= (1u << bit);
			return *this;
		}

		FlagSet& operator|=(FlagSet flag)
		{
			flags_ |= flag.flags_;
			return *this;
		}

		FlagSet& operator&=(FlagSet flag)
		{
			flags_ &= flag.flags_;
			return *this;
		}

		size_t count() const
		{
			// http://www-graphics.stanford.edu/~seander/bithacks.html#CountBitsSetKernighan

			store_type bits = flags_;
			size_t total = 0;
			for (; bits != 0; ++total)
			{
				bits &= bits - 1; // clear the least significant bit set
			}
			return total;
		}

		[[nodiscard]] constexpr size_t size() const
		{
			return sizeof(enum_type) * 8;
		}

		[[nodiscard]] constexpr bool test(enum_type bit) const
		{
			return (flags_ & (1u << bit)) > 0;
		}

		[[nodiscard]] constexpr bool any() const
		{
			return flags_ > 0;
		}

		[[nodiscard]] constexpr bool none() const
		{
			return flags_ == 0;
		}

		[[nodiscard]] constexpr store_type val() const
		{
			return flags_;
		}

		template <std::integral DstFlags>
		[[nodiscard]] constexpr DstFlags map(DstFlags const(&mapping)[to_underlying(enum_type::COUNT)]) const
		{
			std::remove_cv_t<DstFlags> dstFlags = 0;
			store_type flags = flags_;
			while (flags)
			{
				uint32 bit = Util::TrailingZeroes(flags);
				dstFlags |= mapping[bit];
				flags &= ~(1u << bit);
			}
			return dstFlags;
		}

		template <typename T>
		constexpr void forEach(T func)
		{
			store_type flags = flags_;
			while (flags)
			{
				std::underlying_type_t<enum_type> bit = Util::TrailingZeroes(flags);
				func(enum_type(bit));
				flags &= ~(1u << bit);
			}
		}
		
	private:
		store_type flags_ = 0;
	};

	template<flag_scope_enum enumT>
	FlagSet<enumT> operator & (const FlagSet<enumT> lhs, const FlagSet<enumT> rhs)
	{
		return FlagSet<enumT>(FlagSet<enumT>::store_type(lhs.val()) & FlagSet<enumT>::store_type(rhs.val()));
	}

	template<flag_scope_enum enumT>
	FlagSet<enumT> operator | (const FlagSet<enumT> lhs, const FlagSet<enumT> rhs)
	{
		return FlagSet<enumT>(FlagSet<enumT>::store_type(lhs.val()) | FlagSet<enumT>::store_type(rhs.val()));
	}

	template<flag_scope_enum enumT>
	FlagSet<enumT> operator ^ (const FlagSet<enumT> lhs, const FlagSet<enumT> rhs)
	{
		return FlagSet<enumT>(FlagSet<enumT>::store_type(lhs.val()) ^ FlagSet<enumT>::store_type(rhs.val()));
	}
}