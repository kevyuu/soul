//
// Created by Kevin Yudi Utama on 2/1/20.
//
#pragma once

#include "core/type.h"

namespace Soul {

	template<typename EnumType, typename ValType>
	class EnumArray {
	public:
		constexpr EnumArray<EnumType, ValType>() : _buffer(){}

		constexpr explicit EnumArray<EnumType, ValType>(ValType val) noexcept : _buffer() {
			for (ValType& bufferVal : _buffer) {
				bufferVal = val;
			}
		}

		template<size_t N>
		constexpr explicit EnumArray<EnumType, ValType>(const ValType(&list)[N]) noexcept : _buffer() {
			static_assert(N == uint64(EnumType::COUNT), "Enum array argument count mismatch!");
			int i = 0;
			for (auto val : list) {
				_buffer[i++] = val;
			}
		}

		ValType& operator[](EnumType index) noexcept {
			return _buffer[uint64(index)];
		}

		const ValType& operator[](EnumType index) const {
			return _buffer[uint64(index)];
		}

		[[nodiscard]] constexpr int size() const {
			return uint64(EnumType::COUNT);
		}

		[[nodiscard]] const ValType* begin() const { return _buffer; }
		[[nodiscard]] const ValType* end() const { return _buffer + uint64(EnumType::COUNT); }

		[[nodiscard]] ValType* begin() { return _buffer; }
		[[nodiscard]] ValType* end() { return _buffer + uint64(EnumType::COUNT); }

	private:
		ValType _buffer[uint64(EnumType::COUNT)];

	};
}

