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
		Slice(Vector<T>* array, soul_size begin, soul_size end): vector_(array), begin_idx_(begin), end_idx_(end), size_(end_idx_ - begin_idx_) {}
		Slice(const Slice& other) = default;
		Slice& operator=(const Slice& other) = default;
		Slice(Slice&& other) = default;
		Slice& operator=(Slice&& other) = default;
		~Slice() = default;

		void set(Vector<T>* array, soul_size begin, soul_size end) {
			vector_ = array;
			begin_idx_ = begin;
			end_idx_ = end;
			size_ = end_idx_ - begin_idx_;
		}

		[[nodiscard]] T& operator[](soul_size idx) {
			SOUL_ASSERT(0, idx < size_, "");
			return (*vector_)[begin_idx_ + idx];
		}

		[[nodiscard]] const T& operator[](soul_size idx) const {
			SOUL_ASSERT(0, idx < size_, "");
			return this->operator[](idx);
		}

		[[nodiscard]] soul_size size() const { return size_; }

		[[nodiscard]] const T* begin() const { return vector_->data() + begin_idx_; }
		[[nodiscard]] const T* end() const { return vector_->data() + end_idx_; }

		[[nodiscard]] T* begin() { return vector_->data() + begin_idx_; }
		[[nodiscard]] T* end() { return vector_->data() + end_idx_; }
	private:
		Vector<T>* vector_ = nullptr;
		soul_size begin_idx_ = 0;
		soul_size end_idx_ = 0;
		soul_size size_ = 0;
	};

}