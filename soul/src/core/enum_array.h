//
// Created by Kevin Yudi Utama on 2/1/20.
//

#pragma once
#include "core/type.h"

#include <array>

namespace Soul {
	template<typename EnumType, typename ValType>
	class EnumArray {
	public:
		constexpr EnumArray<EnumType, ValType>() = default;
		constexpr explicit EnumArray<EnumType, ValType>(ValType val) : _buffer() {
			for (ValType& _val : _buffer) {
				_val = val;
			}
		}

		template<size_t N>
		constexpr EnumArray<EnumType, ValType>(const ValType(&list)[N]) : _buffer() {
			static_assert(N == uint64(EnumType::COUNT), "Enum array argument count mismatch!");
			int i = 0;
			for (auto val : list) {
				_buffer[i++] = val;
			}
		}

		ValType& operator[](EnumType index) {
			return _buffer[uint64(index)];
		}

		const ValType& operator[](EnumType index) const {
			return _buffer[uint64(index)];
		}

		constexpr int size() const {
			return uint64(EnumType::COUNT);
		}

		const ValType* begin() const { return _buffer; }
		const ValType* end() const { return _buffer + uint64(EnumType::COUNT); }

		ValType* begin() { return _buffer; }
		ValType* end() { return _buffer + uint64(EnumType::COUNT); }

	private:
		ValType _buffer[uint64(EnumType::COUNT)];

	};
}

