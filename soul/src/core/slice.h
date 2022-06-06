//
// Created by Kevin Yudi Utama on 23/12/19.
//
#pragma once

#include "core/vector.h"
#include "core/type.h"

namespace soul {

	template <typename T>
	class Slice
	{
	public:

		Slice() = default;
		Slice(Vector<T>* array, soul_size begin, soul_size end): _array(array), _beginIdx(begin), _endIdx(end), _size(_endIdx - _beginIdx) {}
		Slice(const Slice& other) = default;
		Slice& operator=(const Slice& other) = default;
		Slice(Slice&& other) = default;
		Slice& operator=(Slice&& other) = default;
		~Slice() = default;

		void set(Vector<T>* array, soul_size begin, soul_size end) {
			_array = array;
			_beginIdx = begin;
			_endIdx = end;
			_size = _endIdx - _beginIdx;
		}

		[[nodiscard]] T& operator[](soul_size idx) {
			SOUL_ASSERT(0, idx < _size, "");
			return (*_array)[_beginIdx + idx];
		}

		[[nodiscard]] const T& operator[](soul_size idx) const {
			SOUL_ASSERT(0, idx < _size, "");
			return this->operator[](idx);
		}

		[[nodiscard]] soul_size size() const { return _size; }

		[[nodiscard]] const T* begin() const { return _array->data() + _beginIdx; }
		[[nodiscard]] const T* end() const { return _array->data() + _endIdx; }

		[[nodiscard]] T* begin() { return _array->data() + _beginIdx; }
		[[nodiscard]] T* end() { return _array->data() + _endIdx; }
	private:
		Vector<T>* _array = nullptr;
		soul_size _beginIdx = 0;
		soul_size _endIdx = 0;
		soul_size _size = 0;
	};

}