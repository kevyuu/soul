//
// Created by Kevin Yudi Utama on 2/1/20.
//
#pragma once

#include <array>
#include "core/type.h"

namespace soul {

	template<flag EnumType, typename ValType>
	class FlagMap {
	public:

		static constexpr uint64 COUNT = to_underlying(EnumType::COUNT);

	    constexpr FlagMap<EnumType, ValType>() : buffer_() {}

		constexpr explicit FlagMap<EnumType, ValType>(ValType val) noexcept;

        template<std::size_t N>
		static consteval FlagMap<EnumType, ValType> build_from_list(const ValType (&list)[N]);

        constexpr FlagMap<EnumType, ValType>(std::initializer_list<std::pair<EnumType, ValType>> init_list) noexcept;

		constexpr ValType& operator[](EnumType index) noexcept {
			SOUL_ASSERT(0, to_underlying(index) < to_underlying(EnumType::COUNT), "");
			return buffer_[to_underlying(index)];
		}

		constexpr const ValType& operator[](EnumType index) const {
			SOUL_ASSERT(0, to_underlying(index) < to_underlying(EnumType::COUNT), "");
			return buffer_[to_underlying(index)];
		}

		[[nodiscard]] constexpr int size() const {
			return to_underlying(EnumType::COUNT);
		}

		[[nodiscard]] constexpr const ValType* begin() const { return buffer_; }
		[[nodiscard]] constexpr const ValType* end() const { return buffer_ + to_underlying(EnumType::COUNT); }

		[[nodiscard]] constexpr ValType* begin() { return buffer_; }
		[[nodiscard]] constexpr ValType* end() { return buffer_ + to_underlying(EnumType::COUNT); }

	private:
		ValType buffer_[to_underlying(EnumType::COUNT)];
	};

    template <flag EnumType, typename ValType>
    constexpr FlagMap<EnumType, ValType>::FlagMap(std::initializer_list<std::pair<EnumType, ValType>> init_list) noexcept: buffer_()
    {
        for (auto item : init_list)
        {
            buffer_[to_underlying(item.first)] = item.second;
        }
    }

#pragma warning( disable : 26495 )
    template <flag EnumType, typename ValType>
    constexpr FlagMap<EnumType, ValType>::FlagMap(ValType val) noexcept
    {
        for (ValType& dst_val : buffer_) {
            std::construct_at(&dst_val, val);
        }
    }
#pragma warning( default : 26495 )

    template <flag EnumType, typename ValType>
    template <std::size_t N>
    consteval FlagMap<EnumType, ValType> FlagMap<EnumType, ValType>::build_from_list(const ValType(& list)[N])
    {
        static_assert(N == COUNT);
        FlagMap<EnumType, ValType> ret;
        for (soul_size i = 0; i < N; i++)
        {
            ret.buffer_[i] = list[i];
        }
        return ret;
    }
}

