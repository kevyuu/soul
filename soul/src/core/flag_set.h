#pragma once
#include "core/bitset.h"
#include "core/type.h"
#include <numeric>

namespace soul
{
    template <flag Flag>
    class FlagSet;

    template<class T>
    struct is_flag_set : std::false_type {};

    template<flag Flag>
    struct is_flag_set<FlagSet<Flag>> : std::true_type {};

    template<typename T>
    concept flag_set = is_flag_set<T>::value;

    namespace impl {
        template<typename T>
        concept dst_flag = flag_set<T> || std::integral<T>;
    }
    
	template <flag Flag>
	class FlagSet
	{
	public:

		using flag_type = Flag;
		static constexpr soul_size FLAG_COUNT = to_underlying(Flag::COUNT);
        using store_type = Bitset<FLAG_COUNT>;

        // Default constructor (all 0s)
        constexpr FlagSet() = default;
        constexpr FlagSet(const FlagSet& other) = default;
        constexpr FlagSet& operator=(const FlagSet& other) = default;
        constexpr FlagSet(FlagSet&& other) = default;
        constexpr ~FlagSet() = default;

		// Initializer list constructor
		constexpr FlagSet(std::initializer_list<flag_type> init_list);

        constexpr FlagSet& set();
        constexpr FlagSet& set(flag_type bit, bool value = true);
        constexpr FlagSet& reset();
        constexpr FlagSet& reset(flag_type bit);
        constexpr FlagSet& flip();
        constexpr FlagSet& flip(flag_type bit);

        [[nodiscard]] constexpr bool operator[](flag_type bit) const;
        constexpr FlagSet& operator|=(FlagSet flag);
        constexpr FlagSet& operator&=(FlagSet flag);
        constexpr FlagSet& operator^=(FlagSet flag);
        [[nodiscard]] constexpr FlagSet operator~() const;
        constexpr bool operator==(const FlagSet&) const = default;

        [[nodiscard]] constexpr size_t count() const;
        [[nodiscard]] constexpr size_t size() const;
        [[nodiscard]] constexpr bool test(flag_type bit) const;
        [[nodiscard]] constexpr bool test_any(FlagSet) const;
        [[nodiscard]] constexpr bool test_any(std::initializer_list<flag_type>) const;
        [[nodiscard]] constexpr bool any() const;
        [[nodiscard]] constexpr bool none() const;

        template<soul_size IFlagCount = FLAG_COUNT>
            requires (IFlagCount <= 32)
        [[nodiscard]] constexpr uint32 to_uint32() const;

        template<soul_size IFlagCount = FLAG_COUNT>
            requires (IFlagCount <= 64)
        [[nodiscard]] constexpr uint64 to_uint64() const;

        template <impl::dst_flag DstFlags, soul_size N>
        requires (N == to_underlying(Flag::COUNT))
        [[nodiscard]] constexpr DstFlags map(DstFlags const(&mapping)[N]) const;

        template <typename T>
        requires is_lambda_v<T, void(Flag)>
		constexpr void for_each(T func) const;

        template <typename T>
        requires is_lambda_v<T, bool(Flag)>
        constexpr std::optional<Flag> find_if(T func) const;

        store_type flags_;

    private:
        explicit constexpr FlagSet(store_type flags) : flags_(flags) {}
        constexpr store_type flags_from_init_list(const std::initializer_list<flag_type> init_list);
        static constexpr soul_size MASK = (1u << FLAG_COUNT) - 1;
		
	};

    template <flag Flag>
    constexpr FlagSet<Flag>::FlagSet(const std::initializer_list<flag_type> init_list)
    {
        for (const auto val : init_list)
        {
            flags_.set(to_underlying(val));
        }
    }

    template <flag Flag>
    constexpr size_t FlagSet<Flag>::size() const
    {
        return to_underlying(Flag::COUNT);
    }

    template <flag Flag>
    constexpr bool FlagSet<Flag>::test(flag_type bit) const
    {
        return flags_.test(to_underlying(bit));
    }

    template <flag Flag>
    constexpr bool FlagSet<Flag>::test_any(FlagSet other) const
    {
        return (*this & other).any();
    }

    template <flag Flag>
    constexpr bool FlagSet<Flag>::test_any(std::initializer_list<flag_type> other) const
    {
        return (*this & FlagSet(other)).any();
    }

    template <flag Flag>
    constexpr bool FlagSet<Flag>::any() const
    {
        return flags_.any();
    }

    template <flag Flag>
    constexpr bool FlagSet<Flag>::none() const
    {
        return flags_.none();
    }

    template <flag Flag>
    template <soul_size IFlagCount> requires (IFlagCount <= 32)
    constexpr uint32 FlagSet<Flag>::to_uint32() const
    {
        return flags_.to_uint32();
    }

    template <flag Flag>
    template <soul_size IFlagCount> requires (IFlagCount <= 64)
    constexpr uint64 FlagSet<Flag>::to_uint64() const
    {
        return flags_.to_uint64();
    }

    template <flag Flag>
    template <impl::dst_flag DstFlags, soul_size N>
    requires (N == to_underlying(Flag::COUNT))
    constexpr DstFlags FlagSet<Flag>::map(DstFlags const(& mapping)[N]) const
    {
        std::remove_cv_t<DstFlags> dst_flags{};
        store_type flags = flags_;
        auto pos = flags_.find_first();
        while (pos) {
            dst_flags |= mapping[*pos];
            pos = flags_.find_next(*pos);
        }
        return dst_flags;
    }

    template <flag Flag>
    template <typename T>
    requires is_lambda_v<T, void(Flag)>
    constexpr void FlagSet<Flag>::for_each(T func) const
    {
        auto new_func = [func = std::move(func)](soul_size bit)
        {
            func(flag_type(bit));
        };
        flags_.for_each(new_func);
    }

    template <flag Flag>
    template <typename T>
    requires is_lambda_v<T, bool(Flag)>
    constexpr std::optional<Flag> FlagSet<Flag>::find_if(T func) const
    {
        auto new_func = [func = std::move(func)](soul_size bit) -> bool
        {
            return func(flag_type(bit));
        };
        const auto find_result = flags_.find_if(new_func);
        if (find_result) return static_cast<Flag>(find_result.value());
        return std::nullopt;
    }

    template <flag Flag>
    constexpr typename FlagSet<Flag>::store_type FlagSet<Flag>::flags_from_init_list(
        const std::initializer_list<flag_type> init_list)
    {
        store_type result;
        for (auto val : init_list)
        {
            result.set(to_underlying(val));
        }
        return result;
    }

    template <flag Flag>
    constexpr bool FlagSet<Flag>::operator[](flag_type bit) const
    {
        return test(bit);
    }

    template <flag Flag>
    constexpr FlagSet<Flag>& FlagSet<Flag>::set()
    {
        flags_.set();
        return *this;
    }

    template <flag Flag>
    constexpr FlagSet<Flag>& FlagSet<Flag>::set(flag_type bit, bool value)
    {
        flags_.set(to_underlying(bit), value);
        return *this;
    }

    template <flag Flag>
    constexpr FlagSet<Flag>& FlagSet<Flag>::reset()
    {
        flags_.reset();
        return *this;
    }

    template <flag Flag>
    constexpr FlagSet<Flag>& FlagSet<Flag>::reset(flag_type bit)
    {
        flags_.set(to_underlying(bit), false);
        return *this;
    }

    template <flag Flag>
    constexpr FlagSet<Flag>& FlagSet<Flag>::flip()
    {
        flags_.flip();
        return *this;
    }

    template <flag Flag>
    constexpr FlagSet<Flag>& FlagSet<Flag>::flip(flag_type bit)
    {
        flags_.flip(to_underlying(bit));
        return *this;
    }

    template <flag Flag>
    constexpr FlagSet<Flag>& FlagSet<Flag>::operator|=(FlagSet flag)
    {
        flags_ |= flag.flags_;
        return *this;
    }

    template <flag Flag>
    constexpr FlagSet<Flag>& FlagSet<Flag>::operator&=(FlagSet flag)
    {
        flags_ &= flag.flags_;
        return *this;
    }

    template <flag Flag>
    constexpr FlagSet<Flag>& FlagSet<Flag>::operator^=(FlagSet flag)
    {
        flags_ ^= flag.flags_;
        return *this;
    }

    template <flag Flag>
    constexpr FlagSet<Flag> FlagSet<Flag>::operator~() const
    {
        return FlagSet(~this->flags_);
    }

    template <flag Flag>
    constexpr size_t FlagSet<Flag>::count() const
    {
        return flags_.count();
    }

    template<flag FlagT>
	constexpr FlagSet<FlagT> operator & (const FlagSet<FlagT> lhs, const FlagSet<FlagT> rhs)
	{
        return FlagSet<FlagT>(lhs).operator&=(rhs);
	}

	template<flag FlagT>
	constexpr FlagSet<FlagT> operator | (const FlagSet<FlagT> lhs, const FlagSet<FlagT> rhs)
	{
        return FlagSet<FlagT>(lhs).operator|=(rhs);
	}

	template<flag FlagT>
	constexpr FlagSet<FlagT> operator ^ (const FlagSet<FlagT> lhs, const FlagSet<FlagT> rhs)
	{
        return FlagSet<FlagT>(lhs).operator^=(rhs);
	}
}