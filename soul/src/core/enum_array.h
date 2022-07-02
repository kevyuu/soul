//
// Created by Kevin Yudi Utama on 2/1/20.
//
#pragma once

#include <array>
#include "core/type.h"

namespace soul {

	template<flag EnumType, typename ValType>
	class EnumArray {
	public:

		static constexpr uint64 COUNT = to_underlying(EnumType::COUNT);

		constexpr EnumArray<EnumType, ValType>() : buffer_(){}

		constexpr explicit EnumArray<EnumType, ValType>(ValType val) noexcept {
			for (ValType& dst_val : buffer_) {
				new(&dst_val)ValType(val);
			}
		}

		template<std::size_t N>
		static consteval EnumArray<EnumType, ValType> build_from_list(const ValType (&list)[N])
		{
			static_assert(N == COUNT);
			EnumArray<EnumType, ValType> ret;
			for (soul_size i = 0; i < N; i++)
			{
				ret.buffer_[i] = list[i];
			}
			return ret;
		}

		explicit EnumArray<EnumType, ValType>(std::initializer_list<std::pair<EnumType, ValType>> init_list) noexcept : buffer_()
		{
			for (auto item : init_list)
			{
				buffer_[to_underlying(item.first)] = item.second;
			}
		}

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

		[[nodiscard]] const ValType* begin() const { return buffer_; }
		[[nodiscard]] const ValType* end() const { return buffer_ + to_underlying(EnumType::COUNT); }

		[[nodiscard]] ValType* begin() { return buffer_; }
		[[nodiscard]] ValType* end() { return buffer_ + to_underlying(EnumType::COUNT); }

	private:
		ValType buffer_[to_underlying(EnumType::COUNT)];

	};
}

