#pragma once

#include <iterator>

#include "core/config.h"
#include "core/dev_util.h"
#include "core/util.h"
#include "memory/allocator.h"

namespace soul {

	template <typename T>
	class Vector
	{
	public:
		using value_type = T;
		using pointer = T*;
		using const_pointer = const T*;
		using reference = T&;
		using const_reference = T&;
		using iterator = T*;
		using const_iterator = const T*;
		using reverse_iterator = std::reverse_iterator<iterator>;
		using const_reverse_iterator = std::reverse_iterator<const_iterator>;

		explicit Vector(memory::Allocator* allocator = GetDefaultAllocator());
		Vector(const Vector& other);
		Vector& operator=(const Vector& rhs);
		Vector(Vector&& other) noexcept;
		Vector& operator=(Vector&& other) noexcept;
		~Vector();

		void swap(Vector<T>& other) noexcept;
		friend void swap(Vector<T>& a, Vector<T>& b) noexcept { a.swap(b); }

		[[nodiscard]] iterator begin() { return buffer_; }
		[[nodiscard]] const_iterator begin() const { return buffer_; }
		[[nodiscard]] const_iterator cbegin() const { return buffer_; }

		[[nodiscard]] iterator end() { return buffer_ + size_; }
		[[nodiscard]] const_iterator end() const { return buffer_ + size_; }
		[[nodiscard]] const_iterator cend() const { return buffer_ + size_; }

		[[nodiscard]] reverse_iterator rbegin() { return reverse_iterator(end()); }
		[[nodiscard]] const_reverse_iterator rbegin() const { return const_reverse_iterator(cend()); }
		[[nodiscard]] const_reverse_iterator crbegin() const { return const_reverse_iterator(cend()); }

		[[nodiscard]] reverse_iterator rend() { return reverse_iterator(begin()); }
		[[nodiscard]] const_reverse_iterator rend() const { return const_reverse_iterator(cbegin()); }
		[[nodiscard]] const_reverse_iterator crend() const { return const_reverse_iterator(cbegin()); }

		[[nodiscard]] soul_size capacity() const noexcept { return capacity_; }
		[[nodiscard]] soul_size size() const noexcept { return size_; }
		[[nodiscard]] bool empty() const noexcept { return size_ == 0; }

		void init(memory::Allocator* allocator) noexcept;

		void resize(soul_size size);
		void reserve(soul_size capacity);

		void clear() noexcept;
		void cleanup();

		soul_size add(const T& item);
		soul_size add(T&& item);

		void push_back(const T& item);
		reference push_back();
		void push_back(T&& item);
		void pop_back();

		template<typename... Args>
		reference emplace_back(Args&&... args);

		void append(const Vector<T>& other);

		[[nodiscard]] pointer data() noexcept { return buffer_; }
		[[nodiscard]] const_pointer data() const noexcept { return buffer_; }

		[[nodiscard]] reference front() { SOUL_ASSERT(0, size_ != 0, "Vector cannot be empty when calling Vector::front()"); return buffer_[0]; }
		[[nodiscard]] const_reference front() const { SOUL_ASSERT(0, size_ != 0, "Vector cannot be empty when calling Vector::front()"); return buffer_[0]; }

		[[nodiscard]] reference back() noexcept { SOUL_ASSERT(0, size_ != 0, "Vector cannot be empty when calling Vector::back()"); return buffer_[size_ - 1]; }
		[[nodiscard]] const_reference back() const noexcept { SOUL_ASSERT(0, size_ != 0, "Vector cannot be empty when calling Vector::back()"); return buffer_[size_ - 1]; }

		[[nodiscard]] reference operator[](soul_size idx) {
			SOUL_ASSERT(0, idx < size_, "Out of bound access to array detected. idx = %d, _size = %d", idx, size_);
			return buffer_[idx];
		}

		[[nodiscard]] const_reference operator[](soul_size idx) const {
			SOUL_ASSERT(0, idx < size_, "Out of bound access to array detected. idx = %d, _size=%d", idx, size_);
			return buffer_[idx];
		}

	private:
		soul_size get_new_capacity(soul_size old_capacity);

		static constexpr soul_size GROWTH_FACTOR = 2;
		memory::Allocator* allocator_ = nullptr;
		T* buffer_ = nullptr;
		soul_size size_ = 0;
		soul_size capacity_ = 0;

	};

	template<typename T>
	Vector<T>::Vector(memory::Allocator* const allocator) :
		allocator_(allocator),
		buffer_(nullptr) {}

	template <typename T>
	Vector<T>::Vector(const Vector<T>& other) : allocator_(other.allocator_), buffer_(nullptr) {
		reserve(other.capacity_);
		Copy(other.buffer_, other.buffer_ + other.size_, buffer_);
		size_ = other.size_;
	}

	template <typename T>
	Vector<T>& Vector<T>::operator=(const Vector<T>& rhs) {  // NOLINT(bugprone-unhandled-self-assignment)
		Vector<T>(rhs).swap(*this);
		return *this;
	}

	template<typename T>
	Vector<T>::Vector(Vector<T>&& other) noexcept {
		allocator_ = std::move(other.allocator_);
		buffer_ = std::move(other.buffer_);
		size_ = std::move(other.size_);
		capacity_ = std::move(other.capacity_);
		other.buffer_ = nullptr;
		other.size_ = 0;
		other.capacity_ = 0;
	}

	template<typename T>
	Vector<T>& Vector<T>::operator=(Vector<T>&& other) noexcept {
		Vector<T>(std::move(other)).swap(*this);
		return *this;
	}

	template<typename T>
	Vector<T>::~Vector() {
		cleanup();
	}

	template<typename T>
	void Vector<T>::swap(Vector<T>& other) noexcept {
		using std::swap;
		swap(allocator_, other.allocator_);
		swap(buffer_, other.buffer_);
		swap(size_, other.size_);
		swap(capacity_, other.capacity_);
	}

	template<typename T>
	void Vector<T>::init(memory::Allocator* allocator) noexcept
	{
		SOUL_ASSERT(0, allocator != nullptr, "");
		SOUL_ASSERT(0, empty(), "");
		allocator_ = allocator;
	}

	template<typename T>
	void Vector<T>::reserve(soul_size capacity) {
		if (capacity < capacity_) { return; }
		T* old_buffer = buffer_;
		buffer_ = allocator_->allocate_array<T>(capacity);
		if (old_buffer != nullptr) {
			Move(old_buffer, old_buffer + capacity_, buffer_);
			Destruct(old_buffer, old_buffer + capacity_);
			allocator_->deallocate_array(old_buffer, capacity_);
		}
		capacity_ = capacity;
	}

	template<typename T>
	void Vector<T>::resize(soul_size size) {
		if (size > capacity_) {
			reserve(size);
		}
		for (soul_size i = size_; i < size; i++) {
			new (buffer_ + i) T();
		}
		size_ = size;
	}

	template<typename T>
	void Vector<T>::clear() noexcept
	{
		Destruct(buffer_, buffer_ + size_);
		size_ = 0;
	}

	template <typename T>
	void Vector<T>::cleanup() {
		clear();
		allocator_->deallocate_array(buffer_, capacity_);
		buffer_ = nullptr;
		capacity_ = 0;
	}

	template <typename T>
	soul_size Vector<T>::add(const T& item) {
		T val(item);
		return add(std::move(val));
	}

	template <typename T>
	soul_size Vector<T>::add(T&& item) {
		push_back(std::move(item));
		return size_ - 1;
	}


	template<typename T>
	void Vector<T>::push_back(const T& item)
	{
		T val(item);
		push_back(std::move(val));
	}

	template<typename T>
	typename Vector<T>::reference
	Vector<T>::push_back()
	{
		if (size == capacity_)
		{
			reserve(get_new_capacity());
		}
		pointer item = buffer_ + size_;
		new (item) T();
		++size_;
		return *item;
	}

	template<typename T>
	void Vector<T>::push_back(T&& item)
	{
		if (size_ == capacity_) {
			reserve(get_new_capacity(capacity_));
		}
		new (buffer_ + size_) T(std::move(item));
		++size_;
	}

	template<typename T>
	template<typename... Args>
	typename Vector<T>::reference
	Vector<T>::emplace_back(Args&&... args)
	{
		if (size_ == capacity_) {
			reserve(get_new_capacity(capacity));
		}
		T* item = (buffer_ + size_);
		new (item) T(std::forward<Args>(args)...);
		++size_;
		return *item;
	}

	template <typename T>
	void Vector<T>::append(const Vector<T>& other) {
		reserve(size_ + other.size_);
		for (const T& datum : other) {
			push_back(datum);
		}
	}

	template <typename T>
	soul_size Vector<T>::get_new_capacity(soul_size old_capacity)
	{
		return old_capacity * GROWTH_FACTOR + 8;
	}

	template <typename T>
	void Vector<T>::pop_back()
	{
		SOUL_ASSERT(0, size_ != 0, "Cannot pop an empty array.");
		size_--;
		(buffer_ + size_)->~T();
	}


}
