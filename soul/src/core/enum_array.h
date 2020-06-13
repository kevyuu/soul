//
// Created by Kevin Yudi Utama on 2/1/20.
//

#pragma once
#include "core/type.h"

#include <initializer_list>

template<typename EnumType, typename ValType>
class EnumArray {
public:
	constexpr EnumArray<EnumType, ValType>() = default;
	constexpr explicit EnumArray<EnumType, ValType>(ValType val) noexcept {
		for (ValType& _val : _buffer) {
			_val = val;
		}
	}
	constexpr EnumArray<EnumType, ValType>(std::initializer_list<ValType> list) noexcept {
		SOUL_ASSERT(0, list.size() == uint64(EnumType::COUNT), "");
		int i = 0;
		for (auto val : list) {
			_buffer[i++] = val;
		}
	}

	ValType operator[](uint64 index) {
	    return _buffer[index];
	}

	ValType operator[](uint64 index) const {
	    return _buffer[index];
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
	const ValType* end() const { return _buffer + EnumType::COUNT; }

	ValType* begin() { return _buffer; }
	ValType* end() {return _buffer + uint64(EnumType::COUNT); }

private:
	ValType _buffer[uint64(EnumType::COUNT)];

};

