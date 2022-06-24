#pragma once
#include "core/type.h"
#include <numeric>

namespace soul
{
	template <flag Flag>
	class FlagSet
	{
	public:

		using flag_type = Flag;
		static constexpr soul_size FLAG_COUNT = to_underlying(Flag::COUNT);
	    using store_type = min_uint_t<1u << FLAG_COUNT>;

		// Default constructor (all 0s)
		constexpr FlagSet() = default;
		constexpr FlagSet(const FlagSet& other) = default;
		FlagSet& operator=(const FlagSet& other) = default;
		FlagSet(FlagSet&& other) = default;
		FlagSet& operator=(FlagSet&& other) = default;
		~FlagSet() = default;

		constexpr explicit FlagSet(store_type val) : flags_(val & MASK) {}

		// Initializer list constructor
		constexpr FlagSet(const std::initializer_list<flag_type>& init_list);

        FlagSet& set();
        FlagSet& set(flag_type bit, bool value = true);
        FlagSet& reset();
        FlagSet& reset(flag_type bit);
        FlagSet& flip();
        FlagSet& flip(flag_type bit);

        [[nodiscard]] bool operator[](flag_type bit) const;
        FlagSet& operator|=(FlagSet flag);
        FlagSet& operator&=(FlagSet flag);
        FlagSet& operator^=(FlagSet flag);
        [[nodiscard]] FlagSet operator~() const;
        bool operator==(const FlagSet&) const = default;

        [[nodiscard]] size_t count() const;
        [[nodiscard]] constexpr size_t size() const;
        [[nodiscard]] constexpr bool test(flag_type bit) const;
        [[nodiscard]] constexpr bool any() const;
        [[nodiscard]] constexpr bool none() const;

        [[nodiscard]] constexpr store_type val() const;

        template <std::integral DstFlags>
		[[nodiscard]] constexpr DstFlags map(DstFlags const(&mapping)[to_underlying(flag_type::COUNT)]) const;

        template <typename T>
		constexpr void for_each(T func) const;

    private:
        static constexpr soul_size MASK = (1u << FLAG_COUNT) - 1;
		store_type flags_ = 0;
	};

    template <flag Flag>
    constexpr FlagSet<Flag>::FlagSet(const std::initializer_list<flag_type>& init_list):
        flags_(std::accumulate(init_list.begin(), init_list.end(), store_type(0), [](store_type x, flag_type y)
        {
            return x | store_type(1u << to_underlying(y));
        }))
    {}

    template <flag Flag>
    constexpr size_t FlagSet<Flag>::size() const
    {
        return to_underlying(Flag::COUNT);
    }

    template <flag Flag>
    constexpr bool FlagSet<Flag>::test(flag_type bit) const
    {
        return (flags_ & (1u << bit)) > 0;
    }

    template <flag Flag>
    constexpr bool FlagSet<Flag>::any() const
    {
        return flags_ > 0;
    }

    template <flag Flag>
    constexpr bool FlagSet<Flag>::none() const
    {
        return flags_ == 0;
    }

    template <flag Flag>
    constexpr typename FlagSet<Flag>::store_type FlagSet<Flag>::val() const
    {
        return flags_;
    }

    template <flag Flag>
    template <std::integral DstFlags>
    constexpr DstFlags FlagSet<Flag>::map(DstFlags const(& mapping)[to_underlying(flag_type::COUNT)]) const
    {
        std::remove_cv_t<DstFlags> dst_flags = 0;
        store_type flags = flags_;
        while (flags)
        {
            uint32 bit = Util::trailing_zeroes(flags);
            dst_flags |= mapping[bit];
            flags &= ~(1u << bit);
        }
        return dst_flags;
    }

    template <flag Flag>
    template <typename T>
    constexpr void FlagSet<Flag>::for_each(T func) const
    {
        store_type flags = flags_;
        while (flags)
        {
            std::underlying_type_t<flag_type> bit = Util::trailing_zeroes(flags);
            func(flag_type(bit));
            flags &= ~(1u << bit);
        }
    }

    template <flag Flag>
    bool FlagSet<Flag>::operator[](flag_type bit) const
    {
        return test(bit);
    }

    template <flag Flag>
    FlagSet<Flag>& FlagSet<Flag>::set()
    {
        flags_ = MASK;
        return *this;
    }

    template <flag Flag>
    FlagSet<Flag>& FlagSet<Flag>::set(flag_type bit, bool value)
    {
        if (value)
            flags_ |= (1u << bit);
        else
            flags_ &= ~(1u << bit);
        return *this;
    }

    template <flag Flag>
    FlagSet<Flag>& FlagSet<Flag>::reset()
    {
        flags_ = store_type(0);
        return *this;
    }

    template <flag Flag>
    FlagSet<Flag>& FlagSet<Flag>::reset(flag_type bit)
    {
        flags_ &= ~(1u << bit);
        return *this;
    }

    template <flag Flag>
    FlagSet<Flag>& FlagSet<Flag>::flip()
    {
        flags_ = ~flags_;
        flags_ &= MASK;
        return *this;
    }

    template <flag Flag>
    FlagSet<Flag>& FlagSet<Flag>::flip(flag_type bit)
    {
        flags_ ^= (1u << bit);
        return *this;
    }

    template <flag Flag>
    FlagSet<Flag>& FlagSet<Flag>::operator|=(FlagSet flag)
    {
        flags_ |= flag.flags_;
        return *this;
    }

    template <flag Flag>
    FlagSet<Flag>& FlagSet<Flag>::operator&=(FlagSet flag)
    {
        flags_ &= flag.flags_;
        return *this;
    }

    template <flag Flag>
    FlagSet<Flag>& FlagSet<Flag>::operator^=(FlagSet flag)
    {
        flags_ ^= flag.flags_;
        return *this;
    }

    template <flag Flag>
    FlagSet<Flag> FlagSet<Flag>::operator~() const
    {
        return FlagSet(~this->flags_);
    }

    template <flag Flag>
    size_t FlagSet<Flag>::count() const
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

    template<flag FlagT>
	FlagSet<FlagT> operator & (const FlagSet<FlagT> lhs, const FlagSet<FlagT> rhs)
	{
		return FlagSet<FlagT>(FlagSet<FlagT>::store_type(lhs.val()) & FlagSet<FlagT>::store_type(rhs.val()));
	}

	template<flag FlagT>
	FlagSet<FlagT> operator | (const FlagSet<FlagT> lhs, const FlagSet<FlagT> rhs)
	{
		return FlagSet<FlagT>(FlagSet<FlagT>::store_type(lhs.val()) | FlagSet<FlagT>::store_type(rhs.val()));
	}

	template<flag FlagT>
	FlagSet<FlagT> operator ^ (const FlagSet<FlagT> lhs, const FlagSet<FlagT> rhs)
	{
		return FlagSet<FlagT>(FlagSet<FlagT>::store_type(lhs.val()) ^ FlagSet<FlagT>::store_type(rhs.val()));
	}
}