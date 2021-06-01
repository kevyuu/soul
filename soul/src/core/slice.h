//
// Created by Kevin Yudi Utama on 23/12/19.
//
#pragma once
#include "core/array.h"
#include "core/type.h"

namespace Soul {

	template <typename T>
	class Slice
	{
	public:

		Slice(): _array(nullptr), _beginIdx(0), _endIdx(0), _size(0) {}
		Slice(Array<T>* array, int begin, int end): _array(array), _beginIdx(begin), _endIdx(end), _size(_endIdx - _beginIdx) {}

		Slice(const Slice& other) = delete;
		Slice& operator=(const Slice& other) = delete;
		Slice(Slice&& other) = delete;
		Slice& operator=(const Slice&& other) = delete;

		void set(Array<T>* array, int begin, int end) {
			_array = array;
			_beginIdx = begin;
			_endIdx = end;
			_size = _endIdx - _beginIdx;
		}

		inline T& operator[](uint64 idx) {
			SOUL_ASSERT(0, idx < _size, "");
			return (*_array)[_beginIdx + idx];
		}

		inline const T& operator[](uint64 idx) const {
			SOUL_ASSERT(0, idx < _size, "");
			return this->operator[](idx);
		}

		inline int size() const { return _size; }

		const T* begin() const { return _array->data() + _beginIdx; }
		const T* end() const { return _array->data() + _endIdx; }

		T* begin() { return _array->data() + _beginIdx; }
		T* end() { return _array->data() + _endIdx; }
	private:
		Array<T>* _array;
		uint32 _beginIdx;
		uint32 _endIdx;
		uint32 _size;
	};

}