#pragma once

#include "core/config.h"
#include "core/bit_vector.h"
#include "core/dev_util.h"
#include "core/compiler.h"
#include "core/type.h"

#include "memory/allocator.h"

namespace soul {
	
    using PoolID = uint32;

	template <typename T, memory::allocator_type AllocatorType = memory::Allocator>
	class Pool {
	public:
		using this_type = Pool<T, AllocatorType>;
		using value_type = T;
		using pointer = T*;
		using const_pointer = T*;
		using reference = T&;
		using const_reference = const T&;
		using iterator = T*;
		using const_iterator = const T*;
		using reverse_iterator = std::reverse_iterator<iterator>;
		using const_reverse_iterator = std::reverse_iterator<const_iterator>;
		using size_type = PoolID;

		explicit Pool(AllocatorType* allocator = get_default_allocator()) noexcept;
		Pool(const Pool& other);
		Pool(const Pool& other, AllocatorType& allocator);
		Pool(Pool&& other) noexcept;

		Pool& operator=(const Pool& other);
		Pool& operator=(Pool&& other) noexcept;
		~Pool();

		void swap(Pool& other) noexcept;
		friend void swap(Pool& a, Pool& b) noexcept{ a.swap(b);  }

		void reserve(size_type capacity);

		template <typename... Args>
		PoolID create(Args&&... args);

        void remove(PoolID id);

		[[nodiscard]] reference operator[](PoolID id);
        [[nodiscard]] const_reference operator[](PoolID id) const;

        [[nodiscard]] pointer ptr(PoolID id);
        [[nodiscard]] const_pointer ptr(PoolID id) const;

        [[nodiscard]] size_type size() const { return size_; }
		[[nodiscard]] bool empty() const { return size_ == 0; }

		void clear();
		void cleanup();

	private:

		PoolID allocate();

		union Unit {
			T datum;
			size_type next;
			Unit() = delete;
			Unit(const Unit&) = delete;
			Unit(Unit&&) = delete;
			Unit& operator=(const Unit&) = delete;
			Unit& operator=(Unit&&) = delete;
		    ~Unit() = delete;
		};

		AllocatorType* allocator_;
		BitVector<> bit_vector_;
		Unit* buffer_ = nullptr;
		size_type capacity_ = 0;
		size_type size_ = 0;
		size_type free_list_ = 0;

        void move_units(Unit* dst);
        void copy_units(const Pool<T, AllocatorType>& other);
        void destruct_units();

		void init_reserve(size_type capacity);  

    };

	template<typename T, memory::allocator_type AllocatorType>
	Pool<T, AllocatorType>::Pool(AllocatorType* allocator) noexcept:
    allocator_(allocator), bit_vector_(allocator_) {}

	template <typename T, memory::allocator_type AllocatorType>
	Pool<T, AllocatorType>::Pool(const Pool& other) :
    allocator_(other.allocator_), bit_vector_(other.bit_vector_), buffer_(nullptr)
    {
		init_reserve(other.capacity_);
		copy_units(other);
		free_list_ = other.free_list_;
		size_ = other.size_;
	}

    template <typename T, memory::allocator_type AllocatorType>
    Pool<T, AllocatorType>::Pool(const Pool& other, AllocatorType& allocator) :
	allocator_(allocator), bit_vector_(other.bit_vector_), buffer_(nullptr)
    {
		init_reserve(other.capacity_);
		copy_units(other);
		free_list_ = other.free_list_;
		size_ = other.size_;
    }

    template <typename T, memory::allocator_type AllocatorType>
	Pool<T, AllocatorType>& Pool<T, AllocatorType>::operator=(const Pool& other) {  // NOLINT(bugprone-unhandled-self-assignment)
		Pool<T, AllocatorType>(other).swap(*this);
		return *this;
	}

	template <typename T, memory::allocator_type AllocatorType>
	Pool<T, AllocatorType>::Pool(Pool&& other) noexcept : bit_vector_(std::move(other.bit_vector_)) {
	    allocator_ = std::move(other.allocator_);
		buffer_ = std::move(other.buffer_);
		capacity_ = std::move(other.capacity_);
		size_ = std::move(other.size_);
		free_list_ = std::move(other.free_list_);

		other.buffer_ = nullptr;
		other.capacity_ = 0;
		other.size_ = 0;
		other.free_list_ = 0;
	}

	template <typename T, memory::allocator_type AllocatorType>
	Pool<T, AllocatorType>::~Pool() {
		if (allocator_ == nullptr) return;
		cleanup();
	}

	template <typename T, memory::allocator_type AllocatorType>
	void Pool<T, AllocatorType>::swap(Pool& other) noexcept {
		using std::swap;
		SOUL_ASSERT(0, allocator_ == other.allocator_, "");
		swap(bit_vector_, other.bit_vector_);
		swap(buffer_, other.buffer_);
		swap(capacity_, other.capacity_);
		swap(size_, other.size_);
		swap(free_list_, other.free_list_);
	}

	template <typename T, memory::allocator_type AllocatorType>
	Pool<T, AllocatorType>& Pool<T, AllocatorType>::operator=(Pool&& other) noexcept {
		if (this->allocator_ == other.allocator_)
			this(std::move(other)).swap(*this);
		else
			this_type(other, *allocator_).swap(*this);
		return *this;
	}

	template<typename T, memory::allocator_type AllocatorType>
	void Pool<T, AllocatorType>::reserve(size_type capacity) {
		Unit* new_buffer = allocator_->template allocate_array<Unit>(capacity);
		if (buffer_ != nullptr) {
			move_units(new_buffer);
			destruct_units();
			allocator_->deallocate(buffer_);
		}
		buffer_ = new_buffer;
		for (auto i = capacity_; i < capacity; i++) {
			buffer_[i].next = i + 1;
		}
		free_list_ = size_;
		capacity_ = capacity;

		bit_vector_.resize(capacity_);
	}

	template<typename T, memory::allocator_type AllocatorType>
	PoolID Pool<T, AllocatorType>::allocate() {
		if (size_ == capacity_) {
			reserve(capacity_ * 2 + 8);
		}
		const PoolID id = free_list_;
		free_list_ = buffer_[free_list_].next;
		size_++;
		return id;
	}

    template <typename T, memory::allocator_type AllocatorType>
    void Pool<T, AllocatorType>::move_units(Unit* dst)
    {
        if constexpr (std::is_trivially_move_constructible_v<T>)
        {
            memcpy(dst, buffer_, capacity_ * sizeof(Unit));
        }
        else
        {
            for (size_type i = 0; i < capacity_; i++)
            {
                if (bit_vector_[i])
                {
                    new (dst + i) T(std::move(buffer_[i].datum));
                }
                else
                {
                    dst[i].next = buffer_[i].next;
                }
            }
        }
    }

    template <typename T, memory::allocator_type AllocatorType>
    void Pool<T, AllocatorType>::copy_units(const Pool<T, AllocatorType>& other)
    {
        if constexpr (std::is_trivially_copyable_v<T>)
        {
            memcpy(buffer_, other.buffer_, other.size_ * sizeof(Unit));
        }
        else
        {
            for (size_type i = 0; i < capacity_; i++) {
                if (bit_vector_[i]) {
                    new(buffer_ + i) T(other.buffer_[i].datum);
                }
                else {
                    buffer_[i].next = other.buffer_[i].next;
                }
            }
        }
    }

    template <typename T, memory::allocator_type AllocatorType>
    void Pool<T, AllocatorType>::destruct_units()
    {
        if constexpr (!std::is_trivially_destructible_v<T>)
        {
            for (size_type i = 0; i < capacity_; i++) {
                if (!bit_vector_[i]) continue;
                buffer_[i].datum.~T();
            }
        }
    }

    template <typename T, memory::allocator_type AllocatorType>
    void Pool<T, AllocatorType>::init_reserve(size_type capacity)
    {
		buffer_ = allocator_->template allocate_array<Unit>(capacity);
		for (size_type i = 0; i < capacity; i++) {
			buffer_[i].next = i + 1;
		}
		free_list_ = size_;
		capacity_ = capacity;

		bit_vector_.resize(capacity_);
    }

    template <typename T, memory::allocator_type AllocatorType>
    template <typename ... Args>
    PoolID Pool<T, AllocatorType>::create(Args&&... args)
    {
        PoolID id = allocate();
        new (buffer_ + id) T(std::forward<Args>(args)...);
        bit_vector_.set(id);
        return id;
    }

    template <typename T, memory::allocator_type AllocatorType>
	void Pool<T, AllocatorType>::remove(PoolID id) {
		size_--;
		buffer_[id].datum.~T();
		buffer_[id].next = free_list_;
		free_list_ = id;
		bit_vector_[id] = false;
	}

    template <typename T, memory::allocator_type AllocatorType>
    typename Pool<T, AllocatorType>::reference Pool<T, AllocatorType>::operator[](PoolID id)
    {
        SOUL_ASSERT(0, id < capacity_, "Pool access violation");
        return buffer_[id].datum;
    }

    template <typename T, memory::allocator_type AllocatorType>
    typename Pool<T, AllocatorType>::const_reference Pool<T, AllocatorType>::operator[](PoolID id) const
    {
        SOUL_ASSERT(0, id < capacity_, "Pool access violation");
        return buffer_[id].datum;
    }

    template <typename T, memory::allocator_type AllocatorType>
    typename Pool<T, AllocatorType>::pointer Pool<T, AllocatorType>::ptr(PoolID id)
    {
        SOUL_ASSERT(0, id < capacity_, "Pool access violation");
        return &(buffer_[id].datum);
    }

    template <typename T, memory::allocator_type AllocatorType>
    typename Pool<T, AllocatorType>::const_pointer Pool<T, AllocatorType>::ptr(PoolID id) const
    {
        SOUL_ASSERT(0, id < capacity_, "Pool access violation");
        return &(buffer_[id].datum);
    }

    template <typename T, memory::allocator_type AllocatorType>
	void Pool<T, AllocatorType>::clear() {
		destruct_units();
		size_ = 0;
		free_list_ = 0;
		for (uint64 i = 0; i < capacity_; i++) {
			buffer_[i].next = i + 1;
		}
		bit_vector_.clear();
	}

	template <typename T, memory::allocator_type AllocatorType>
	void Pool<T, AllocatorType>::cleanup() {
		destruct_units();
		capacity_ = 0;
		size_ = 0;
		allocator_->deallocate(buffer_);
		buffer_ = nullptr;
		bit_vector_.cleanup();
	}

}