#pragma once

#include "core/compiler.h"
#include "core/config.h"
#include "core/type.h"

namespace soul {

	template<typename T, memory::allocator_type AllocatorType = memory::Allocator>
	class FixedVector {
	public:
		using this_type = FixedVector<T, AllocatorType>;
		using value_type = T;
		using pointer = T*;
		using const_pointer = const T*;
		using reference = T&;
		using const_reference = T&;
		using iterator = T*;
		using const_iterator = const T*;
		using reverse_iterator = std::reverse_iterator<iterator>;
		using const_reverse_iterator = std::reverse_iterator<const_iterator>;

		explicit FixedVector(AllocatorType* allocator = get_default_allocator()) : allocator_(allocator) {}
		FixedVector(const FixedVector& other);
		FixedVector& operator=(const FixedVector& other) = delete;
		FixedVector(FixedVector&& other) noexcept;
		FixedVector& operator=(FixedVector&& other) noexcept = delete;
		~FixedVector();

		template<typename... Args>
		void init(AllocatorType& allocator, soul_size size, Args&&... args);

		template<typename... Args>
		void init(soul_size size, Args&&... args);

		template<typename Construct>
		void init_construct(soul_size size, Construct func);

        void cleanup();
		
		[[nodiscard]] pointer data() { return &buffer_[0]; }
		[[nodiscard]] const_pointer data() const { return &buffer_[0];}

		[[nodiscard]] reference operator[](soul_size idx);
        [[nodiscard]] const_reference operator[](soul_size idx) const;

        [[nodiscard]] soul_size size() const { return size_; }

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

	private:
		AllocatorType* allocator_;
		T* buffer_ = nullptr;
		soul_size size_ = 0;
	};

	template <typename T, memory::allocator_type AllocatorType>
	FixedVector<T, AllocatorType>::FixedVector(const FixedVector<T, AllocatorType>& other) :
    allocator_(other.allocator_), size_(other.size_) {
		buffer_ = allocator_->template allocate_array<T>(size_);
		std::uninitialized_copy_n(other.buffer_, other.size_, buffer_);
	}

	template <typename T, memory::allocator_type AllocatorType>
	FixedVector<T, AllocatorType>::FixedVector(FixedVector&& other) noexcept {
		allocator_ = std::move(other.allocator_);
		buffer_ = std::move(other.buffer_);
		size_ = std::move(other.size_);
		other.buffer_ = nullptr;
	}

    template <typename T, memory::allocator_type AllocatorType>
    FixedVector<T, AllocatorType>::~FixedVector()
    {
		if (allocator_ == nullptr) return;
		cleanup();
    }

    template <typename T, memory::allocator_type AllocatorType>
    template <typename... Args>
    void FixedVector<T, AllocatorType>::init(AllocatorType& allocator, soul_size size, Args&&... args) {
        SOUL_ASSERT(0, size != 0, "");
		SOUL_ASSERT(0, size_ == 0, "Array have been initialized before");
        SOUL_ASSERT(0, buffer_ == nullptr, "Array have been initialized before");
        SOUL_ASSERT(0, allocator_ == nullptr, "");
        allocator_ = &allocator;
        init(size, std::forward<Args>(args) ...);
    }

	template <typename T, memory::allocator_type AllocatorType>
	template <typename... Args>
	void FixedVector<T, AllocatorType>::init(const soul_size size, Args&&... args) {
		SOUL_ASSERT(0, size != 0, "");
		SOUL_ASSERT(0, size_ == 0, "Array have been initialized before");
		SOUL_ASSERT(0, buffer_ == nullptr, "Array have been initialized before");
		size_ = size;
		buffer_ = allocator_->template allocate_array<T>(size_);
		for (soul_size i = 0; i < size_; i++) {
			new (buffer_ + i) T(std::forward<Args>(args) ... );
		}
	}

    template <typename T, memory::allocator_type AllocatorType>
    template <typename Construct>
    void FixedVector<T, AllocatorType>::init_construct(const soul_size size, Construct func)
    {
        size_ = size;
		buffer_ = allocator_->template allocate_array<T>(size_);
        for (soul_size i = 0; i < size_; i++) {
            func(i, buffer_ + i);
        }
    }

    template <typename T, memory::allocator_type AllocatorType>
	void FixedVector<T, AllocatorType>::cleanup() {
		std::destroy_n(buffer_, size_);
		allocator_->deallocate(buffer_);
		buffer_ = nullptr;
	}

    template <typename T, memory::allocator_type AllocatorType>
    typename FixedVector<T, AllocatorType>::reference
    FixedVector<T, AllocatorType>::operator[](soul_size idx)
    {
        SOUL_ASSERT(0, idx < size_, "Out of bound access to array detected. idx = %d, _size = %d", idx, size_);
        return buffer_[idx];
    }

    template <typename T, memory::allocator_type AllocatorType>
    typename FixedVector<T, AllocatorType>::const_reference
    FixedVector<T, AllocatorType>::operator[](soul_size idx) const
    {
        SOUL_ASSERT(0, idx < size_, "Out of bound access to array detected. idx = %d, _size=%d", idx, size_);
        return buffer_[idx];
    }
}
