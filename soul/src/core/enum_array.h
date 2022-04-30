//
// Created by Kevin Yudi Utama on 2/1/20.
//
#pragma once

#include <array>
#include "core/type.h"

namespace soul {

	template<counted_scoped_enum EnumType, typename ValType>
	class EnumArray {
	public:

		static constexpr uint64 COUNT = to_underlying(EnumType::COUNT);
		using init_list = std::array<ValType, COUNT>;

		constexpr EnumArray<EnumType, ValType>() : buffer_(){}

		constexpr explicit EnumArray<EnumType, ValType>(ValType val) noexcept : buffer_() {
			for (ValType& bufferVal : buffer_) {
				bufferVal = val;
			}
		}

		constexpr explicit EnumArray<EnumType, ValType>(const std::array<ValType, COUNT>& list) noexcept : buffer_() {
			int i = 0;
			for (auto val : list) {
				buffer_[i++] = val;
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

