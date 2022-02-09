#pragma once

#include "core/compiler.h"
#include "core/config.h"
#include "core/dev_util.h"
#include "core/util.h"
#include "memory/allocator.h"

namespace soul {

	template <typename T>
	class Array
	{
	public:
		using value_type = T;

		explicit Array(memory::Allocator* allocator = GetDefaultAllocator());
		Array(const Array& other);
		Array& operator=(const Array& rhs);
		Array(Array&& other) noexcept;
		Array& operator=(Array&& other) noexcept;
		~Array();

		void swap(Array<T>& other) noexcept;
		friend void swap(Array<T>& a, Array<T>& b) noexcept { a.swap(b); }

		void init(memory::Allocator* allocator) noexcept;
		void reserve(soul_size capacity);
		void resize(soul_size size);

		void clear() noexcept;
		void cleanup();

		soul_size add(const T& item);
		soul_size add(T&& item);

		void push_back(const T& item);
		void push_back(T&& item);

		void append(const Array<T>& other);

		SOUL_NODISCARD T& back() noexcept { return buffer_[size_ - 1]; }
		SOUL_NODISCARD const T& back() const noexcept { return buffer_[size_ - 1]; }

		void pop() {
			SOUL_ASSERT(0, size_ != 0, "Cannot pop an empty array.");
			size_--;
			((T*) buffer_ + size_)->~T();
		}

		SOUL_NODISCARD T* ptr(int idx) { return &buffer_[idx]; }
		SOUL_NODISCARD T* data() noexcept { return &buffer_[0]; }
		SOUL_NODISCARD const T* data() const noexcept { return &buffer_[0]; }

		SOUL_NODISCARD T& operator[](soul_size idx) {
			SOUL_ASSERT(0, idx < size_, "Out of bound access to array detected. idx = %d, _size = %d", idx, size_);
			return buffer_[idx];
		}

		SOUL_NODISCARD const T& operator[](soul_size idx) const {
			SOUL_ASSERT(0, idx < size_, "Out of bound access to array detected. idx = %d, _size=%d", idx, size_);
			return buffer_[idx];
		}

		SOUL_NODISCARD soul_size capacity() const noexcept { return capacity_; }
		SOUL_NODISCARD soul_size size() const noexcept { return size_; }
		SOUL_NODISCARD bool empty() const noexcept {return size_ == 0; }

		SOUL_NODISCARD const T* begin() const { return buffer_; }
		SOUL_NODISCARD const T* end() const { return buffer_ + size_; }

		T* begin() { return buffer_; }
		T* end() {return buffer_ + size_; }

	private:
		static constexpr soul_size GROWTH_FACTOR = 2;
		memory::Allocator* allocator_ = nullptr;
		T* buffer_ = nullptr;
		soul_size size_ = 0;
		soul_size capacity_ = 0;

	};

	template<typename T>
	Array<T>::Array(memory::Allocator* const allocator) :
		allocator_(allocator),
		buffer_(nullptr) {}

	template <typename T>
	Array<T>::Array(const Array<T>& other) : allocator_(other.allocator_), buffer_(nullptr) {
		reserve(other.capacity_);
		Copy(other.buffer_, other.buffer_ + other.size_, buffer_);
		size_ = other.size_;
	}

	template <typename T>
	Array<T>& Array<T>::operator=(const Array<T>& rhs) {  // NOLINT(bugprone-unhandled-self-assignment)
		Array<T>(rhs).swap(*this);
		return *this;
	}

	template<typename T>
	Array<T>::Array(Array<T>&& other) noexcept {
		allocator_ = std::move(other.allocator_);
		buffer_ = std::move(other.buffer_);
		size_ = std::move(other.size_);
		capacity_ = std::move(other.capacity_);
		other.buffer_ = nullptr;
		other.size_ = 0;
		other.capacity_ = 0;
	}

	template<typename T>
	Array<T>& Array<T>::operator=(Array<T>&& other) noexcept {
		Array<T>(std::move(other)).swap(*this);
		return *this;
	}

	template<typename T>
	Array<T>::~Array() {
		cleanup();
	}

	template<typename T>
	void Array<T>::swap(Array<T>& other) noexcept {
		using std::swap;
		swap(allocator_, other.allocator_);
		swap(buffer_, other.buffer_);
		swap(size_, other.size_);
		swap(capacity_, other.capacity_);
	}

	template<typename T>
	void Array<T>::init(memory::Allocator* allocator) noexcept
	{
		SOUL_ASSERT(0, allocator != nullptr, "");
		allocator_ = allocator;
	}

	template<typename T>
	void Array<T>::reserve(soul_size capacity) {
		if (capacity < capacity_) { return; }
		T* oldBuffer = buffer_;
		buffer_ = (T*) allocator_->allocate(sizeof(T) * capacity, alignof(T));  // NOLINT(bugprone-sizeof-expression)
		if (oldBuffer != nullptr) {
			Move(oldBuffer, oldBuffer + capacity_, buffer_);
			Destruct(oldBuffer, oldBuffer + capacity_);
			allocator_->deallocate(oldBuffer, sizeof(T) * capacity_);  // NOLINT(bugprone-sizeof-expression)
		}
		capacity_ = capacity;
	}

	template<typename T>
	void Array<T>::resize(soul_size size) {
		if (size > capacity_) {
			reserve(size);
		}
		for (soul_size i = size_; i < size; i++) {
			new (buffer_ + i) T();
		}
		size_ = size;
	}

	template<typename T>
	void Array<T>::clear() noexcept
	{
		Destruct(buffer_, buffer_ + size_);
		size_ = 0;
	}

	template <typename T>
	void Array<T>::cleanup() {
		clear();
		allocator_->deallocate(buffer_, capacity_ * sizeof(T));  // NOLINT(bugprone-sizeof-expression)
		buffer_ = nullptr;
		capacity_ = 0;
	}

	template <typename T>
	soul_size Array<T>::add(const T& item) {
		T val(item);
		return add(std::move(val));
	}

	template <typename T>
	soul_size Array<T>::add(T&& item) {
		push_back(std::move(item));
		return size_ - 1;
	}

	template<typename T>
	void Array<T>::push_back(const T& item)
	{
		T val(item);
		push_back(std::move(val));
	}

	template<typename T>
	void Array<T>::push_back(T&& item)
	{
		if (size_ == capacity_) {
			reserve((capacity_ * GROWTH_FACTOR) + 8);
		}
		new (buffer_ + size_) T(std::move(item));
		++size_;
	}

	template <typename T>
	void Array<T>::append(const Array<T>& other) {
		for (const T& datum : other) {
			add(datum);
		}
	}

}
