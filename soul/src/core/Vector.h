#pragma once

#include <iterator>

#include "core/config.h"
#include "core/dev_util.h"
#include "core/util.h"
#include "memory/allocator.h"

namespace soul {

	template <typename T, memory::allocator_type AllocatorType = memory::Allocator>
	class Vector
	{
	public:
		using this_type = Vector<T, AllocatorType>;
		using value_type = T;
		using pointer = T*;
		using const_pointer = const T*;
		using reference = T&;
		using const_reference = T&;
		using iterator = T*;
		using const_iterator = const T*;
		using reverse_iterator = std::reverse_iterator<iterator>;
		using const_reverse_iterator = std::reverse_iterator<const_iterator>;

		explicit Vector(AllocatorType* allocator = get_default_allocator());
		explicit Vector(soul_size size, AllocatorType& allocator = *get_default_allocator());
		Vector(soul_size size, const value_type& value, AllocatorType& allocator = *get_default_allocator());
		Vector(const Vector& other);
		Vector(const Vector& other, AllocatorType& allocator);
		Vector(Vector&& other) noexcept;

		template <std::input_iterator Iterator>
		Vector(Iterator first, Iterator last, AllocatorType* allocator = get_default_allocator());

	    Vector& operator=(const Vector& rhs);  // NOLINT(bugprone-unhandled-self-assignment) use copy and swap
		Vector& operator=(Vector&& other) noexcept;
		~Vector();

		void assign(soul_size size, const value_type& value);

		template <std::input_iterator InputIterator>
		void assign(InputIterator first, InputIterator last);
		
		void swap(this_type& other) noexcept;
		friend void swap(this_type& a, this_type& b) noexcept { a.swap(b); }

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

		void set_allocator(AllocatorType& allocator) noexcept;
		[[nodiscard]] AllocatorType* get_allocator() const noexcept;

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
		void pop_back(soul_size count);

		template<typename... Args>
		reference emplace_back(Args&&... args);

		void append(const this_type& other);

		template<std::input_iterator InputIterator>
		void append(InputIterator first, InputIterator last);

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
		static soul_size get_new_capacity(soul_size old_capacity);
		void init_reserve(soul_size capacity);

		static constexpr soul_size GROWTH_FACTOR = 2;
		AllocatorType* allocator_ = nullptr;
		T* buffer_ = nullptr;
		soul_size size_ = 0;
		soul_size capacity_ = 0;

	};

	template <typename T, memory::allocator_type AllocatorType>
	Vector<T, AllocatorType>::Vector(AllocatorType* const allocator) :
		allocator_(allocator) {}

	template <typename T, memory::allocator_type AllocatorType>
	Vector<T, AllocatorType>::Vector(soul_size size, AllocatorType& allocator) :
    allocator_(&allocator)
    {
		init_reserve(size);
		std::uninitialized_value_construct_n(buffer_, size);
		size_ = size;
	}

	template <typename T, memory::allocator_type AllocatorType>
	Vector<T, AllocatorType>::Vector(soul_size size, const value_type& val, AllocatorType& allocator):
	allocator_(&allocator)
	{
		init_reserve(size);
		std::uninitialized_fill_n(buffer_, size, val);
		size_ = size;
	}

	template <typename T, memory::allocator_type AllocatorType>
	Vector<T, AllocatorType>::Vector(const this_type& other) : allocator_(other.allocator_), buffer_(nullptr), size_(other.size_) {
		init_reserve(other.capacity_);
		std::uninitialized_copy(other.buffer_, other.buffer_ + other.size_, buffer_);
	}

	template <typename T, memory::allocator_type AllocatorType>
	Vector<T, AllocatorType>::Vector(const this_type& other, AllocatorType& allocator) : allocator_(&allocator), buffer_(nullptr), size_(other.size_)
	{
		init_reserve(other.capacity_);
		std::uninitialized_copy(other.buffer_, other.buffer_ + other.size_, buffer_);
	}

	template <typename T, memory::allocator_type AllocatorType>
	template <std::input_iterator InputIterator>
	Vector<T, AllocatorType>::Vector(InputIterator first, InputIterator last, AllocatorType* const allocator) : allocator_(allocator)
	{
		if constexpr (std::forward_iterator<InputIterator>) {
			const soul_size size = std::distance(first, last);
			init_reserve(size);
			std::uninitialized_copy(first, last, buffer_);
			size_ = size;
		}
	    else {
			for (; first != last; ++first)
			{
				push_back(*first);
			}
		}
	}

	template <typename T, memory::allocator_type AllocatorType>
	Vector<T, AllocatorType>&
    Vector<T, AllocatorType>::operator=(const this_type& rhs) {  // NOLINT(bugprone-unhandled-self-assignment)
		this_type(rhs, *allocator_).swap(*this);
		return *this;
	}

	template <typename T, memory::allocator_type AllocatorType>
	Vector<T, AllocatorType>::Vector(this_type&& other) noexcept :
		allocator_(std::exchange(other.allocator_, nullptr)),
		buffer_(std::exchange(other.buffer_, nullptr)),
		size_(std::exchange(other.size_, 0)),
		capacity_(std::exchange(other.capacity_, 0)) {}

	template <typename T, memory::allocator_type AllocatorType>
	Vector<T, AllocatorType>&
	Vector<T, AllocatorType>::operator=(this_type&& other) noexcept {
		if (this->allocator_ == other.allocator_)
			this_type(std::move(other)).swap(*this);
		else
			this_type(other, *allocator_).swap(*this);
		return *this;
	}

	template <typename T, memory::allocator_type AllocatorType>
	Vector<T, AllocatorType>::~Vector() {
		SOUL_ASSERT(0, allocator_ != nullptr || buffer_ == nullptr, "");
		if (allocator_ == nullptr) return;
		cleanup();
	}

	template <typename T, memory::allocator_type AllocatorType>
	void Vector<T, AllocatorType>::assign(soul_size size, const value_type& value)
	{
	    if (size > capacity_)
	    {
			this_type tmp(size, value, *allocator_);
			swap(tmp);
	    }
		else if (size > size_)
		{
			std::fill_n(buffer_, size_, value);
			std::uninitialized_fill_n(buffer_ + size_, size - size_, value);
			size_ = size;
		}
		else
		{
			std::fill_n(buffer_, size, value);
			std::destroy(buffer_ + size, buffer_ + size_);
			size_ = size;
		}
	}

    template <typename T, memory::allocator_type AllocatorType>
    template <std::input_iterator InputIterator>
    void Vector<T, AllocatorType>::assign(InputIterator first, InputIterator last)
    {
		const soul_size new_size = std::distance(first, last);
		if constexpr(std::random_access_iterator<InputIterator>)
		{
			if (new_size > capacity_)
			{
				T* new_buffer = allocator_->template allocate_array<T>(new_size);
				std::uninitialized_copy(first, last, new_buffer);
				std::destroy_n(buffer_, size_);
				allocator_->deallocate_array(buffer_, capacity_);
				buffer_ = new_buffer;
				size_ = new_size;
				capacity_ = new_size;
			}
			else if (new_size > size_)
			{
				std::copy_n(first, size_, buffer_);
				std::uninitialized_copy(first + size_, first + new_size, buffer_ + size_);
				size_ = new_size;
			}
			else
			{
				std::copy_n(first, new_size, buffer_);
				std::destroy(buffer_ + new_size, buffer_ + size_);
				size_ = new_size;
			}
		}
	    else
		{
			iterator buffer_start(buffer_);
			iterator buffer_end(buffer_ + size_);
			while((buffer_start != buffer_end) && (first != last))
			{
				*buffer_start = *first;
				++first;
				++buffer_start;
			}
			if (first == last)
				pop_back(std::distance(buffer_start, buffer_end));
			else
				append(first, last);
		}
    }

    template <typename T, memory::allocator_type AllocatorType>
	void Vector<T, AllocatorType>::swap(this_type& other) noexcept {
		using std::swap;
		SOUL_ASSERT(0, allocator_ == other.allocator_, "Cannot swap container with different allocators.");
		swap(buffer_, other.buffer_);
		swap(size_, other.size_);
		swap(capacity_, other.capacity_);
	}

	template <typename T, memory::allocator_type AllocatorType>
    void Vector<T, AllocatorType>::set_allocator(AllocatorType& allocator) noexcept
    {
		if (allocator_ != nullptr && buffer_ != nullptr)
		{
			T* buffer = allocator.template allocate_array<T>(size_);
			std::uninitialized_move(buffer_, buffer_ + size_, buffer);
			allocator_->deallocate_array(buffer_, capacity_);
			buffer_ = buffer;
		}
		allocator_ = &allocator;

    }

    template <typename T, memory::allocator_type AllocatorType>
    AllocatorType * Vector<T, AllocatorType>::get_allocator() const noexcept
    {
		return allocator_;
    }

    template <typename T, memory::allocator_type AllocatorType>
	void Vector<T, AllocatorType>::reserve(const soul_size capacity) {
		if (capacity < capacity_) { return; }
		T* old_buffer = buffer_;
		buffer_ = allocator_->template allocate_array<T>(capacity);
		if (old_buffer != nullptr) {
			std::uninitialized_move(old_buffer, old_buffer + capacity_, buffer_);
			allocator_->deallocate_array(old_buffer, capacity_);
		}
		capacity_ = capacity;
	}

	template <typename T, memory::allocator_type AllocatorType>
	void Vector<T, AllocatorType>::resize(const soul_size size) {
		if (size > size_)
		{
			if (size > capacity_) reserve(size);
			std::uninitialized_value_construct(buffer_ + size_, buffer_ + size);
		}
		else
		{
			std::destroy(buffer_ + size, buffer_ + size_);
		}
		size_ = size;
	}

	template <typename T, memory::allocator_type AllocatorType>
	void Vector<T, AllocatorType>::clear() noexcept
	{
		std::destroy(buffer_, buffer_ + size_);
		size_ = 0;
	}

	template <typename T, memory::allocator_type AllocatorType>
	void Vector<T, AllocatorType>::cleanup() {
		clear();
		allocator_->deallocate_array(buffer_, capacity_);
		buffer_ = nullptr;
		capacity_ = 0;
	}

	template <typename T, memory::allocator_type AllocatorType>
	soul_size Vector<T, AllocatorType>::add(const T& item) {
		T val(item);
		return add(std::move(val));
	}

	template <typename T, memory::allocator_type AllocatorType>
	soul_size Vector<T, AllocatorType>::add(T&& item) {
		push_back(std::move(item));
		return size_ - 1;
	}


	template <typename T, memory::allocator_type AllocatorType>
	void Vector<T, AllocatorType>::push_back(const T& item)
	{
		T val(item);
		push_back(std::move(val));
	}

	template <typename T, memory::allocator_type AllocatorType>
	typename Vector<T, AllocatorType>::reference
	Vector<T, AllocatorType>::push_back()
	{
		if (size_ == capacity_)
		{
			reserve(get_new_capacity(capacity_));
		}
		pointer item = buffer_ + size_;
		new (item) T();
		++size_;
		return *item;
	}

	template <typename T, memory::allocator_type AllocatorType>
	void Vector<T, AllocatorType>::push_back(T&& item)
	{
		if (size_ == capacity_) {
			reserve(get_new_capacity(capacity_));
		}
		new (buffer_ + size_) T(std::move(item));
		++size_;
	}

	template <typename T, memory::allocator_type AllocatorType>
	template <typename... Args>
	typename Vector<T, AllocatorType>::reference
	Vector<T, AllocatorType>::emplace_back(Args&&... args)
	{
		if (size_ == capacity_) {
			reserve(get_new_capacity(capacity_));
		}
		T* item = (buffer_ + size_);
		new (item) T(std::forward<Args>(args)...);
		++size_;
		return *item;
	}

	template <typename T, memory::allocator_type AllocatorType>
	void Vector<T, AllocatorType>::append(const this_type& other) {
		append(other.begin(), other.end());
	}

    template <typename T, memory::allocator_type AllocatorType>
    template <std::input_iterator InputIterator>
    void Vector<T, AllocatorType>::append(InputIterator first, InputIterator last)
    {
		if constexpr(std::forward_iterator<InputIterator>)
		{
			const soul_size new_size = std::distance(first, last) + size_;
			reserve(new_size);
			std::uninitialized_copy(first, last, buffer_ + size_);
			size_ = new_size;
		}
		else
		{
		    for(;first!=last;++first)
		    {
				push_back(*first);
		    }
		}
    }

    template <typename T, memory::allocator_type AllocatorType>
	soul_size Vector<T, AllocatorType>::get_new_capacity(const soul_size old_capacity)
	{
		return old_capacity * GROWTH_FACTOR + 8;
	}

	template <typename T, memory::allocator_type AllocatorType>
	void Vector<T, AllocatorType>::init_reserve(soul_size capacity)
	{
		buffer_ = allocator_->template allocate_array<T>(capacity);
		capacity_ = capacity;
	}

	template <typename T, memory::allocator_type AllocatorType>
	void Vector<T, AllocatorType>::pop_back()
	{
		SOUL_ASSERT(0, size_ != 0, "Cannot pop_back an empty vector.");
		size_--;
		(buffer_ + size_)->~T();
	}

    template <typename T, memory::allocator_type AllocatorType>
    void Vector<T, AllocatorType>::pop_back(soul_size count)
    {
		SOUL_ASSERT(0, size_ > count, "Cannot pop_back more that vector size");
		size_ = size_ - count;
		std::destroy_n(buffer_ + size_, count);
    }
}
